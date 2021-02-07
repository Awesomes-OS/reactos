/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/thrdschd.c
 * PURPOSE:         Kernel Thread Scheduler (Affinity, Priority, Scheduling)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#if 0
KNODE ExNode0;
PROCESSOR_GROUP_BLOCK KiGroupBlock[MAX_PROC_GROUPS];
BOOLEAN KeForceGroupAwareness;
ULONG32 KiProcessorIndexToNumberMappingTable[MAX_PROC_GROUPS * MAXIMUM_PROC_PER_GROUP];
ULONG32 KiProcessorNumberToIndexMappingTable[MAX_PROC_GROUPS * MAXIMUM_PROC_PER_GROUP];
UINT16 KiActiveGroups = 1;
UINT16 KiAssignedGroupCount;
UINT16 KiMaximumGroups;
ULONG32 KiMaximumGroupSize = MAXIMUM_PROC_PER_GROUP;
ULONG_PTR KeNumberProcessors_0;
CCHAR KeNumberProcessorsGroup0 = 0;
PKNODE KeNodeBlock[MAXIMUM_PROC_PER_GROUP];
KNODE KiNodeInit[MAXIMUM_PROC_PER_GROUP];
UINT16 KeNumberNodes = 1;
UINT16 KiProcessNodeSeed = 0;

/* FUNCTIONS *****************************************************************/

const KAFFINITY m1 = (~(KAFFINITY)0) / 3; // Binary: 0101...
const KAFFINITY m2 = (~(KAFFINITY)0) / 5; // Binary: 00110011..
const KAFFINITY m4 = (~(KAFFINITY)0) / 17; // Binary: 4 zeros, 4 ones ...
const KAFFINITY h01 = (~(KAFFINITY)0) / 255; // The sum of 256 to the power of 0,1,2,3...
const unsigned TEST_BITS = sizeof(KAFFINITY) * CHAR_BIT;

///
/// Hamming weight
///
FORCEINLINE
ULONG
popcount(KAFFINITY x)
{
    x -= (x >> 1) & m1; // put count of each 2 bits into those 2 bits
    x = (x & m2) + ((x >> 2) & m2); // put count of each 4 bits into those 4 bits
    x = (x + (x >> 4)) & m4; // put count of each 8 bits into those 8 bits
    return (x * h01) >> (TEST_BITS - 8); // returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ...
}

ULONG
NTAPI
KeQueryActiveProcessorCount(OUT PKAFFINITY ActiveProcessors OPTIONAL)
{
    if (ActiveProcessors)
        *ActiveProcessors = KeActiveProcessors.Bitmap[0];
    return KeQueryActiveProcessorCountEx(0);
}

ULONG
NTAPI
KeQueryActiveProcessorCountEx(IN USHORT GroupNumber)
{
    if (GroupNumber == ALL_PROCESSOR_GROUPS)
        return KeNumberProcessors_0;
    if (GroupNumber >= KiActiveGroups)
        return 0;
    return popcount(KeActiveProcessors.Bitmap[GroupNumber]);
}

FORCEINLINE
BOOLEAN
NTAPI
KiTestAndSetAffinity(KAFFINITY* const Affinity, const LONG BitIndex)
{
#if defined(_WIN64)
    return BitTestAndSet64((LONG_PTR*)Affinity, BitIndex);
#else
    return BitTestAndSet((LONG_PTR*)Affinity, BitIndex);
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
KiTestAndResetAffinity(KAFFINITY* const Affinity, const LONG BitIndex)
{
#if defined(_WIN64)
    return BitTestAndReset64((LONG_PTR*)Affinity, BitIndex);
#else
    return BitTestAndReset((LONG_PTR*)Affinity, BitIndex);
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
KiBitScanForwardAffinity(ULONG* const Index, const KAFFINITY Affinity)
{
#if defined(_WIN64)
    return BitScanForward64(Index, Affinity);
#else
    return BitScanForward(Index, Affinity);
#endif
}

FORCEINLINE
BOOLEAN
NTAPI
KiBitScanReverseAffinity(ULONG* const Index, const KAFFINITY Affinity)
{
#if defined(_WIN64)
    return BitScanReverse64(Index, Affinity);
#else
    return BitScanReverse(Index, Affinity);
#endif
}

PKPRCB
NTAPI
KiConsumeNextProcessor(GROUP_AFFINITY* const Affinity)
{
    const KAFFINITY Mask = Affinity->Mask;
    if (!Mask)
        return NULL;

    ULONG BitIndex = 0;
    ASSERT(KiBitScanForwardAffinity(&BitIndex, Mask));

    PROCESSOR_NUMBER ProcNumber = { 0 };
    ProcNumber.Group = Affinity->Group;
    ProcNumber.Number = BitIndex;

    const ULONG Index = KeGetProcessorIndexFromNumber(&ProcNumber);
    if (Index == INVALID_PROCESSOR_INDEX)
        return NULL;

    ASSERT(KiTestAndResetAffinity(&Affinity->Mask, BitIndex));

    return KiProcessorBlock[Index];
}

ULONG
FASTCALL
KeAddProcessorAffinityEx(IN OUT PKAFFINITY_EX TargetAffinity, IN ULONG32 ProcessorIndex)
{
    PROCESSOR_NUMBER Number;

    ASSERT(NT_SUCCESS(KeGetProcessorNumberFromIndex(ProcessorIndex, &Number)));

    const UINT16 ProcessorNumber = Number.Number;
    const UINT16 Group = Number.Group;

    if (TargetAffinity->Count <= Group)
        TargetAffinity->Count = Group + 1;

    KiTestAndSetAffinity(&TargetAffinity->Bitmap[Group], ProcessorNumber);

    return ProcessorNumber;
}

/*
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nf-wdm-kegetprocessornumberfromindex
 * @implemented
 */
NTSTATUS
NTAPI
KeGetProcessorNumberFromIndex(
    IN ULONG ProcIndex,
    OUT PPROCESSOR_NUMBER ProcNumber)
{
    if (ProcIndex == 0)
    {
        ProcNumber->Group = 0;
        ProcNumber->Number = 0;
        ProcNumber->Reserved = 0;
        return STATUS_SUCCESS;
    }

    if (ProcIndex >= MAX_PROC_GROUPS * MAXIMUM_PROC_PER_GROUP)
        return STATUS_INVALID_PARAMETER;

    ProcIndex = KiProcessorIndexToNumberMappingTable[ProcIndex];
    if (!ProcIndex)
        return STATUS_INVALID_PARAMETER;

    ProcNumber->Group = PROCESSOR_INDEX_GROUP_MASK(ProcIndex);
    ProcNumber->Number = PROCESSOR_INDEX_NUMBER_MASK(ProcIndex);
    ProcNumber->Reserved = 0;

    return STATUS_SUCCESS;
}

/*
 * https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/nf-wdm-kegetprocessorindexfromnumber
 * @implemented
 */
ULONG
NTAPI
KeGetProcessorIndexFromNumber(IN PPROCESSOR_NUMBER ProcNumber)
{
    if (ProcNumber->Reserved)
        return INVALID_PROCESSOR_INDEX;

    if (!ProcNumber->Group && !ProcNumber->Number)
        return 0;

    if (ProcNumber->Number >= MAXIMUM_PROC_PER_GROUP)
        return INVALID_PROCESSOR_INDEX;

    if (ProcNumber->Group >= KiMaximumGroups)
        return INVALID_PROCESSOR_INDEX;

    // todo (andrew.boyarshin)
    if (ProcNumber->Number >= MAXIMUM_PROC_PER_GROUP)
        return INVALID_PROCESSOR_INDEX;

    ULONG32 Index = PROCESSOR_NUMBER_TO_INDEX_LOOKUP(ProcNumber->Group, ProcNumber->Number);
    Index = KiProcessorNumberToIndexMappingTable[Index];

    return Index ? Index : INVALID_PROCESSOR_INDEX;
}

void
FASTCALL
KiAddProcessorToGroupDatabase(IN PKPRCB Prcb, IN BOOLEAN DynamicallyAdded)
{
    UNREFERENCED_PARAMETER(DynamicallyAdded);

    const PKNODE Node = Prcb->ParentNode;

    Prcb->Group = Node->Affinity.Group;

    KAFFINITY* GroupSetPtr = &KiGroupBlock[Prcb->Group].ProcessorSet;
    const KAFFINITY GroupSet = *GroupSetPtr + 1;

    Prcb->GroupSetMember = GroupSet;

    ULONG GroupIndex = 0;
    KiBitScanReverseAffinity(&GroupIndex, GroupSet);

    Prcb->GroupIndex = GroupIndex;

    // 0b00000 -> 0b00001 -> 0b00011 -> 0b00111 -> 0b01111 -> ...
    *GroupSetPtr |= GroupSet;

    if (KeForceGroupAwareness)
    {
        Prcb->LegacyNumber = (Prcb->Number > 0xFF) ? 0xFF : Prcb->Number;
    }
    else if (Prcb->Group)
    {
        // Keep LegacyNumber less than the number of processors in group 0
        // so that legacy applications using group-unaware routines
        // will not fail assertions.
        Prcb->LegacyNumber = GroupIndex % KeQueryActiveProcessorCountEx(0);
    }
    else
    {
        Prcb->LegacyNumber = GroupIndex;
    }

    const ULONG32 Lookup = PROCESSOR_NUMBER_TO_INDEX_LOOKUP(Prcb->Group, GroupIndex);

    ASSERT(Prcb->Number < ARRAYSIZE(KiProcessorIndexToNumberMappingTable));
    ASSERT(Lookup < ARRAYSIZE(KiProcessorNumberToIndexMappingTable));

    KiProcessorIndexToNumberMappingTable[Prcb->Number] = Lookup;
    KiProcessorNumberToIndexMappingTable[Lookup] = Prcb->Number;
}

void
FASTCALL
KiCommitNodeAssignment(PKNODE ParentNode)
{
    const USHORT ParentGroup = ParentNode->Affinity.Group;
    const UINT16 GroupNumber = KiAssignedGroupCount;

    KiVBoxPrint("KiCommitNodeAssignment ");
    KiVBoxPrintInteger(KeNumberNodes);
    KiVBoxPrint("\n");

    for (UINT16 i = 0; i < KeNumberNodes; ++i)
    {
        const PKNODE Node = KeNodeBlock[i];

        if (!Node->Flags.GroupAssigned)
            continue;
        if (Node->Affinity.Group != ParentGroup)
            continue;

        Node->Affinity.Group = GroupNumber;
        Node->Flags.GroupCommitted = TRUE;
    }

    for (UINT16 i = 0; i < KeNumberNodes; ++i)
    {
        const PKNODE Node = KeNodeBlock[i];

        if (Node->Flags.GroupCommitted)
            continue;
        if (!Node->Flags.GroupAssigned)
            continue;
        if (Node->Affinity.Group != GroupNumber)
            continue;

        Node->Affinity.Group = ParentGroup;
    }

    KiAssignedGroupCount = GroupNumber + 1;

    KiVBoxPrint("KiCommitNodeAssignment EXIT ");
    KiVBoxPrintInteger(KiAssignedGroupCount);
    KiVBoxPrint("\n");
}

UINT16
NTAPI
KiGetCurrentGroupCount()
{
    return KiAssignedGroupCount;
}

void
NTAPI
KiReconfigureNodeSchedulingInformation(IN PKNODE Node, IN PKPRCB Prcb)
{
    Node->StrideMask |= Prcb->GroupSetMember;
}

void
FASTCALL
KiConfigureInitialNodes(IN PKPRCB Prcb)
{
    ExNode0.Flags.GroupAssigned = TRUE;
    ExNode0.StrideMask |= 1u;
    ExNode0.MaximumProcessors = KiMaximumGroupSize;
    KeNodeBlock[0] = &ExNode0;
    KiCommitNodeAssignment(&ExNode0);

    Prcb->ParentNode = &ExNode0;
    KiAddProcessorToGroupDatabase(Prcb, FALSE);

    for (UINT16 i = 1; i < MAXIMUM_PROC_PER_GROUP; ++i)
    {
        KeNodeBlock[i] = &KiNodeInit[i];
        KeNodeBlock[i]->NodeNumber = i;
    }
}

void
FASTCALL
KiConfigureProcessorBlock(IN PKPRCB Prcb)
{
    PROCESSOR_NUMBER Number;

    ASSERT(NT_SUCCESS(KeGetProcessorNumberFromIndex(Prcb->Number, &Number)));

    Prcb->ParentNode->Affinity.Mask |= AFFINITY_MASK(Number.Number);

    const KAFFINITY NodeMask = Prcb->ParentNode->Affinity.Mask;

    ULONG Lowest = 0, Highest = 0;
    KiBitScanForwardAffinity(&Lowest, NodeMask);
    KiBitScanReverseAffinity(&Highest, NodeMask);

    PROCESSOR_NUMBER LowestNumber = { 0 }, HighestNumber = {0};
    LowestNumber.Group = HighestNumber.Group = Number.Group;
    LowestNumber.Number = Lowest;
    HighestNumber.Number = Highest;

    Prcb->ParentNode->Lowest = KeGetProcessorIndexFromNumber(&LowestNumber);
    Prcb->ParentNode->Highest = KeGetProcessorIndexFromNumber(&HighestNumber);

    ASSERT(Prcb->ParentNode->Lowest != INVALID_PROCESSOR_INDEX);
    ASSERT(Prcb->ParentNode->Highest != INVALID_PROCESSOR_INDEX);

    Prcb->ParentNode->ProcessSeed = Prcb->ParentNode->ThreadSeed = Lowest;

    // Does the ParentNode have only 1 processor? Check if the mask has only one bit set.
    //
    //           1         2         3         4         5         6         7         8         9
    // 0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
    // FFFTFTTTFTTTTTTTFTTTTTTTTTTTTTTTFTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTFTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
    if ((NodeMask - 1) & NodeMask)
        return;

    // Only one processor, initialize SiblingMask

    const ULONG32 NodeNumberMask = AFFINITY_MASK(Prcb->ParentNode->NodeNumber);

    PKNODE PreviousNode = NULL;

    for (UINT16 i = 0; i < KeNumberNodes; ++i)
    {
        const PKNODE Node = KeNodeBlock[i];

        if (Node->Affinity.Group != Number.Group)
            continue;

        const PKNODE SiblingNode = PreviousNode ? PreviousNode : Node;

        Node->SiblingMask = SiblingNode->SiblingMask | NodeNumberMask;

        PreviousNode = Node;
    }
}

void
FASTCALL
KeSetGroupMaskProcess(PKPROCESS Process, ULONG GroupMask)
{
    Process->ActiveGroupsMask = GroupMask;
}

NTSTATUS
NTAPI
KeFirstGroupAffinityEx(IN OUT PGROUP_AFFINITY ThreadAffinity, IN PKAFFINITY_EX ProcessAffinity)
{
    UINT16 Group = 0;

    if (!ProcessAffinity->Count)
        return STATUS_NOT_FOUND;

    while (!ProcessAffinity->Bitmap[Group])
    {
        if (++Group >= ProcessAffinity->Count)
            return STATUS_NOT_FOUND;
    }

    RtlZeroMemory(ThreadAffinity, sizeof(*ThreadAffinity));
    ThreadAffinity->Group = Group;
    ThreadAffinity->Mask = ProcessAffinity->Bitmap[Group];

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
KiPrcbInGroupAffinity(IN const KPRCB* const Prcb, IN const GROUP_AFFINITY* const Affinity)
{
    return Prcb->Group == Affinity->Group && Prcb->GroupSetMember & Affinity->Mask;
}

UINT16
NTAPI
KiSelectIdealProcessor(IN PKNODE Node, IN UINT16 Seed, IN UINT64 MatchMask)
{
    if (Node->Lowest == Node->Highest)
        return Node->Lowest;

    UINT16 IdealIndex;
#if 0
    // Pre 19H1
    UINT64 Stride = Node->Stride;

    UINT16 IdealIndex = Seed;
    do
    {
        IdealIndex += Stride;
        if (IdealIndex > Node->Highest)
            IdealIndex = Node->Lowest + (IdealIndex - Node->Lowest + 1) % Stride;
    } while (!BitTest64((const __int64*)&MatchMask, IdealIndex));
#elif 0
    // 19H1
    // Use StrideMask instead of Stride, complex bit logic
#else
    // ReactOS :)
    // This will certainly degrade performance on real NUMA systems
    // But who cares in alpha state?
    IdealIndex = Node->Highest;
#endif

    return IdealIndex;
}

UINT16
NTAPI
KeSelectIdealProcessor(
    IN PKNODE Node,
    IN PGROUP_AFFINITY ThreadAffinity,
    IN PUINT16 SeedPointer OPTIONAL
)
{
    if (!SeedPointer)
        SeedPointer = &Node->ThreadSeed;

    const UINT64 MatchMask = ThreadAffinity->Mask & Node->Affinity.Mask;

    const UINT16 Index = KiSelectIdealProcessor(Node, *SeedPointer, MatchMask);
    *SeedPointer = Index;

    return Index;
}

PKNODE
FASTCALL
KeSelectNodeForAffinity(PGROUP_AFFINITY Affinity)
{
    if (KeNumberNodes == 1)
        return KeNodeBlock[0];

    UINT16 ProcessNodeSeed = KiProcessNodeSeed++;
    if (KiProcessNodeSeed >= KeNumberNodes)
        KiProcessNodeSeed = 0;

    for (UINT16 i = 0; i < KeNumberNodes; ++i, ++ProcessNodeSeed)
    {
        if (ProcessNodeSeed >= KeNumberNodes)
            ProcessNodeSeed = 0;

        const PKNODE Node = KeNodeBlock[ProcessNodeSeed];

        if (Node->Flags.ProcessorOnly)
            continue;
        if (Node->Affinity.Group != Affinity->Group)
            continue;
        if (!(Node->Affinity.Mask & Affinity->Mask))
            continue;

        return Node;
    }

    for (UINT16 i = 0; i < KeNumberNodes; ++i, ++ProcessNodeSeed)
    {
        if (ProcessNodeSeed >= KeNumberNodes)
            ProcessNodeSeed = 0;

        const PKNODE Node = KeNodeBlock[ProcessNodeSeed];

        if (Node->Affinity.Group != Affinity->Group)
            continue;
        if (!(Node->Affinity.Mask & Affinity->Mask))
            continue;

        return Node;
    }

    return NULL;
}

ULONG32
FASTCALL
KiSelectIdealProcessorForProcess(PKNODE Node, PGROUP_AFFINITY Affinity)
{
    const KAFFINITY MatchMask = Affinity->Mask & Node->Affinity.Mask;
    ULONG Number = 0;

    KiBitScanForwardAffinity(&Number, MatchMask);

    return KiProcessorNumberToIndexMappingTable[PROCESSOR_NUMBER_TO_INDEX_LOOKUP(Affinity->Group, Number)];
}

ULONG32
FASTCALL
KiSetIdealNodeProcessByGroup(PKPROCESS Process, PKNODE Node, UINT16 Group)
{
    GROUP_AFFINITY GroupAffinity = {0};

    GroupAffinity.Group = Group;
    GroupAffinity.Mask = Process->Affinity.Bitmap[Group];
    if (!Node)
        Node = KeSelectNodeForAffinity(&GroupAffinity);

    GroupAffinity.Mask = Process->Affinity.Bitmap[Group] & Node->Affinity.Mask;

    const ULONG32 Index = KiSelectIdealProcessorForProcess(Node, &GroupAffinity);

    Process->IdealNode[Group] = Node->NodeNumber;
    Process->IdealProcessor[Group] = Index;
    Process->ThreadSeed[Group] = Index;

    return Index;
}

void
FASTCALL
KiExtendProcessAffinity(PKPROCESS Process, UINT16 Group)
{
    const KAFFINITY ActiveProcessors = KeActiveProcessors.Bitmap[Group];

    if (Process->Affinity.Bitmap[Group])
    {
        Process->Affinity.Bitmap[Group] |= ActiveProcessors;
    }
    else
    {
        if (Process->Affinity.Count <= Group)
            Process->Affinity.Count = Group + 1;

        Process->Affinity.Bitmap[Group] |= ActiveProcessors;

        ASSERT(KiSetIdealNodeProcessByGroup(Process, 0i64, Group) != INVALID_PROCESSOR_INDEX);

        Process->ActiveGroupsMask |= AFFINITY_MASK(Group);
    }
}

BOOLEAN
FASTCALL
KiTestNodeAffinity(UINT16 Group, KAFFINITY Mask)
{
    KiVBoxPrint("KiTestNodeAffinity 1 ");
    KiVBoxPrintInteger(Group);
    KiVBoxPrint(" ");
    KiVBoxPrintInteger(Mask);
    KiVBoxPrint(" ");
    KiVBoxPrintInteger(KiActiveGroups);
    KiVBoxPrint(" ");
    KiVBoxPrintInteger(KeActiveProcessors.Count);
    KiVBoxPrint(" ");
    KiVBoxPrintInteger(KeActiveProcessors.Bitmap[0]);
    KiVBoxPrint(" ");
    KiVBoxPrintInteger(KeNumberNodes);
    KiVBoxPrint("\n");

    ASSERT(Mask);
    ASSERT(Group < KiActiveGroups);

    if (Mask == KeActiveProcessors.Bitmap[Group])
        return TRUE;

    if (KeNumberNodes <= 1)
        return FALSE;

    while (TRUE)
    {
        ULONG Number;

        KiBitScanReverseAffinity(&Number, Mask);

        const ULONG Lookup = PROCESSOR_NUMBER_TO_INDEX_LOOKUP(Group, Number);
        const ULONG32 Index = KiProcessorNumberToIndexMappingTable[Lookup];

        ASSERT(KiProcessorIndexToNumberMappingTable[Index] == Lookup);

        const KAFFINITY PrcbMask = KiProcessorBlock[Index]->ParentNode->Affinity.Mask;

        if ((Mask & PrcbMask) != PrcbMask)
            return FALSE;

        Mask &= ~PrcbMask;

        if (!Mask)
            return TRUE;
    }
}

void
FASTCALL
KiUpdateNodeAffinitizedFlag(PKTHREAD Thread)
{
    const BOOLEAN Affinitized = KiTestNodeAffinity(Thread->Affinity.Group, Thread->Affinity.Mask);

    if (!Affinitized)
        // todo: Interlocked
        Thread->Header.AffinitySet = TRUE;
}


void
NTAPI
KeQueryNodeActiveAffinity(
    USHORT NodeNumber,
    PGROUP_AFFINITY Affinity,
    PUSHORT Count
)
{
    if (Affinity)
        Affinity->Mask = 0u;
    if (Count)
        *Count = 0;

    if (NodeNumber >= KeNumberNodes)
        return;

    KeMemoryBarrier();
    const PKNODE Node = KeNodeBlock[NodeNumber];

    if (Affinity)
    {
        Affinity->Group = Node->Affinity.Group;
        Affinity->Mask = Node->Affinity.Mask;
    }

    if (Count)
    {
        *Count = popcount(Node->Affinity.Mask);
    }
}

ULONG32
NTAPI
KeFindFirstSetLeftGroupAffinity(PGROUP_AFFINITY Affinity)
{
    KAFFINITY Mask = Affinity->Mask;
    if (!Mask)
        return INVALID_PROCESSOR_INDEX;

    ASSERT(KiBitScanReverseAffinity(&Mask, Mask));

    PROCESSOR_NUMBER ProcNumber = { 0 };
    ProcNumber.Group = Affinity->Group;
    ProcNumber.Number = Mask;

    return KeGetProcessorIndexFromNumber(&ProcNumber);
}

ULONG32
NTAPI
KeFindFirstSetRightGroupAffinity(PGROUP_AFFINITY Affinity)
{
    KAFFINITY Mask = Affinity->Mask;
    if (!Mask)
        return INVALID_PROCESSOR_INDEX;

    ASSERT(KiBitScanForwardAffinity(&Mask, Mask));

    PROCESSOR_NUMBER ProcNumber = { 0 };
    ProcNumber.Group = Affinity->Group;
    ProcNumber.Number = Mask;

    return KeGetProcessorIndexFromNumber(&ProcNumber);
}

ULONG32
NTAPI
KeFindFirstSetLeftAffinityEx(PKAFFINITY_EX Affinity)
{
    for (INT32 Group = Affinity->Count - 1; Group >= 0; --Group)
    {
        const KAFFINITY Mask = Affinity->Bitmap[Group];
        if (!Mask)
            continue;

        ULONG Index = 0;

        ASSERT(KiBitScanReverseAffinity(&Index, Mask));

        PROCESSOR_NUMBER ProcNumber = { 0 };
        ProcNumber.Group = Group;
        ProcNumber.Number = Index;

        return KeGetProcessorIndexFromNumber(&ProcNumber);
    }

    return INVALID_PROCESSOR_INDEX;
}

ULONG32
NTAPI
KeFindFirstSetRightAffinityEx(PKAFFINITY_EX Affinity)
{
    for (UINT16 Group = 0; Group < Affinity->Count; ++Group)
    {
        const KAFFINITY Mask = Affinity->Bitmap[Group];
        if (!Mask)
            continue;

        ULONG Index = 0;

        ASSERT(KiBitScanForwardAffinity(&Index, Mask));

        PROCESSOR_NUMBER ProcNumber = { 0 };
        ProcNumber.Group = Group;
        ProcNumber.Number = Index;

        return KeGetProcessorIndexFromNumber(&ProcNumber);
    }

    return INVALID_PROCESSOR_INDEX;
}

ULONG
NTAPI
KeCountSetBitsGroupAffinity(PGROUP_AFFINITY Affinity)
{
    return popcount(Affinity->Mask);
}

KAFFINITY
NTAPI
KeQueryGroupAffinityEx(IN PKAFFINITY_EX Affinity, IN USHORT GroupNumber)
{
    return GroupNumber >= Affinity->Count ? 0 : Affinity->Bitmap[GroupNumber];
}

KAFFINITY
NTAPI
KeQueryGroupAffinity(IN USHORT GroupNumber)
{
    return KeQueryGroupAffinityEx(&KeActiveProcessors, GroupNumber);
}

USHORT
NTAPI
KeQueryActiveGroupCount(VOID)
{
    return KiActiveGroups;
}

BOOLEAN
NTAPI
KiVerifyReservedFieldGroupAffinity(const GROUP_AFFINITY* const Affinity)
{
    return !(Affinity->Reserved[0] || Affinity->Reserved[1] || Affinity->Reserved[2]);
}

BOOLEAN
NTAPI
KeVerifyGroupAffinity(const GROUP_AFFINITY* const Affinity, const BOOLEAN AllowEmptyMask)
{
    const USHORT Group = Affinity->Group;

    if (Group >= KeQueryActiveGroupCount())
        return FALSE;

    if (!AllowEmptyMask && !Affinity->Mask)
        return FALSE;

    if (!KiVerifyReservedFieldGroupAffinity(Affinity))
        return FALSE;

    return (Affinity->Mask & KeActiveProcessors.Bitmap[Group]) == Affinity->Mask;
}

BOOLEAN
NTAPI
KeAndAffinityEx(const PKAFFINITY_EX Affinity1, const PKAFFINITY_EX Affinity2, const PKAFFINITY_EX AffinityResult)
{
    KAFFINITY_EX AffinityBuffer = {0};
    const UINT16 CommonCount = min(Affinity1->Count, Affinity2->Count);
    KAFFINITY_EX *Affinity = AffinityResult ? AffinityResult : &AffinityBuffer;
    BOOLEAN NonEmpty = FALSE;

    Affinity->Count = CommonCount;
    for (UINT16 Group = 0; Group < CommonCount; ++Group)
    {
        Affinity->Bitmap[Group] = Affinity1->Bitmap[Group] & Affinity2->Bitmap[Group];
        if (Affinity->Bitmap[Group])
            NonEmpty = TRUE;
    }

    if (Affinity != &AffinityBuffer)
    {
        Affinity->Reserved = 0;
        Affinity->Size = MAX_PROC_GROUPS;
        for (UINT16 Group = CommonCount; Group < Affinity->Size; ++Group)
        {
            Affinity->Bitmap[Group] = 0;
        }
    }
    return NonEmpty;
}

BOOLEAN
NTAPI
KeIsEqualAffinityEx(const PKAFFINITY_EX Affinity1, const PKAFFINITY_EX Affinity2)
{
    const UINT16 CommonCount = min(Affinity1->Count, Affinity2->Count);

    for (UINT16 Group = 0; Group < CommonCount; ++Group)
    {
        if (Affinity1->Bitmap[Group] != Affinity2->Bitmap[Group])
            return FALSE;
    }

    for (UINT16 Group = CommonCount; Group < Affinity1->Count; ++Group)
    {
        if (Affinity1->Bitmap[Group])
            return FALSE;
    }

    for (UINT16 Group = CommonCount; Group < Affinity2->Count; ++Group)
    {
        if (Affinity2->Bitmap[Group])
            return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTAPI
KeIsSubsetAffinityEx(const PKAFFINITY_EX AffinitySubset, const PKAFFINITY_EX Affinity)
{
    KAFFINITY_EX AffinityIntersection = { 0 };

    KeAndAffinityEx(AffinitySubset, Affinity, &AffinityIntersection);
    return KeIsEqualAffinityEx(AffinitySubset, &AffinityIntersection);
}

void
NTAPI
KiSetSystemAffinityThreadToProcessor(ULONG32 Index, PGROUP_AFFINITY OldAffinity)
{
    GROUP_AFFINITY Affinity = { 0 };

    const ULONG32 Number = KiProcessorIndexToNumberMappingTable[Index];

    Affinity.Group = PROCESSOR_INDEX_GROUP_MASK(Number);
    Affinity.Mask = AFFINITY_MASK(PROCESSOR_INDEX_NUMBER_MASK(Number));

    KeSetSystemGroupAffinityThread(&Affinity, OldAffinity);
}

void
NTAPI
KiUpdateProcessorCount(ULONG ProcIndex, USHORT Group)
{
    KiActiveGroups = KiGetCurrentGroupCount();

    _disable();

    ++KeNumberProcessors_0;
    KeAddProcessorAffinityEx(&KeActiveProcessors, ProcIndex);

    _enable();

    if (!Group)
        ++KeNumberProcessorsGroup0;
}
#endif
