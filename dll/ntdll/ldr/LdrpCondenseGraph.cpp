#include <ldrp.h>

typedef void (NTAPI *LdrpMergeNodesProcessListCallback)(PLDR_DDAG_NODE Root, PLDRP_CSLIST_ENTRY DependencyEntry);

void
NTAPI
LdrpMergeNodesDependenciesCallback(PLDR_DDAG_NODE Root, PLDRP_CSLIST_ENTRY DependencyEntry)
{
    LDRP_ASSERT_DDAG_NODE(Root);
    LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

    DependencyEntry->DependenciesLink.Next = NULL;
}

void
NTAPI
LdrpMergeNodesIncomingDependenciesCallback(PLDR_DDAG_NODE Root, PLDRP_CSLIST_ENTRY DependencyEntry)
{
    LDRP_ASSERT_DDAG_NODE(Root);
    LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

    ASSERT(Root->LoadCount > 1);
    --Root->LoadCount;
    LdrpHeapFree(0, DependencyEntry);
}

void
NTAPI
LdrpMergeNodesProcessList(PLDRP_CSLIST List, PLDR_DDAG_NODE Root, LONG LinkOffset, LONG NodeOffset,
                          LdrpMergeNodesProcessListCallback Callback)
{
    LDRP_ASSERT_DDAG_NODE(Root);

    const PSINGLE_LIST_ENTRY InitialTail = List->Tail;

    ASSERT(InitialTail);

    PSINGLE_LIST_ENTRY PrevLink = InitialTail;

    // Note the difference in the loop's __iteration_expression__.
    // It uses the Next pointer of the PrevLink, not ListEntry.
    // It is done so that we can modify the list as we please.
    for (PSINGLE_LIST_ENTRY ListEntry = InitialTail->Next;
         ListEntry;
         ListEntry = ListEntry != InitialTail ? PrevLink->Next : NULL)
    {
        auto* DependencyEntry = static_cast<PLDRP_CSLIST_ENTRY>(PTR_SUB_OFFSET(ListEntry, LinkOffset));

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        ASSERT(PrevLink->Next == PTR_ADD_OFFSET(DependencyEntry, LinkOffset));

        const auto* DependencyNode = *static_cast<PLDR_DDAG_NODE*>(PTR_ADD_OFFSET(DependencyEntry, NodeOffset));

        LDRP_ASSERT_DDAG_NODE(DependencyNode);

        if (DependencyNode != Root)
        {
            // This is a normal dependency, not a cycle.
            // It's the base entry from which to search for the next dependency, save it as one.
            PrevLink = ListEntry;
            continue;
        }

        ASSERT(List->Tail);
        ASSERT(PrevLink->Next == ListEntry);

        // Cycle appeared after merging the modules into one.
        // e.g. Before merging it could have been like A->B->A. Now it's A->A.

        // First, drop the entry from the dependency list, so that the next iteration will check
        // the entry after this ("faulty") one.
        PrevLink->Next = ListEntry->Next;

        if (ListEntry == List->Tail)
        {
            // Last iteration of the loop

            // Were there any other dependency entries, except for this one?
            // If there were, set the list tail to the last normal dependency entry.
            // Otherwise, delete the whole list.

            List->Tail = PrevLink != List->Tail ? PrevLink : NULL;
        }

        Callback(Root, DependencyEntry);
    }
}

PSINGLE_LIST_ENTRY
NTAPI
LdrpMergeNodesSearchList(LDRP_CSLIST List, PLDR_DDAG_NODE DependencyNode, LONG LinkOffset, LONG NodeOffset)
{
    LDRP_ASSERT_DDAG_NODE(DependencyNode);

    const PSINGLE_LIST_ENTRY ListHead = List.Tail;
    PSINGLE_LIST_ENTRY ListEntry;

    for (ListEntry = ListHead ? ListHead->Next : NULL;
         ListEntry;
         ListEntry = ListEntry != ListHead ? ListEntry->Next : NULL)
    {
        const auto* RootDependencyEntry = static_cast<PLDRP_CSLIST_ENTRY>(PTR_SUB_OFFSET(ListEntry, LinkOffset));

        LDRP_ASSERT_CSLIST_ENTRY(RootDependencyEntry);

        const auto* RootDependencyNode = *static_cast<PLDR_DDAG_NODE*>(PTR_ADD_OFFSET(RootDependencyEntry, NodeOffset));

        LDRP_ASSERT_DDAG_NODE(RootDependencyNode);

        if (RootDependencyNode == DependencyNode)
            break;
    }

    return ListEntry;
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
    ASSERT(List);
    LDRP_ASSERT_DDAG_NODE(Root);
    ASSERT(DependencyListEntry);
    ASSERT(Callback);

    auto* Dependency = static_cast<PLDRP_CSLIST_ENTRY>(PTR_SUB_OFFSET(DependencyListEntry, LinkOffset));

    LDRP_ASSERT_CSLIST_ENTRY(Dependency);
    LDRP_ASSERT_DDAG_NODE(Dependency->DependencyNode);
    LDRP_ASSERT_DDAG_NODE(Dependency->ParentNode);

    auto* DependencyNode = *static_cast<PLDR_DDAG_NODE*>(PTR_ADD_OFFSET(Dependency, NodeOffset));

    LDRP_ASSERT_DDAG_NODE(DependencyNode);

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

    // Well. The list exists, and therefore, non-empty.

    // Find link: Root->DependencyNode.
    const PSINGLE_LIST_ENTRY ListEntry = LdrpMergeNodesSearchList(
        *List,
        DependencyNode,
        LinkOffset,
        NodeOffset
    );

    if (ListEntry)
    {
        LDRP_ASSERT_CSLIST_ENTRY((PLDRP_CSLIST_ENTRY)PTR_SUB_OFFSET(ListEntry, LinkOffset));

        // Found it. Preserve the ListEntry, since the found link is older,
        // than the link we would create in case we didn't find the DependencyNode.
        //
        // It also means that we must destroy Dependency (it is a duplicate Root->DependencyNode link).
        //
        // But first, we must drop the link in opposite direction.

        // Remove the Dependency from the list
        LdrpRemoveDependencyEntry(
            static_cast<PLDRP_CSLIST>(PTR_ADD_OFFSET(DependencyNode, OppositeListOffset)),
            static_cast<PSINGLE_LIST_ENTRY>(PTR_ADD_OFFSET(Dependency, OppositeLinkOffset))
        );

        // Decrement the node LoadCount (the number of incoming dependencies has dropped after merge)
        --Dependency->DependencyNode->LoadCount;

        // Destroy the link we no longer need
        LdrpHeapFree(0, Dependency);
    }
    else
    {
        // We didn't find the link in the List. Create one.
        PushEntryList(List->Tail, DependencyListEntry);
        List->Tail = DependencyListEntry;
    }
}

void
LdrpAssertChildrenList(IN PSINGLE_LIST_ENTRY Children)
{
    ASSERT(Children);

    for (PSINGLE_LIST_ENTRY Entry = Children->Next; Entry; Entry = Entry->Next)
    {
        const PLDR_DDAG_NODE EntryNode = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);

        LDRP_ASSERT_DDAG_NODE(EntryNode);
    }
}

void
NTAPI
LdrpMergeNodes(PLDR_DDAG_NODE Root, PSINGLE_LIST_ENTRY Children)
{
    LDRP_ASSERT_DDAG_NODE(Root);

    const PLDR_DATA_TABLE_ENTRY RootLdrEntry = LDRP_FIRST_MODULE(Root);

    LDRP_ASSERT_MODULE_ENTRY(RootLdrEntry);
    LDRP_ASSERT_NODE_ENTRY(Root, RootLdrEntry);

    LdrpAssertChildrenList(Children);

    DPRINT1("(" LDR_HEXPTR_FMT "[%wZ], " LDR_HEXPTR_FMT "): Merging a cycle.\n",
            Root, &RootLdrEntry->BaseDllName, Children);

    for (PSINGLE_LIST_ENTRY Entry = Children->Next; Entry; Entry = Entry->Next)
    {
        const PLDR_DDAG_NODE Node = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);

        LDRP_ASSERT_DDAG_NODE(Node);

        RtlpCheckListEntry(&Node->Modules);

        const PLIST_ENTRY NodeModuleLink = Node->Modules.Flink;

        RtlpCheckListEntry(NodeModuleLink);

        const PLDR_DATA_TABLE_ENTRY NodeEntry = CONTAINING_RECORD(NodeModuleLink, LDR_DATA_TABLE_ENTRY,
                                                                  NodeModuleLink);

        LDRP_ASSERT_MODULE_ENTRY(NodeEntry);
        LDRP_ASSERT_NODE_ENTRY(Node, NodeEntry);

        ASSERT(NodeEntry == LDRP_FIRST_MODULE(Node));

        RemoveEntryList(NodeModuleLink);

        DPRINT1("(" LDR_HEXPTR_FMT "[%wZ], " LDR_HEXPTR_FMT "): Adding cyclic module %wZ.\n",
                Root, &RootLdrEntry->BaseDllName, Children, &NodeEntry->BaseDllName);

        NodeEntry->DdagNode = Root;

        InsertTailList(&Root->Modules, NodeModuleLink);

        LDRP_ASSERT_NODE_ENTRY(Root, NodeEntry);

        Root->LoadCount += Node->LoadCount;
        Node->LoadCount = 0;

        ASSERT(Node->State <= LdrModulesSnapped);

        Node->State = LdrModulesMerged;

        const PSINGLE_LIST_ENTRY DependencyHead = Node->Dependencies.Tail;
        for (PSINGLE_LIST_ENTRY ListEntry = DependencyHead ? DependencyHead->Next : NULL;
             ListEntry;
             ListEntry = ListEntry != DependencyHead ? ListEntry->Next : NULL)
        {
            const PLDRP_CSLIST_ENTRY DependencyEntry =
                CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

            LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

            ASSERT(DependencyEntry->ParentNode == Node);

            DependencyEntry->ParentNode = Root;
        }

        const PSINGLE_LIST_ENTRY IncomingDependencyHead = Node->IncomingDependencies.Tail;
        for (PSINGLE_LIST_ENTRY ListEntry = IncomingDependencyHead ? IncomingDependencyHead->Next : NULL;
             ListEntry;
             ListEntry = ListEntry != IncomingDependencyHead ? ListEntry->Next : NULL)
        {
            const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY,
                                                                         IncomingDependenciesLink);

            LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

            ASSERT(DependencyEntry->DependencyNode == Node);

            DependencyEntry->DependencyNode = Root;
        }
    }

    LdrpAssertChildrenList(Children);

    // Process all Root->Dependencies (OUTGOING direction)

    LdrpMergeNodesProcessList(
        &Root->Dependencies,
        Root,
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependenciesLink),
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, DependencyNode),
        LdrpMergeNodesDependenciesCallback
    );

    LdrpAssertChildrenList(Children);

    for (PSINGLE_LIST_ENTRY ChildEntry = Children->Next; ChildEntry; ChildEntry = ChildEntry->Next)
    {
        const PLDR_DDAG_NODE Node = CONTAINING_RECORD(ChildEntry, LDR_DDAG_NODE, CondenseLink);

        LDRP_ASSERT_DDAG_NODE(Node);

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
                LdrpMergeNodesDependenciesCallback
            );
        }
    }

    LdrpAssertChildrenList(Children);

    // Now do (almost) the same thing with Root->IncomingDependencies (INCOMING direction)
    // No comments this time.

    LdrpMergeNodesProcessList(
        &Root->IncomingDependencies,
        Root,
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, IncomingDependenciesLink),
        FIELD_OFFSET(LDRP_CSLIST_ENTRY, ParentNode),
        LdrpMergeNodesIncomingDependenciesCallback
    );

    LdrpAssertChildrenList(Children);

    for (PSINGLE_LIST_ENTRY ChildEntry = Children->Next; ChildEntry; ChildEntry = ChildEntry->Next)
    {
        const PLDR_DDAG_NODE Node = CONTAINING_RECORD(ChildEntry, LDR_DDAG_NODE, CondenseLink);

        LDRP_ASSERT_DDAG_NODE(Node);

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
                LdrpMergeNodesIncomingDependenciesCallback
            );
        }

        ASSERT(!Node->LoadCount);
        ASSERT(!Node->IncomingDependencies.Tail);
        ASSERT(!Node->Dependencies.Tail);
        ASSERT(Node->State == LdrModulesMerged);
    }

    LdrpAssertChildrenList(Children);
}

// Non-NULL for distiction ON_STACK vs CROSS-EDGE.
const PSINGLE_LIST_ENTRY INITIAL_CONDENSE_LINK = reinterpret_cast<PSINGLE_LIST_ENTRY>(static_cast<ULONG_PTR>(1u));

void
NTAPI
LdrpCondenseGraphRecurse(const PLDR_DDAG_NODE Node, const PULONG32 PreorderNumberStorage, const PSINGLE_LIST_ENTRY VisitStack)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    ASSERT(!Node->CondenseLink.Next);
    ASSERT(!Node->PreorderNumber);
    ASSERT(!Node->LowestLink);

    // Tarjan's strongly connected components algorithm

    *PreorderNumberStorage += 1;

    ASSERT(VisitStack->Next);

    {
        PLDR_DATA_TABLE_ENTRY ModuleEntry = LDRP_FIRST_MODULE(Node);

        LDRP_ASSERT_MODULE_ENTRY(ModuleEntry);
        LDRP_ASSERT_NODE_ENTRY(Node, ModuleEntry);

        DPRINT1("CONDENSE IN %wZ (%lu):", &ModuleEntry->BaseDllName, *PreorderNumberStorage);

        for (PSINGLE_LIST_ENTRY Entry = VisitStack->Next; Entry; Entry = Entry->Next)
        {
            if (Entry != INITIAL_CONDENSE_LINK)
            {
                const PLDR_DDAG_NODE Node = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);

                LDRP_ASSERT_DDAG_NODE(Node);

                ModuleEntry = LDRP_FIRST_MODULE(Node);

                LDRP_ASSERT_MODULE_ENTRY(ModuleEntry);
                LDRP_ASSERT_NODE_ENTRY(Node, ModuleEntry);

                DbgPrint(" %wZ,", &ModuleEntry->BaseDllName);
            }
            else
            {
                DbgPrint(" <INITIAL>\n");
                break;
            }
        }
    }

    // Set the depth for Node to the smallest unused index
    Node->LowestLink = Node->PreorderNumber = *PreorderNumberStorage;

    const PSINGLE_LIST_ENTRY EnterLink = &Node->CondenseLink;

    // Push to stack
    PushEntryList(VisitStack, EnterLink);

    // Note, that, for each Node, CondenseLink.Next points to
    // where stack was pointing before Node was pushed to the stack.
    // This simplifies the code below.

    // Consider the successors of Node
    const PSINGLE_LIST_ENTRY InitialDependencyHead = Node->Dependencies.Tail;
    for (PSINGLE_LIST_ENTRY ListEntry = InitialDependencyHead ? InitialDependencyHead->Next : NULL;
         ListEntry;
         ListEntry = ListEntry != Node->Dependencies.Tail ? ListEntry->Next : NULL)
    {
        const PLDRP_CSLIST_ENTRY DependencyEntry = CONTAINING_RECORD(ListEntry, LDRP_CSLIST_ENTRY, DependenciesLink);

        LDRP_ASSERT_CSLIST_ENTRY(DependencyEntry);

        const PLDR_DDAG_NODE DependencyNode = DependencyEntry->DependencyNode;

        LDRP_ASSERT_DDAG_NODE(DependencyNode);

        ASSERT(DependencyEntry->ParentNode == Node);

        if (DependencyNode->State <= LdrModulesSnapped)
        {
            const ULONG32 LowestLink = DependencyNode->PreorderNumber;
            if (!LowestLink)
            {
                // Successor DependencyNode has not yet been visited; recurse on it
                LdrpCondenseGraphRecurse(DependencyNode, PreorderNumberStorage, VisitStack);
                Node->LowestLink = min(Node->LowestLink, DependencyNode->LowestLink);
            }
            else if (DependencyNode->CondenseLink.Next)
            {
                // Successor DependencyNode is on stack and hence in the current SCC
                // Note: The next line may look odd - but is correct.
                // It uses the PreorderNumber [see definition of LowestLink variable], not the LowestLink of DependencyNode;
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

    ASSERT(Node->LowestLink <= Node->PreorderNumber);

    // Is Node a root of any strongly connected component?
    if (Node->LowestLink == Node->PreorderNumber)
    {
        // Start a new strongly connected component by doing pop
        // This will remove the Node (root of found SCC) from the stack
        PSINGLE_LIST_ENTRY NewLink = PopEntryList(VisitStack);

        if (NewLink != EnterLink)
        {
            SINGLE_LIST_ENTRY Children = {NULL};
            PSINGLE_LIST_ENTRY TheOneBeforeLast;
            PSINGLE_LIST_ENTRY CurrentLink;

            do
            {
                // Add NewLink to Children list
                PushEntryList(&Children, NewLink);

                CurrentLink = VisitStack->Next;

                if (CurrentLink)
                    VisitStack->Next = CurrentLink->Next;

                TheOneBeforeLast = NewLink;
                NewLink = CurrentLink;
            }
            while (EnterLink != CurrentLink);

            // Verify that NewLink wasn't NULL at the end
            if (TheOneBeforeLast)
            {
                LdrpAssertChildrenList(&Children);

                // Merge all Children into Node
                RtlAcquireSRWLockExclusive(&LdrpModuleDatatableLock);
                LdrpMergeNodes(Node, &Children);
                RtlReleaseSRWLockExclusive(&LdrpModuleDatatableLock);

                // Destroy all merged nodes
                for (PSINGLE_LIST_ENTRY Entry = Children.Next; Entry;)
                {
                    const PLDR_DDAG_NODE EntryNode = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);

                    LDRP_ASSERT_DDAG_NODE(EntryNode);

                    Entry = Entry->Next;

                    LdrpDestroyNode(EntryNode);
                }
            }
        }

        // Clean-up so that it wouldn't confuse the algorithm on the next LdrpCondenseGraph call.
        EnterLink->Next = NULL;

        // Hooray! Allow us to proceed to the next stage.
        if (Node->State == LdrModulesSnapped)
            Node->State = LdrModulesCondensed;
    }

    {
        PLDR_DATA_TABLE_ENTRY ModuleEntry = LDRP_FIRST_MODULE(Node);

        LDRP_ASSERT_MODULE_ENTRY(ModuleEntry);
        LDRP_ASSERT_NODE_ENTRY(Node, ModuleEntry);

        DPRINT1("CONDENSE OUT %wZ (%lu):", &ModuleEntry->BaseDllName, *PreorderNumberStorage);
        for (PSINGLE_LIST_ENTRY Entry = VisitStack->Next; Entry; Entry = Entry->Next)
        {
            if (Entry != INITIAL_CONDENSE_LINK)
            {
                const PLDR_DDAG_NODE Node = CONTAINING_RECORD(Entry, LDR_DDAG_NODE, CondenseLink);

                LDRP_ASSERT_DDAG_NODE(Node);

                ModuleEntry = LDRP_FIRST_MODULE(Node);

                LDRP_ASSERT_MODULE_ENTRY(ModuleEntry);
                LDRP_ASSERT_NODE_ENTRY(Node, ModuleEntry);

                DbgPrint(" %wZ,", &ModuleEntry->BaseDllName);
            }
            else
            {
                DbgPrint(" <INITIAL>\n");
                break;
            }
        }
    }
}

void
NTAPI
LdrpCondenseGraph(PLDR_DDAG_NODE Node)
{
    LDRP_ASSERT_DDAG_NODE(Node);

    if (Node->State >= LdrModulesCondensed)
        return;

    ULONG32 PreorderNumberStorage = 0;
    SINGLE_LIST_ENTRY VisitStack = {INITIAL_CONDENSE_LINK};

    LdrpCondenseGraphRecurse(Node, &PreorderNumberStorage, &VisitStack);

    ASSERT(VisitStack.Next == INITIAL_CONDENSE_LINK);
}
