#include <ldrp.h>

typedef void (NTAPI *LdrpMergeNodesProcessListCallback)(PLDR_DDAG_NODE Root, PLDRP_CSLIST_ENTRY DependencyEntry);

void
NTAPI
LdrpMergeNodesProcessListDependenciesCallback(PLDR_DDAG_NODE Root, PLDRP_CSLIST_ENTRY DependencyEntry)
{
    DependencyEntry->DependenciesLink.Next = NULL;
}

void
NTAPI
LdrpMergeNodesProcessListIncomingDependenciesCallback(PLDR_DDAG_NODE Root, PLDRP_CSLIST_ENTRY DependencyEntry)
{
    --Root->LoadCount;
    LdrpHeapFree(0, DependencyEntry);
}

void
NTAPI
LdrpMergeNodesProcessList(PLDRP_CSLIST List, PLDR_DDAG_NODE Root, LONG LinkOffset, LONG NodeOffset,
                          LdrpMergeNodesProcessListCallback Callback)
{
    const PSINGLE_LIST_ENTRY DependencyHead = List->Tail;

    PSINGLE_LIST_ENTRY LastNormalDependency = DependencyHead;

    // Note the difference in the loop's __iteration_expression__.
    // It uses the Next pointer of the LastNormalDependency, not ListEntry.
    // It is done so that we can modify the list as we please.
    for (PSINGLE_LIST_ENTRY ListEntry = DependencyHead ? DependencyHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != DependencyHead) ? LastNormalDependency->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = PTR_SUB_OFFSET(ListEntry, LinkOffset);
        const PLDR_DDAG_NODE DependencyNode = *(PLDR_DDAG_NODE*)PTR_ADD_OFFSET(DependencyEntry, NodeOffset);

        if (DependencyNode != Root)
        {
            // This is a normal dependency, not a cycle.
            // It's the base entry from which to search for next normal dependency, save it as one.
            LastNormalDependency = ListEntry;
            continue;
        }

        // Cycle appeared after merging the modules into one.
        // e.g. Before merging it could have been like A->B->A. Now it's A->A.

        // First, drop the entry from the dependency list, so that the next iteration will check
        // the entry after this ("faulty") one.
        LastNormalDependency->Next = ListEntry->Next;

        if (ListEntry == DependencyHead)
        {
            // Last iteration of the loop

            // Were there any other dependency entries, except for this one?
            // If there were, set the list tail to the last normal dependency entry.
            // Otherwise, delete the whole list.

            List->Tail = (LastNormalDependency != DependencyHead) ? LastNormalDependency : NULL;
        }

        Callback(Root, DependencyEntry);
    }
}

PLDRP_CSLIST_ENTRY
NTAPI
LdrpMergeNodesSearchList(LDRP_CSLIST List, PLDR_DDAG_NODE DependencyNode, LONG LinkOffset, LONG NodeOffset)
{
    const PSINGLE_LIST_ENTRY ListHead = List.Tail;
    PSINGLE_LIST_ENTRY ListEntry;

    for (ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != ListHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY RootDependencyEntry = PTR_SUB_OFFSET(ListEntry, LinkOffset);

        const PLDR_DDAG_NODE RootDependencyNode = *(PLDR_DDAG_NODE*)PTR_ADD_OFFSET(RootDependencyEntry, NodeOffset);

        if (RootDependencyNode == DependencyNode)
            break;
    }

    return PTR_SUB_OFFSET(ListEntry, LinkOffset);
}

void
NTAPI
LdrpMergeNodesEnsureLink(
    PLDRP_CSLIST List,
    PLDR_DDAG_NODE Root,
    PSINGLE_LIST_ENTRY DependencyListEntry,
    LONG LinkOffset,
    LONG NodeOffset,
    LONG OppositeLinkOffset,
    LONG OppositeListOffset,
    LdrpMergeNodesProcessListCallback Callback
)
{
    const PLDRP_CSLIST_ENTRY Dependency = PTR_SUB_OFFSET(DependencyListEntry, LinkOffset);
    const PLDR_DDAG_NODE DependencyNode = *(PLDR_DDAG_NODE*)PTR_ADD_OFFSET(Dependency, NodeOffset);

    if (DependencyNode == Root)
    {
        // Wow. Such luck. A cycle. End the list.
        Callback(Root, Dependency);

        return;
    }

    // First, check if the list even exists.
    if (!List->Tail)
    {
        // Easy case. Add this single dependency to the non-existent list.

        // It's the only item in the CSLIST. Remember the contract? Circular list!
        DependencyListEntry->Next = DependencyListEntry;
        List->Tail = DependencyListEntry;

        return;
    }

    // Well. The list is exists, and therefore, non-empty.

    // Find link: Root->DependencyNode.
    const PLDRP_CSLIST_ENTRY ListEntry = LdrpMergeNodesSearchList(
        *List,
        DependencyNode,
        LinkOffset,
        NodeOffset
    );

    if (ListEntry)
    {
        // Found it. Preserve the RootDependencyEntry, since the found link is older,
        // than the link we would create in case we didn't find the RootDependencyNode.
        //
        // It also means that we must destroy Dependency (it is a duplicate Root->DependencyNode link).
        //
        // But first, we must drop the link in opposite direction.

        // Remove the Dependency from the list
        LdrpRemoveDependencyEntry(PTR_ADD_OFFSET(DependencyNode, OppositeListOffset), PTR_ADD_OFFSET(Dependency, OppositeLinkOffset));

        // Dereference the node LoadCount (the number of incoming dependencies has dropped after merge)
        --Dependency->DependencyNode->LoadCount;

        // Destroy the link we no longer need
        LdrpHeapFree(0, Dependency);
    }
    else
    {
        // We didn't find the link in Root->Dependencies list. Create one.
        PushEntryList(List->Tail, DependencyListEntry);
        List->Tail = DependencyListEntry;
    }
}

void
NTAPI
LdrpMergeNodes(PLDR_DDAG_NODE Root, PSINGLE_LIST_ENTRY Children)
{
    const PLDR_DATA_TABLE_ENTRY RootLdrEntry = CONTAINING_RECORD(Root->Modules.Flink, LDR_DATA_TABLE_ENTRY,
                                                                 NodeModuleLink);

    DPRINT1("LDR: %s(%p[%wZ], %p): Merging a cycle.\n",
            __FUNCTION__,
            Root, &RootLdrEntry->BaseDllName, Children);

    for (PSINGLE_LIST_ENTRY Entry = Children->Next; Entry; Entry = Entry->Next)
    {
        const PLDR_DDAG_NODE Node = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);

        RtlpCheckListEntry(&Node->Modules);

        const PLIST_ENTRY NodeModuleLink = Node->Modules.Flink;

        RtlpCheckListEntry(NodeModuleLink);

        const PLDR_DATA_TABLE_ENTRY NodeEntry = CONTAINING_RECORD(NodeModuleLink, LDR_DATA_TABLE_ENTRY,
                                                                  NodeModuleLink);

        RemoveEntryList(NodeModuleLink);

        DPRINT1("LDR: %s(%p[%wZ], %p): Adding cyclic module %wZ.\n",
                __FUNCTION__,
                Root, &RootLdrEntry->BaseDllName, Children, &NodeEntry->BaseDllName);

        NodeEntry->DdagNode = Root;

        InsertTailList(&Root->Modules, NodeModuleLink);

        Root->LoadCount += Node->LoadCount;
        Node->LoadCount = 0;
        Node->State = LdrModulesMerged;

        const PSINGLE_LIST_ENTRY DependencyHead = Node->Dependencies.Tail;
        for (PSINGLE_LIST_ENTRY ListEntry = DependencyHead ? DependencyHead->Next : NULL;
             ListEntry;
             ListEntry = (ListEntry != DependencyHead) ? ListEntry->Next : NULL)
        {
            const PLDRP_CSLIST_ENTRY DependencyEntry =
                CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
            DependencyEntry->ParentNode = Root;
        }

        const PSINGLE_LIST_ENTRY IncomingDependencyHead = Node->IncomingDependencies.Tail;
        for (PSINGLE_LIST_ENTRY ListEntry = IncomingDependencyHead ? IncomingDependencyHead->Next : NULL;
             ListEntry;
             ListEntry = (ListEntry != IncomingDependencyHead) ? ListEntry->Next : NULL)
        {
            const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY,
                                                                         IncomingDependenciesLink);

            DependencyEntry->DependencyNode = Root;
        }
    }

    // Process all Root->Dependencies (OUTGOING direction)

    LdrpMergeNodesProcessList(
        &Root->Dependencies,
        Root,
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependenciesLink),
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependencyNode),
        LdrpMergeNodesProcessListDependenciesCallback
    );

    for (PSINGLE_LIST_ENTRY ChildEntry = Children->Next; ChildEntry; ChildEntry = ChildEntry->Next)
    {
        const PLDR_DDAG_NODE Node = CONTAINING_RECORD(ChildEntry, LDR_DDAG_NODE, CondenseLink);

        while (Node->Dependencies.Tail)
        {
            const PSINGLE_LIST_ENTRY DependencyListEntry = Node->Dependencies.Tail->Next;

            // Are there any other dependencies?
            if (DependencyListEntry == Node->Dependencies.Tail)
            {
                // The loop consists of just one dependency entry. Kill the whole list.
                Node->Dependencies.Tail = NULL;
            }
            else
            {
                // Yep, there are. Drop a single tail entry.
                Node->Dependencies.Tail->Next = DependencyListEntry->Next;
            }

            if (!DependencyListEntry)
                break;

            // There *is*, will be *was*, more than one dependency.
            // We need to make sure that this dependency is already stored in Root->Dependencies list.

            LdrpMergeNodesEnsureLink(
                &Root->Dependencies,
                Root,
                DependencyListEntry,
                FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependenciesLink),
                FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependencyNode),
                FIELD_OFFSET(LDRP_CSLIST_ENTRY, IncomingDependenciesLink),
                FIELD_OFFSET(LDR_DDAG_NODE, IncomingDependencies),
                LdrpMergeNodesProcessListDependenciesCallback
            );
        }
    }

    // Now do (almost) the same thing with Root->IncomingDependencies (INCOMING direction)
    // No comments this time.

    LdrpMergeNodesProcessList(
        &Root->IncomingDependencies,
        Root,
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, IncomingDependenciesLink),
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, ParentNode),
        LdrpMergeNodesProcessListIncomingDependenciesCallback
    );

    for (PSINGLE_LIST_ENTRY ChildEntry = Children->Next; ChildEntry; ChildEntry = ChildEntry->Next)
    {
        const PLDR_DDAG_NODE Node = CONTAINING_RECORD(ChildEntry, LDR_DDAG_NODE, CondenseLink);

        while (Node->IncomingDependencies.Tail)
        {
            const PSINGLE_LIST_ENTRY DependencyListEntry = Node->IncomingDependencies.Tail->Next;

            if (DependencyListEntry == Node->IncomingDependencies.Tail)
            {
                Node->IncomingDependencies.Tail = NULL;
            }
            else
            {
                Node->IncomingDependencies.Tail->Next = DependencyListEntry->Next;
            }

            if (!DependencyListEntry)
                break;

            LdrpMergeNodesEnsureLink(
                &Root->IncomingDependencies,
                Root,
                DependencyListEntry,
                FIELD_OFFSET(LDRP_CSLIST_ENTRY, IncomingDependenciesLink),
                FIELD_OFFSET(LDRP_CSLIST_ENTRY, ParentNode),
                FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependenciesLink),
                FIELD_OFFSET(LDR_DDAG_NODE, Dependencies),
                LdrpMergeNodesProcessListIncomingDependenciesCallback
            );
        }
    }
}


// Non-NULL for distiction ON_STACK vs CROSS-EDGE.
const PVOID INITIAL_CONDENSE_LINK = (PVOID)(ULONG_PTR)1u;

void
NTAPI
LdrpCondenseGraphRecurse(LDR_DDAG_NODE* Node, ULONG32* PreorderNumberStorage, PSINGLE_LIST_ENTRY Stack)
{
    // Tarjan's strongly connected components algorithm

    *PreorderNumberStorage += 1;

    DPRINT1("CONDENSE IN %wZ (%lu):", &(CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink)->BaseDllName), *PreorderNumberStorage);

    for (PSINGLE_LIST_ENTRY Entry = Stack->Next; Entry; Entry = Entry->Next)
    {
        if (Entry != INITIAL_CONDENSE_LINK)
        {
            const PLDR_DDAG_NODE Node = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);
            DbgPrint(" %wZ,", &(CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink)->BaseDllName));
        }
        else
        {
            DbgPrint(" <INITIAL>\n");
            break;
        }
    }

    // Set the depth for Node to the smallest unused index
    Node->LowestLink = Node->PreorderNumber = *PreorderNumberStorage;

    const PSINGLE_LIST_ENTRY EnterLink = &Node->CondenseLink;

    // Push to stack
    Node->CondenseLink.Next = Stack->Next;
    Stack->Next = EnterLink;

    // Note, that, for each Node, CondenseLink.Next points to
    // where stack was pointing before Node was pushed to the stack.
    // This simplifies the code below.

    // Consider the successors of Node
    const PSINGLE_LIST_ENTRY DependencyHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = DependencyHead ? DependencyHead->Next : NULL;
         ListEntry;
         ListEntry = (ListEntry != DependencyHead) ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);
        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;

        if (DependencyNode->State <= LdrModulesSnapped)
        {
            const ULONG32 LowestLink = DependencyNode->PreorderNumber;
            if (!LowestLink)
            {
                // Successor DependencyNode has not yet been visited; recurse on it
                LdrpCondenseGraphRecurse(DependencyNode, PreorderNumberStorage, Stack);
                Node->LowestLink = min(Node->LowestLink, DependencyNode->LowestLink);
            }
            else if (DependencyNode->CondenseLink.Next)
            {
                // Successor DependencyNode is on stack and hence in the current SCC
                // Note: The next line may look odd - but is correct.
                // It uses the PreorderNumber, not the LowestLink of DependencyNode;
                // that is deliberate and from the original paper
                Node->LowestLink = min(Node->LowestLink, LowestLink);
            }
            // If DependencyNode is not on stack, then (Node, DependencyNode) is a cross-edge in the DFS tree and must be ignored
        }

        if (DependencyNode->State == LdrModulesSnapError)
        {
            Node->State = LdrModulesSnapError;
        }
    }

    // Is Node a root of any strongly connected component?
    if (Node->LowestLink == Node->PreorderNumber)
    {
        // Start a new strongly connected component by doing pop
        // This will remove the Node (root of found SCC) from the stack
        PSINGLE_LIST_ENTRY NewLink = Stack->Next;
        if (NewLink)
            Stack->Next = NewLink->Next;

        if (NewLink != EnterLink)
        {
            SINGLE_LIST_ENTRY Children = {NULL};
            PSINGLE_LIST_ENTRY TheOneBeforeLast;
            PSINGLE_LIST_ENTRY CurrentLink;

            do
            {
                CurrentLink = Stack->Next;

                // Add NewLink to Children list
                NewLink->Next = Children.Next;
                Children.Next = NewLink;

                if (CurrentLink)
                    Stack->Next = CurrentLink->Next;

                TheOneBeforeLast = NewLink;
                NewLink = CurrentLink;
            }
            while (EnterLink != CurrentLink);

            // Verify that NewLink wasn't NULL at the end
            if (TheOneBeforeLast)
            {
                // Merge all Children into Node
                RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
                LdrpMergeNodes(Node, &Children);
                RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

                // Destroy all merged nodes
                for (PSINGLE_LIST_ENTRY Entry = Children.Next; Entry; Entry = Entry->Next)
                {
                    LdrpDestroyNode(CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink));
                }
            }
        }

        // Clean-up so that it wouldn't confuse the algorithm on the next LdrpCondenseGraph call.
        EnterLink->Next = NULL;

        // Hooray! Allow us to proceed to the next stage.
        if (Node->State == LdrModulesSnapped)
            Node->State = LdrModulesCondensed;
    }

    DPRINT1("CONDENSE OUT %wZ (%lu):", &(CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink)->BaseDllName), *PreorderNumberStorage);

    for (PSINGLE_LIST_ENTRY Entry = Stack->Next; Entry; Entry = Entry->Next)
    {
        if (Entry != INITIAL_CONDENSE_LINK)
        {
            const PLDR_DDAG_NODE Node = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);
            DbgPrint(" %wZ,", &(CONTAINING_RECORD(Node->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink)->BaseDllName));
        }
        else
        {
            DbgPrint(" <INITIAL>\n");
            break;
        }
    }
}

void
NTAPI
LdrpCondenseGraph(PLDR_DDAG_NODE Node)
{
    if (Node->State >= LdrModulesCondensed)
        return;

    ULONG32 PreorderNumberStorage = 0;
    SINGLE_LIST_ENTRY Stack = {INITIAL_CONDENSE_LINK};

    LdrpCondenseGraphRecurse(Node, &PreorderNumberStorage, &Stack);
}
