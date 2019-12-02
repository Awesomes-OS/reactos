#pragma once

/* INCLUDES *****************************************************************/

#include "arch/ke.h"

/* INTERNAL KERNEL TYPES ****************************************************/

typedef struct _WOW64_PROCESS
{
  PVOID Wow64;
} WOW64_PROCESS, *PWOW64_PROCESS;

typedef struct _KPROFILE_SOURCE_OBJECT
{
    KPROFILE_SOURCE Source;
    LIST_ENTRY ListEntry;
} KPROFILE_SOURCE_OBJECT, *PKPROFILE_SOURCE_OBJECT;

typedef enum _CONNECT_TYPE
{
    NoConnect,
    NormalConnect,
    ChainConnect,
    UnknownConnect
} CONNECT_TYPE, *PCONNECT_TYPE;

typedef struct _DISPATCH_INFO
{
    CONNECT_TYPE Type;
    PKINTERRUPT Interrupt;
    PKINTERRUPT_ROUTINE NoDispatch;
    PKINTERRUPT_ROUTINE InterruptDispatch;
    PKINTERRUPT_ROUTINE FloatingDispatch;
    PKINTERRUPT_ROUTINE ChainedDispatch;
    PKINTERRUPT_ROUTINE *FlatDispatch;
} DISPATCH_INFO, *PDISPATCH_INFO;

typedef struct _PROCESS_VALUES
{
    LARGE_INTEGER TotalKernelTime;
    LARGE_INTEGER TotalUserTime;
    IO_COUNTERS IoInfo;
} PROCESS_VALUES, *PPROCESS_VALUES;

typedef struct _DEFERRED_REVERSE_BARRIER
{
    ULONG Barrier;
    ULONG TotalProcessors;
} DEFERRED_REVERSE_BARRIER, *PDEFERRED_REVERSE_BARRIER;

typedef struct _KI_SAMPLE_MAP
{
    LARGE_INTEGER PerfStart;
    LARGE_INTEGER PerfEnd;
    LONGLONG PerfDelta;
    LARGE_INTEGER PerfFreq;
    LONGLONG TSCStart;
    LONGLONG TSCEnd;
    LONGLONG TSCDelta;
    ULONG MHz;
} KI_SAMPLE_MAP, *PKI_SAMPLE_MAP;

#define MAX_TIMER_DPCS                      16

typedef struct _DPC_QUEUE_ENTRY
{
    PKDPC Dpc;
    PKDEFERRED_ROUTINE Routine;
    PVOID Context;
} DPC_QUEUE_ENTRY, *PDPC_QUEUE_ENTRY;

typedef struct _KNMI_HANDLER_CALLBACK
{
    struct _KNMI_HANDLER_CALLBACK* Next;
    PNMI_CALLBACK Callback;
    PVOID Context;
    PVOID Handle;
} KNMI_HANDLER_CALLBACK, *PKNMI_HANDLER_CALLBACK;

typedef PCHAR
(NTAPI *PKE_BUGCHECK_UNICODE_TO_ANSI)(
    IN PUNICODE_STRING Unicode,
    IN PCHAR Ansi,
    IN ULONG Length
);

typedef struct _PROCESSOR_GROUP_BLOCK
{
    KAFFINITY ProcessorSet;
    KAFFINITY InitialProcessorSet;
    // The following are introduced in 19H1 for AMD & NUMA
    // see ProcessorOnly in _flags for KNODE
    KAFFINITY Value3;
    KAFFINITY Value4;
} PROCESSOR_GROUP_BLOCK, *PPROCESSOR_GROUP_BLOCK;

typedef struct _THREAD_PRIORITY_VALUES
{
    BYTE FullPriority;
    BYTE Major;
    BYTE Minor;
} THREAD_PRIORITY_VALUES;

extern KAFFINITY_EX KeActiveProcessors;
extern PKNMI_HANDLER_CALLBACK KiNmiCallbackListHead;
extern KSPIN_LOCK KiNmiCallbackListLock;
extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;
extern LARGE_INTEGER KeBootTime;
extern ULONGLONG KeBootTimeBias;
extern BOOLEAN ExCmosClockIsSane;
extern USHORT KeProcessorArchitecture;
extern USHORT KeProcessorLevel;
extern USHORT KeProcessorRevision;
extern ULONG KeFeatureBits;
extern KNODE ExNode0;
extern PKNODE KeNodeBlock[];
extern UINT16 KeNumberNodes;
extern UCHAR KeProcessNodeSeed;
extern ETHREAD KiInitialThread;
extern EPROCESS KiInitialProcess;
extern PULONG KiInterruptTemplateObject;
extern PULONG KiInterruptTemplateDispatch;
extern PULONG KiInterruptTemplate2ndDispatch;
extern ULONG KiUnexpectedEntrySize;
extern ULONG_PTR KiDoubleFaultStack;
extern EX_PUSH_LOCK KernelAddressSpaceLock;
extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;
extern BOOLEAN KeThreadDpcEnable;
extern LARGE_INTEGER KiTimeIncrementReciprocal;
extern UCHAR KiTimeIncrementShiftCount;
extern ULONG KiTimeLimitIsrMicroseconds;
extern ULONG KiServiceLimit;
extern LIST_ENTRY KeBugcheckCallbackListHead, KeBugcheckReasonCallbackListHead;
extern KSPIN_LOCK BugCheckCallbackLock;
extern KDPC KiTimerExpireDpc;
extern KTIMER_TABLE_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
extern FAST_MUTEX KiGenericCallDpcMutex;
extern LIST_ENTRY KiProfileListHead, KiProfileSourceListHead;
extern KSPIN_LOCK KiProfileLock;
extern LIST_ENTRY KiProcessListHead;
extern LIST_ENTRY KiProcessInSwapListHead, KiProcessOutSwapListHead;
extern LIST_ENTRY KiStackInSwapListHead;
extern KEVENT KiSwapEvent;
extern PKPRCB KiProcessorBlock[];
extern PROCESSOR_GROUP_BLOCK KiGroupBlock[];
extern BOOLEAN KeForceGroupAwareness;
extern ULONG32 KiProcessorIndexToNumberMappingTable[];
extern ULONG32 KiProcessorNumberToIndexMappingTable[];
extern UINT16 KiActiveGroups;
extern UINT16 KiMaximumGroups;
extern ULONG_PTR KeNumberProcessors_0;
extern CCHAR KeNumberProcessorsGroup0;
// extern ULONG_PTR KiIdleSummary;
extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;
extern ULONG KeTimeIncrement;
extern ULONG KeTimeAdjustment;
extern BOOLEAN KiTimeAdjustmentEnabled;
extern LONG KiTickOffset;
extern ULONG_PTR KiBugCheckData[5];
extern ULONG KiFreezeFlag;
extern ULONG KiDPCTimeout;
extern PGDI_BATCHFLUSH_ROUTINE KeGdiFlushUserBatch;
extern ULONGLONG BootCycles, BootCyclesEnd;
extern ULONG ProcessCount;
extern VOID __cdecl KiInterruptTemplate(VOID);

/* MACROS *************************************************************************/

#define AFFINITY_MASK(Id) (((ULONG_PTR) 1) << (Id))
#define PRIORITY_MASK(Id) (((ULONG_PTR) 1) << (Id))
#define PROCESSOR_INDEX_GROUP_MASK(Index) ((Index) >> 6)
#define PROCESSOR_INDEX_NUMBER_MASK(Index) ((Index) & 0x3F)
#define PROCESSOR_NUMBER_TO_INDEX_LOOKUP(Group, Number) (((Group) << 6) + (Number))

/* Tells us if the Timer or Event is a Syncronization or Notification Object */
#define TIMER_OR_EVENT_TYPE 0x7L

/* One of the Reserved Wait Blocks, this one is for the Thread's Timer */
#define TIMER_WAIT_BLOCK 0x3L

/* INTERNAL KERNEL FUNCTIONS ************************************************/

/* Finds a new thread to run */
LONG_PTR
FASTCALL
KiSwapThread(
    IN PKTHREAD Thread,
    IN PKPRCB Prcb
);

VOID
NTAPI
KeReadyThread(
    IN PKTHREAD Thread
);

BOOLEAN
NTAPI
KeSetDisableBoostThread(
    IN OUT PKTHREAD Thread,
    IN BOOLEAN Disable
);

BOOLEAN
NTAPI
KeSetDisableBoostProcess(
    IN PKPROCESS Process,
    IN BOOLEAN Disable
);

BOOLEAN
NTAPI
KeSetAutoAlignmentProcess(
    IN PKPROCESS Process,
    IN BOOLEAN Enable
);

NTSTATUS
NTAPI
KeSetAffinityProcess(
    IN PKPROCESS Process,
    IN UINT8 Flags,
    IN PKAFFINITY_EX Affinity
);

VOID
NTAPI
KeBoostPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
);

VOID
NTAPI
KeBalanceSetManager(IN PVOID Context);

VOID
NTAPI
KiReadyThread(IN PKTHREAD Thread);

BOOLEAN
FASTCALL
KiSuspendThread(PKTHREAD Thread, PKPRCB Prcb);

ULONG
NTAPI
KeSuspendThread(PKTHREAD Thread);

BOOLEAN
NTAPI
KeReadStateThread(IN PKTHREAD Thread);

BOOLEAN
FASTCALL
KiSwapContext(
    IN KIRQL WaitIrql,
    IN PKTHREAD CurrentThread
);

VOID
NTAPI
KiAdjustQuantumThread(IN PKTHREAD Thread);

VOID
FASTCALL
KiExitDispatcher(KIRQL OldIrql);

VOID
FASTCALL
KiDeferredReadyThread(IN PKTHREAD Thread);

PKTHREAD
FASTCALL
KiIdleSchedule(
    IN PKPRCB Prcb
);

VOID
FASTCALL
KiProcessDeferredReadyList(
    IN PKPRCB Prcb
);

#if 1 || (NTDDI_VERSION >= NTDDI_WIN7)
void
FASTCALL
KiSetAffinityThread(
    IN PKTHREAD Thread,
    IN PGROUP_AFFINITY Affinity
);
#else
void
FASTCALL
KiSetAffinityThread(
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
);
#endif

PKTHREAD
FASTCALL
KiSelectNextThread(
    IN PKPRCB Prcb
);

BOOLEAN
FASTCALL
KiInsertTimerTable(
    IN PKTIMER Timer,
    IN ULONG Hand
);

VOID
FASTCALL
KiTimerListExpire(
    IN PLIST_ENTRY ExpiredListHead,
    IN KIRQL OldIrql
);

BOOLEAN
FASTCALL
KiInsertTreeTimer(
    IN PKTIMER Timer,
    IN LARGE_INTEGER Interval
);

VOID
FASTCALL
KiCompleteTimer(
    IN PKTIMER Timer,
    IN PKSPIN_LOCK_QUEUE LockQueue
);

/* gmutex.c ********************************************************************/

VOID
FASTCALL
KiAcquireGuardedMutex(
    IN OUT PKGUARDED_MUTEX GuardedMutex
);

VOID
FASTCALL
KiAcquireFastMutex(
    IN PFAST_MUTEX FastMutex
);

/* gate.c **********************************************************************/

VOID
FASTCALL
KeInitializeGate(PKGATE Gate);

VOID
FASTCALL
KeSignalGateBoostPriority(PKGATE Gate);

VOID
FASTCALL
KeWaitForGate(
    PKGATE Gate,
    KWAIT_REASON WaitReason,
    KPROCESSOR_MODE WaitMode
);

/* ipi.c ********************************************************************/

typedef enum _IPI_TYPE {
    IpiAffinity = 0,
    IpiAllButSelf,
    IpiAll
} IPI_TYPE;

VOID
FASTCALL
KiIpiSend(
    KAFFINITY TargetSet,
    ULONG IpiRequest
);

VOID
NTAPI
KiIpiSendPacket(
    IN IPI_TYPE IpiType,
    IN PKAFFINITY_EX TargetProcessors,
    IN PKIPI_WORKER WorkerFunction,
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG_PTR Context,
    IN PULONG Count
);

VOID
FASTCALL
KiIpiSignalPacketDone(
    IN PKIPI_CONTEXT PacketContext
);

VOID
FASTCALL
KiIpiSignalPacketDoneAndStall(
    IN PKIPI_CONTEXT PacketContext,
    IN volatile PULONG ReverseStall
);

/* next file ***************************************************************/

UCHAR
NTAPI
KeFindNextRightSetAffinity(
    IN UCHAR Number,
    IN ULONG Set
);

VOID
NTAPI
DbgBreakPointNoBugCheck(VOID);

VOID
NTAPI
KeInitializeProfile(
    struct _KPROFILE* Profile,
    struct _KPROCESS* Process,
    PVOID ImageBase,
    SIZE_T ImageSize,
    ULONG BucketSize,
    KPROFILE_SOURCE ProfileSource,
    KAFFINITY Affinity
);

BOOLEAN
NTAPI
KeStartProfile(
    struct _KPROFILE* Profile,
    PVOID Buffer
);

BOOLEAN
NTAPI
KeStopProfile(struct _KPROFILE* Profile);

ULONG
NTAPI
KeQueryIntervalProfile(KPROFILE_SOURCE ProfileSource);

VOID
NTAPI
KeSetIntervalProfile(
    ULONG Interval,
    KPROFILE_SOURCE ProfileSource
);

VOID
NTAPI
KeUpdateRunTime(
    PKTRAP_FRAME TrapFrame,
    KIRQL Irql
);

VOID
NTAPI
KiExpireTimers(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

VOID
NTAPI
KeInitializeThread(
    IN PKPROCESS Process,
    IN OUT PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context,
    IN PVOID Teb,
    IN PVOID KernelStack
);

VOID
NTAPI
KeUninitThread(
    IN PKTHREAD Thread
);

NTSTATUS
NTAPI
KeInitThread(
    IN OUT PKTHREAD Thread,
    IN PVOID KernelStack,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context,
    IN PVOID Teb,
    IN PKPROCESS Process
);

VOID
NTAPI
KiInitializeContextThread(
    PKTHREAD Thread,
    PKSYSTEM_ROUTINE SystemRoutine,
    PKSTART_ROUTINE StartRoutine,
    PVOID StartContext,
    PCONTEXT Context
);

VOID
NTAPI
KeStartThread(
    IN OUT PKTHREAD Thread
);

BOOLEAN
NTAPI
KeAlertThread(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE AlertMode
);

ULONG
NTAPI
KeAlertResumeThread(
    IN PKTHREAD Thread
);

ULONG
NTAPI
KeResumeThread(
    IN PKTHREAD Thread
);

PVOID
NTAPI
KeSwitchKernelStack(
    IN PVOID StackBase,
    IN PVOID StackLimit
);

VOID
NTAPI
KeRundownThread(VOID);

NTSTATUS
NTAPI
KeReleaseThread(PKTHREAD Thread);

VOID
NTAPI
KiSchedulerRundown(
    IN PKAPC Apc
);

VOID
NTAPI
KiSchedulerNop(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
);

VOID
NTAPI
KiSchedulerApc(
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

LONG
NTAPI
KeQueryBasePriorityThread(IN PKTHREAD Thread);

VOID
FASTCALL
KiSetPriorityThread(
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
);

VOID
FASTCALL
KiUnlinkThread(
    IN PKTHREAD Thread,
    IN LONG_PTR WaitStatus
);

VOID
NTAPI
KeDumpStackFrames(PULONG Frame);

BOOLEAN
NTAPI
KiTestAlert(VOID);

VOID
FASTCALL
KiUnwaitThread(
    IN PKTHREAD Thread,
    IN LONG_PTR WaitStatus,
    IN KPRIORITY Increment
);

#if (NTDDI_VERSION >= NTDDI_WIN8)
VOID
NTAPI
KeInitializeProcess(
    struct _KPROCESS *Process,
    IN KPRIORITY Priority,
    IN PGROUP_AFFINITY Affinity,
    IN BOOLEAN Enable
);
#elif (NTDDI_VERSION >= NTDDI_LONGHORN)
VOID
NTAPI
KeInitializeProcess(
    struct _KPROCESS *Process,
    KPRIORITY Priority,
    KAFFINITY Affinity,
    ULONG_PTR DirectoryTableBase,
    IN BOOLEAN Enable
);
#elif 1
VOID
NTAPI
KeInitializeProcess(
    struct _KPROCESS *Process,
    KPRIORITY Priority,
    PGROUP_AFFINITY Affinity,
    PULONG_PTR DirectoryTableBase,
    BOOLEAN Enable
);
#else
VOID
NTAPI
KeInitializeProcess(
    struct _KPROCESS *Process,
    KPRIORITY Priority,
    KAFFINITY Affinity,
    PULONG_PTR DirectoryTableBase,
    IN BOOLEAN Enable
);
#endif

VOID
NTAPI
KeSetQuantumProcess(
    IN PKPROCESS Process,
    IN UCHAR Quantum
);

KPRIORITY
NTAPI
KeSetPriorityAndQuantumProcess(
    IN PKPROCESS Process,
    IN KPRIORITY Priority,
    IN UCHAR Quantum OPTIONAL
);

ULONG
NTAPI
KeForceResumeThread(IN PKTHREAD Thread);

#if (NTDDI_VERSION < NTDDI_WIN8)
VOID
NTAPI
KeThawAllThreads(
    VOID
);

VOID
NTAPI
KeFreezeAllThreads(
    VOID
);
#endif

BOOLEAN
NTAPI
KeDisableThreadApcQueueing(IN PKTHREAD Thread);

VOID
FASTCALL
KiWaitTest(
    PVOID Object,
    KPRIORITY Increment
);

VOID
NTAPI
KeContextToTrapFrame(
    PCONTEXT Context,
    PKEXCEPTION_FRAME ExeptionFrame,
    PKTRAP_FRAME TrapFrame,
    ULONG ContextFlags,
    KPROCESSOR_MODE PreviousMode
);

VOID
NTAPI
Ke386SetIOPL(VOID);

VOID
NTAPI
KiCheckForKernelApcDelivery(VOID);

LONG
NTAPI
KiInsertQueue(
    IN PKQUEUE Queue,
    IN PLIST_ENTRY Entry,
    BOOLEAN Head
);

VOID
NTAPI
KiTimerExpiration(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

ULONG
NTAPI
KeSetProcess(
    struct _KPROCESS* Process,
    KPRIORITY Increment,
    BOOLEAN InWait
);

VOID
NTAPI
KeInitializeEventPair(PKEVENT_PAIR EventPair);

VOID
NTAPI
KiInitializeUserApc(
    IN PKEXCEPTION_FRAME Reserved,
    IN PKTRAP_FRAME TrapFrame,
    IN PKNORMAL_ROUTINE NormalRoutine,
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
);

PLIST_ENTRY
NTAPI
KeFlushQueueApc(
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE PreviousMode
);

VOID
NTAPI
KiAttachProcess(
    struct _KTHREAD *Thread,
    struct _KPROCESS *Process,
    PKLOCK_QUEUE_HANDLE ApcLock,
    struct _KAPC_STATE *SavedApcState
);

VOID
NTAPI
KiSwapProcess(
    struct _KPROCESS *NewProcess,
    struct _KPROCESS *OldProcess
);

BOOLEAN
NTAPI
KeTestAlertThread(IN KPROCESSOR_MODE AlertMode);

BOOLEAN
NTAPI
KeRemoveQueueApc(PKAPC Apc);

VOID
FASTCALL
KiActivateWaiterQueue(IN PKQUEUE Queue);

ULONG
NTAPI
KeQueryRuntimeProcess(IN PKPROCESS Process,
                      OUT PULONG UserTime);

VOID
NTAPI
KeQueryValuesProcess(IN PKPROCESS Process,
                     PPROCESS_VALUES Values);

/* INITIALIZATION FUNCTIONS *************************************************/

INIT_FUNCTION
BOOLEAN
NTAPI
KeInitSystem(VOID);

INIT_FUNCTION
VOID
NTAPI
KeInitExceptions(VOID);

VOID
NTAPI
KeInitInterrupts(VOID);

INIT_FUNCTION
VOID
NTAPI
KiInitializeBugCheck(VOID);

INIT_FUNCTION
VOID
NTAPI
KiSystemStartup(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

BOOLEAN
NTAPI
KiDeliverUserApc(PKTRAP_FRAME TrapFrame);

VOID
NTAPI
KiMoveApcState(
    PKAPC_STATE OldState,
    PKAPC_STATE NewState
);

VOID
NTAPI
KiAddProfileEvent(
    KPROFILE_SOURCE Source,
    ULONG Pc
);

VOID
NTAPI
KiDispatchException(
    PEXCEPTION_RECORD ExceptionRecord,
    PKEXCEPTION_FRAME ExceptionFrame,
    PKTRAP_FRAME Tf,
    KPROCESSOR_MODE PreviousMode,
    BOOLEAN SearchFrames
);

VOID
NTAPI
KeTrapFrameToContext(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PCONTEXT Context
);

DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheckWithTf(
    ULONG BugCheckCode,
    ULONG_PTR BugCheckParameter1,
    ULONG_PTR BugCheckParameter2,
    ULONG_PTR BugCheckParameter3,
    ULONG_PTR BugCheckParameter4,
    PKTRAP_FRAME Tf
);

BOOLEAN
NTAPI
KiHandleNmi(VOID);

VOID
NTAPI
KeFlushCurrentTb(VOID);

BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID);

VOID
FASTCALL
KeZeroPages(IN PVOID Address,
            IN ULONG Size);

BOOLEAN
FASTCALL
KeInvalidAccessAllowed(IN PVOID TrapInformation OPTIONAL);

VOID
NTAPI
KeRosDumpStackFrames(
    PULONG_PTR Frame,
    ULONG FrameCount
);

VOID
NTAPI
KeSetSystemTime(
    IN PLARGE_INTEGER NewSystemTime,
    OUT PLARGE_INTEGER OldSystemTime,
    IN BOOLEAN FixInterruptTime,
    IN PLARGE_INTEGER HalTime
);

ULONG
NTAPI
KeV86Exception(
    ULONG ExceptionNr,
    PKTRAP_FRAME Tf,
    ULONG address
);

VOID
NTAPI
KiStartUnexpectedRange(
    VOID
);

VOID
NTAPI
KiEndUnexpectedRange(
    VOID
);

NTSTATUS
NTAPI
KiRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT Context,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN BOOLEAN SearchFrames
);

NTSTATUS
NTAPI
KiContinue(
    IN PCONTEXT Context,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiServiceExit(
    IN PKTRAP_FRAME TrapFrame,
    IN NTSTATUS Status
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiServiceExit2(
    IN PKTRAP_FRAME TrapFrame
);

#ifndef _M_AMD64
VOID
FASTCALL
KiInterruptDispatch(
    IN PKTRAP_FRAME TrapFrame,
    IN PKINTERRUPT Interrupt
);
#endif

VOID
FASTCALL
KiChainedDispatch(
    IN PKTRAP_FRAME TrapFrame,
    IN PKINTERRUPT Interrupt
);

INIT_FUNCTION
VOID
NTAPI
KiInitializeMachineType(
    VOID
);

VOID
NTAPI
KiSetupStackAndInitializeKernel(
    IN PKPROCESS InitProcess,
    IN PKTHREAD InitThread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

INIT_FUNCTION
VOID
NTAPI
KiInitSpinLocks(
    IN PKPRCB Prcb,
    IN CCHAR Number
);

INIT_FUNCTION
LARGE_INTEGER
NTAPI
KiComputeReciprocal(
    IN LONG Divisor,
    OUT PUCHAR Shift
);

INIT_FUNCTION
VOID
NTAPI
KiInitSystem(IN PEPROCESS InitProcess);

VOID
FASTCALL
KiInsertQueueApc(
    IN PKAPC Apc
);

VOID
FASTCALL
KiSignalThreadForApc(
    IN PKPRCB Prcb,
    IN PKAPC Apc,
    IN KPRIORITY PriorityBoost,
    IN KIRQL OldIrql
);

NTSTATUS
NTAPI
KiCallUserMode(
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength
);

DECLSPEC_NORETURN
VOID
FASTCALL
KiCallbackReturn(
    IN PVOID Stack,
    IN NTSTATUS Status
);

INIT_FUNCTION
VOID
NTAPI
KiInitMachineDependent(VOID);

BOOLEAN
NTAPI
KeFreezeExecution(IN PKTRAP_FRAME TrapFrame,
                  IN PKEXCEPTION_FRAME ExceptionFrame);

VOID
NTAPI
KeThawExecution(IN BOOLEAN Enable);

VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(
    IN OUT PKSPIN_LOCK_QUEUE LockQueue
);

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel(
    IN OUT PKSPIN_LOCK_QUEUE LockQueue
);

VOID
NTAPI
KiRestoreProcessorControlState(
    IN PKPROCESSOR_STATE ProcessorState
);

VOID
NTAPI
KiSaveProcessorControlState(
    OUT PKPROCESSOR_STATE ProcessorState
);

VOID
NTAPI
KiSaveProcessorState(
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
);

VOID
FASTCALL
KiRetireDpcList(
    IN PKPRCB Prcb
);

VOID
NTAPI
KiQuantumEnd(
    VOID
);

VOID
NTAPI
KiIdleLoop(VOID);

DECLSPEC_NORETURN
VOID
FASTCALL
KiSystemFatalException(
    IN ULONG ExceptionCode,
    IN PKTRAP_FRAME TrapFrame
);

PVOID
NTAPI
KiPcToFileHeader(IN PVOID Eip,
                 OUT PLDR_DATA_TABLE_ENTRY *LdrEntry,
                 IN BOOLEAN DriversOnly,
                 OUT PBOOLEAN InKernel);

PVOID
NTAPI
KiRosPcToUserFileHeader(IN PVOID Eip,
                        OUT PLDR_DATA_TABLE_ENTRY *LdrEntry);

PCHAR
NTAPI
KeBugCheckUnicodeToAnsi(
    IN PUNICODE_STRING Unicode,
    OUT PCHAR Ansi,
    IN ULONG Length
);

ULONG64
NTAPI
KiEndThreadCycleAccumulation(
    IN PKPRCB Prcb,
    IN PKTHREAD Thread,
    OUT ULONG64* CurrentClock OPTIONAL
);

ULONG64
NTAPI
KiUpdateTotalCyclesCurrentThread(
    IN PKPRCB Prcb,
    IN PKTHREAD Thread,
    OUT ULONG64* CurrentClock OPTIONAL
);

void
FASTCALL
KiConfigureInitialNodes(IN PKPRCB Prcb);

void
FASTCALL
KiConfigureProcessorBlock(IN PKPRCB Prcb);

void
FASTCALL
KeSetGroupMaskProcess(PKPROCESS Process, ULONG GroupMask);

NTSTATUS
NTAPI
KeFirstGroupAffinityEx(IN OUT PGROUP_AFFINITY ThreadAffinity, IN PKAFFINITY_EX ProcessAffinity);

BOOLEAN
NTAPI
KiPrcbInGroupAffinity(IN const KPRCB* const Prcb, IN const GROUP_AFFINITY* const Affinity);

UINT16
NTAPI
KeSelectIdealProcessor(
    IN PKNODE Node,
    IN PGROUP_AFFINITY ThreadAffinity,
    IN PUINT16 SeedPointer OPTIONAL
);

PKNODE
FASTCALL
KeSelectNodeForAffinity(PGROUP_AFFINITY Affinity);

ULONG32
FASTCALL
KiSetIdealNodeProcessByGroup(PKPROCESS Process, PKNODE Node, UINT16 Group);

void
FASTCALL
KiExtendProcessAffinity(PKPROCESS Process, UINT16 Group);

void
FASTCALL
KiUpdateNodeAffinitizedFlag(PKTHREAD Thread);

ULONGLONG
NTAPI
KiQueryUnbiasedInterruptTime(BOOLEAN WaitForSafePoint);

BOOLEAN
NTAPI
KiFreezeSingleThread(PKPRCB Prcb, PKTHREAD Thread);

BOOLEAN
NTAPI
KeFreezeProcess(PKPROCESS Process, BOOLEAN DeepFreeze);

VOID
NTAPI
KeForceResumeProcess(PKPROCESS Process);

BOOLEAN
NTAPI
PsFreezeProcess(PEPROCESS Process, BOOLEAN DeepFreeze);

ULONG32
NTAPI
KeThawProcess(PKPROCESS Process, BOOLEAN DeepUnfreeze);

void
NTAPI
PsThawProcess(PEPROCESS Process, BOOLEAN DeepUnfreeze);

ULONG32
NTAPI
KeFindFirstSetLeftGroupAffinity(PGROUP_AFFINITY Affinity);

ULONG32
NTAPI
KeFindFirstSetRightGroupAffinity(PGROUP_AFFINITY Affinity);

ULONG
NTAPI
KeCountSetBitsGroupAffinity(PGROUP_AFFINITY Affinity);

BOOLEAN
NTAPI
KiVerifyReservedFieldGroupAffinity(const GROUP_AFFINITY* const Affinity);

BOOLEAN
NTAPI
KeVerifyGroupAffinity(const GROUP_AFFINITY* const Affinity, const BOOLEAN AllowEmptyMask);

PKPRCB
NTAPI
KiConsumeNextProcessor(GROUP_AFFINITY* const Affinity);

KAFFINITY
NTAPI
KeQueryGroupAffinityEx(IN PKAFFINITY_EX Affinity, IN USHORT GroupNumber);

#if 0
NTSTATUS
NTAPI
KeConnectInterrupt(
    _Inout_ PKINTERRUPT* InterruptObject,
    _In_ ULONG InterruptCount
);

NTSTATUS
NTAPI
KeDisconnectInterrupt(
    _Inout_ PKINTERRUPT* InterruptObject,
    _In_ ULONG InterruptCount
);
#else
BOOLEAN
NTAPI
KeConnectInterrupt(IN PKINTERRUPT Interrupt);

BOOLEAN
NTAPI
KeDisconnectInterrupt(IN PKINTERRUPT Interrupt);
#endif

BOOLEAN
NTAPI
KeAndAffinityEx(const PKAFFINITY_EX Affinity1, const PKAFFINITY_EX Affinity2, const PKAFFINITY_EX AffinityResult);

BOOLEAN
NTAPI
KeIsEqualAffinityEx(const PKAFFINITY_EX Affinity1, const PKAFFINITY_EX Affinity2);

BOOLEAN
NTAPI
KeIsSubsetAffinityEx(const PKAFFINITY_EX AffinitySubset, const PKAFFINITY_EX Affinity);

KAFFINITY
NTAPI
KeSetLegacyAffinityThread(
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
);

void
NTAPI
KiSetSystemAffinityThreadToProcessor(ULONG32 Index, PGROUP_AFFINITY OldAffinity);

ULONG
FASTCALL
KeAddProcessorAffinityEx(IN OUT PKAFFINITY_EX TargetAffinity, IN ULONG32 ProcessorIndex);

void
NTAPI
KeSetPriorityBoost(IN PKTHREAD Thread, IN KPRIORITY Priority);

void
NTAPI
KiSetQuantumTargetThread(IN PKPRCB Prcb, IN PKTHREAD Thread, IN BOOLEAN UpdateCurrentThread);

ULARGE_INTEGER
NTAPI
KiCaptureThreadCycleTime(IN PKTHREAD Thread);

THREAD_PRIORITY_VALUES
NTAPI
KiExtractThreadPriorityValues(IN BYTE Priority);

void
NTAPI
KiVBoxPrint(const char *s);

void
NTAPI
KiVBoxPrintInteger(ULONG Value);

void
NTAPI
KiUpdateProcessorCount(ULONG ProcIndex, USHORT Group);

VOID
NTAPI
KiUpdateThreadPriority(IN PKPRCB Prcb, IN PKTHREAD Thread, IN SCHAR Priority);

void
NTAPI
KiSetProcessorIdle(IN PKPRCB Prcb, BOOLEAN Idle, BOOLEAN UseIdleScheduler);

BOOLEAN
NTAPI
KeAreInterruptsEnabled(VOID);

#include "ke_x.h"
