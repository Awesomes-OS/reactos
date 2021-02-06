/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/ntdll/include/ldrp.h
 * PURPOSE:         NTDLL Loader Internal Header
 */

#pragma once
#define _CTYPE_DISABLE_MACROS
#define WIN32_NO_STATUS

#if __cplusplus
extern "C"
{
#endif

#include <ndk/ldrfuncs.h>
#include <ndk/rtlfuncs.h>

#if __cplusplus
}
#endif

#define LDR_HASH_TABLE_ENTRIES 32
#define LDR_GET_HASH_ENTRY(x) ((x) & (LDR_HASH_TABLE_ENTRIES - 1))

/* Loader flags */
#define IMAGE_LOADER_FLAGS_COMPLUS 0x00000001
#define IMAGE_LOADER_FLAGS_SYSTEM_GLOBAL 0x01000000

#define COR_IS_32BIT_REQUIRED(_flags) \
    (((_flags) & (COMIMAGE_FLAGS_32BITREQUIRED|COMIMAGE_FLAGS_32BITPREFERRED)) == (COMIMAGE_FLAGS_32BITREQUIRED))

#define COR_IS_32BIT_PREFERRED(_flags) \
    (((_flags) & (COMIMAGE_FLAGS_32BITREQUIRED|COMIMAGE_FLAGS_32BITPREFERRED)) == (COMIMAGE_FLAGS_32BITREQUIRED|COMIMAGE_FLAGS_32BITPREFERRED))

/* Page heap flags */
#define DPH_FLAG_DLL_NOTIFY 0x40

typedef struct _LDRP_PATH_SEARCH_CONTEXT
{
    UNICODE_STRING DllSearchPath;
    BOOLEAN AllocatedOnLdrpHeap;
    LDRP_PATH_SEARCH_OPTIONS SearchOptions;
    UNICODE_STRING OriginalFullDllName; // e.g. for forwarders
} LDRP_PATH_SEARCH_CONTEXT, *PLDRP_PATH_SEARCH_CONTEXT;

typedef union _LDRP_LOAD_CONTEXT_FLAGS
{
    ULONG32 Flags;

    struct
    {
        ULONG32 ProcessImage : 1;
        ULONG32 Redirected : 1;
        ULONG32 BaseNameOnly : 1;
        ULONG32 HasFullPath : 1;
        ULONG32 KnownDll : 1;
        ULONG32 SystemImage : 1;
        ULONG32 ExecutableImage : 1;
        ULONG32 AppContainerImage : 1;
        ULONG32 CallInit : 1;
        ULONG32 UserAllocated : 1;
        ULONG32 SearchOnlyFirstPathSegment : 1;
        ULONG32 RedirectedByAPISet : 1;
        ULONG32 CorImage : 1;
        ULONG32 InRetryQueue : 1;
        ULONG32 RetryDependentLoadWhenMissing : 1;
    };
} LDRP_LOAD_CONTEXT_FLAGS, *PLDRP_LOAD_CONTEXT_FLAGS;

typedef struct _LDRP_LOAD_CONTEXT
{
    UNICODE_STRING DllName;
    PLDRP_PATH_SEARCH_CONTEXT PathSearchContext;
    HANDLE MappedSectionHandle;
    LDRP_LOAD_CONTEXT_FLAGS Flags;
    // Unknown DWORD
    NTSTATUS* LoadStatus;
    PLDR_DATA_TABLE_ENTRY ParentModule;
    PLDR_DATA_TABLE_ENTRY Module;
    LIST_ENTRY QueueListEntry; // LdrpWorkQueue or LdrpRetryQueue
    PLDR_DATA_TABLE_ENTRY PendingModule;
    struct
    {
        PLDR_DATA_TABLE_ENTRY* ImportArray;
        ULONG ImportCount;
        ULONG32 UnmappedChildCount;
        PVOID IATBase;
        ULONG IATSize;
        ULONG ImportIndex; // 0 <= ImportIndex <= ImportCount
        ULONG ThunkIndex;
        PIMAGE_IMPORT_DESCRIPTOR ImportDescriptors;
        ULONG IATOriginalProtection;
    };
    // Unknown DWORD
    PVOID GuardCFCheckFunctionPointer;
    PVOID* GuardCFCheckFunctionPointerThunk;
    SIZE_T MappedSectionSize;
    // Enclave Context
    HANDLE MappedFileHandle;
    // ProcessImageSectionViewBase
#ifndef NDEBUG
    ULONG Debug;
#endif
    WCHAR DllNameStorage[ANYSIZE_ARRAY];
} LDRP_LOAD_CONTEXT, *PLDRP_LOAD_CONTEXT;

typedef struct _TLS_VECTOR
{
    union
    {
        ULONG Length;
        HANDLE ThreadId;
    };

    struct _TLS_VECTOR* PreviousDeferredTlsVector;
    PVOID ModuleTlsData[ANYSIZE_ARRAY];
} TLS_VECTOR, *PTLS_VECTOR;

typedef struct _LDRP_LAZY_PATH_EVALUATION_CONTEXT
{
    UNICODE_STRING RemainingSearchPath;
} LDRP_LAZY_PATH_EVALUATION_CONTEXT, *PLDRP_LAZY_PATH_EVALUATION_CONTEXT;

typedef enum _LDRP_PROCESS_WORK_MODE
{
    QueuedWork,
    StandaloneRequest
} LDRP_PROCESS_WORK_MODE;

typedef enum _LDRP_DRAIN_WORK_QUEUE_TARGET
{
    LoadComplete,
    WorkComplete
} LDRP_DRAIN_WORK_QUEUE_TARGET;

typedef
NTSTATUS
(NTAPI* PLDR_APP_COMPAT_DLL_REDIRECTION_CALLBACK_FUNCTION)(
    _In_ ULONG Flags,
    _In_ PCWSTR DllName,
    _In_ PCWSTR DllPath OPTIONAL,
    _Inout_opt_ PULONG DllCharacteristics,
    _In_ PVOID CallbackData,
    _Outptr_ PWSTR* EffectiveDllPath);

/* Global data */
extern ULONG LdrInitState;
extern PVOID LdrpSystemDllBase;
extern RTL_CRITICAL_SECTION LdrpLoaderLock;
extern PVOID LdrpHeap;
extern LIST_ENTRY LdrpHashTable[LDR_HASH_TABLE_ENTRIES];
extern UNICODE_STRING LdrpDefaultPath;
extern HANDLE LdrpKnownDllDirectoryHandle;
extern ULONG LdrpFatalHardErrorCount;
extern PUNICODE_STRING LdrpTopLevelDllBeingLoaded;
extern PLDR_DATA_TABLE_ENTRY LdrpCurrentDllInitializer;
extern PLDR_DATA_TABLE_ENTRY LdrpNtDllDataTableEntry;
extern UNICODE_STRING LdrpDefaultExtension;
extern UNICODE_STRING LdrpKnownDllPath;
extern PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;
EXTERN_C BOOLEAN RtlpPageHeapEnabled;
EXTERN_C ULONG RtlpDphGlobalFlags;
extern BOOLEAN g_ShimsEnabled;
extern PVOID g_pShimEngineModule;
extern PVOID g_pfnSE_DllLoaded;
extern PVOID g_pfnSE_DllUnloaded;
extern PVOID g_pfnSE_InstallBeforeInit;
extern PVOID g_pfnSE_InstallAfterInit;
extern PVOID g_pfnSE_ProcessDying;
extern PVOID g_pfnSE_LdrResolveDllName;
extern PVOID LdrpMscoreeDllHandle;
extern BOOLEAN UseCOR, UseWOW64;
extern RTL_SRWLOCK LdrpModuleDatatableLock;
extern LIST_ENTRY LdrpWorkQueue;
extern LIST_ENTRY LdrpRetryQueue;
extern RTL_CRITICAL_SECTION LdrpWorkQueueLock;
EXTERN_C PLDR_DATA_TABLE_ENTRY LdrpImageEntry;
extern PCONTEXT LdrpProcessInitContextRecord;
extern PLDR_MANIFEST_PROBER_ROUTINE LdrpManifestProberRoutine;
extern PIMAGE_NT_HEADERS LdrpAppHeaders;
extern LIST_ENTRY LdrpTlsList;
#define TLS_BITMAP_GROW_INCREMENT 8u
extern RTL_BITMAP LdrpTlsBitmap;
extern ULONG LdrpActiveThreadCount;
extern ULONG LdrpActualBitmapSize;
extern RTL_SRWLOCK LdrpTlsLock;
extern UNICODE_STRING SlashSystem32SlashString;
extern ULONG LdrpWorkInProgress;
extern HANDLE LdrpLoadCompleteEvent, LdrpWorkCompleteEvent;
extern const UNICODE_STRING LdrpKernel32DllName;

typedef union
{
    ULONG32 Flags;

    struct
    {
#if 0
        ULONG32 LogWarning : 1;
        ULONG32 LogInformation : 1;
        ULONG32 LogDebug : 1;
        ULONG32 LogTrace : 1;
#endif
        ULONG32 BreakInDebugger : 1;
    };
} LDRP_DEBUG_FLAGS;

extern LDRP_DEBUG_FLAGS LdrpDebugFlags;

/* ldrtls.c */
typedef struct _TLS_ENTRY
{
    LIST_ENTRY TlsEntryLinks;
    IMAGE_TLS_DIRECTORY TlsDirectory;
    PLDR_DATA_TABLE_ENTRY ModuleEntry;
} TLS_ENTRY, *PTLS_ENTRY;

PTLS_ENTRY
FASTCALL
LdrpFindTlsEntry(
    IN PLDR_DATA_TABLE_ENTRY ModuleEntry
);

#define LDRP_BITMAP_BITALIGN (sizeof(ULONG)*8)
#define LDRP_BITMAP_CALC_ALIGN(x, base) (((x) + (base) - 1) / (base))

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define PTR_SUB_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(Offset)))

// undocumented LDRP DDAG storage class
typedef struct _LDRP_CSLIST_ENTRY
{
    SINGLE_LIST_ENTRY DependenciesLink;
    struct _LDR_DDAG_NODE* DependencyNode;
    SINGLE_LIST_ENTRY IncomingDependenciesLink;
    struct _LDR_DDAG_NODE* ParentNode;
#ifndef NDEBUG
    ULONG Debug;
#endif
} LDRP_CSLIST_ENTRY, *PLDRP_CSLIST_ENTRY;

typedef enum _LDR_DDAG_STATE
{
    LdrModulesMerged = -5,
    LdrModulesInitError = -4,
    LdrModulesSnapError = -3,
    LdrModulesUnloaded = -2,
    LdrModulesUnloading = -1,
    LdrModulesPlaceHolder = 0,
    LdrModulesMapping = 1,
    LdrModulesMapped = 2,
    LdrModulesWaitingForDependencies = 3,
    LdrModulesSnapping = 4,
    LdrModulesSnapped = 5,
    LdrModulesCondensed = 6,
    LdrModulesReadyToInit = 7,
    LdrModulesInitializing = 8,
    LdrModulesReadyToRun = 9
} LDR_DDAG_STATE, *PLDR_DDAG_STATE;

typedef struct _LDRP_CSLIST
{
    // see LDRP_CSLIST_ENTRY.DependenciesLink or LDRP_CSLIST_ENTRY.IncomingDependenciesLink
    PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST, *PLDRP_CSLIST;

typedef struct _LDR_SERVICE_TAG_RECORD
{
    struct _LDR_SERVICE_TAG_RECORD* Next;
    ULONG32 ServiceTag;
} LDR_SERVICE_TAG_RECORD, *PLDR_SERVICE_TAG_RECORD;

// Loader Distributed Dependency Graph Node (as in Windows Internals)
// DDAG likely stands for Distributed Dependency Acyclic Graph
typedef struct _LDR_DDAG_NODE
{
    LIST_ENTRY Modules;
    LDR_SERVICE_TAG_RECORD* ServiceTagList;
    ULONG32 LoadCount;
    ULONG32 LoadWhileUnloadingCount;
    ULONG32 LowestLink;
    LDRP_CSLIST Dependencies;
    LDRP_CSLIST IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY CondenseLink;
    ULONG32 PreorderNumber;
#ifndef NDEBUG
    ULONG Debug;
#endif
} LDR_DDAG_NODE, *PLDR_DDAG_NODE;

#ifndef NDEBUG
#define LDRP_CSLIST_ENTRY_MARKER 0xDEADBEEF
#define LDRP_DDAG_NODE_MARKER 0xDDADDADE
#define LDRP_MODULE_ENTRY_MARKER 0xFEEDFACE
#define LDRP_LOAD_CONTEXT_MARKER 0xABADBABE
#endif

#include "tracing.hpp"

#define LDRP_FIRST_MODULE(Node) (CONTAINING_RECORD((Node)->Modules.Flink, LDR_DATA_TABLE_ENTRY, NodeModuleLink))
#define LDRP_NODE_MODULE_PINNED(Node, Module) ((Node)->LoadCount == LDR_LOADCOUNT_MAX || (Module)->ProcessStaticImport)
#define LDRP_NODE_PINNED(Node) LDRP_NODE_MODULE_PINNED(Node, LDRP_FIRST_MODULE(Node))
#define LDRP_MODULE_PINNED(Module) LDRP_NODE_MODULE_PINNED((Module)->DdagNode, Module)

typedef struct _LDRP_UNICODE_STRING_BUNDLE
{
    UNICODE_STRING String;
    WCHAR StaticBuffer[128];
} LDRP_UNICODE_STRING_BUNDLE, * PLDRP_UNICODE_STRING_BUNDLE;

NTSTATUS
NTAPI
LdrpReleaseTlsEntry(
    IN PLDR_DATA_TABLE_ENTRY ModuleEntry,
    OUT PTLS_ENTRY* FoundTlsEntry OPTIONAL
);

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpAllocateModuleEntry(IN PLDRP_LOAD_CONTEXT LoadContext OPTIONAL);

NTSTATUS
NTAPI
LdrpAllocatePlaceHolder(IN PUNICODE_STRING DllName,
                        IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                        IN LDRP_LOAD_CONTEXT_FLAGS Flags,
                        IN LDR_DLL_LOAD_REASON LoadReason,
                        IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                        OUT PLDR_DATA_TABLE_ENTRY* OutLdrEntry,
                        IN NTSTATUS* OutStatus);

NTSTATUS
NTAPI
LdrpPrepareModuleForExecution(IN PLDR_DATA_TABLE_ENTRY Module,
                              IN NTSTATUS* StatusResponse);

BOOLEAN
NTAPI
LdrpInitSecurityCookie(PVOID BaseAddress,
                       ULONG_PTR SizeOfImage,
                       PULONG_PTR CookieOverride,
                       ULONG_PTR Salt,
                       OUT PIMAGE_LOAD_CONFIG_DIRECTORY* OutLoadConfig);

NTSTATUS
NTAPI
LdrpMapAndSnapDependency(PLDRP_LOAD_CONTEXT LoadContext);

ULONG32
NTAPI
LdrpHashUnicodeString(IN PUNICODE_STRING NameString);

void
NTAPI
LdrpDrainWorkQueue(IN LDRP_DRAIN_WORK_QUEUE_TARGET Target);

void
NTAPI
LdrpQueueWork(IN PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpIncrementModuleLoadCount(IN PLDR_DATA_TABLE_ENTRY Module);

NTSTATUS
NTAPI
LdrpDoPostSnapWork(IN PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpProcessMappedModule(PLDR_DATA_TABLE_ENTRY Module, LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags, BOOL AdvanceLoad);

NTSTATUS
NTAPI
LdrpResolveDllName(IN PUNICODE_STRING DosName,
                   IN OUT PLDRP_UNICODE_STRING_BUNDLE NtName,
                   OUT PUNICODE_STRING BaseDllName,
                   OUT PUNICODE_STRING FullDosName,
                   IN LDRP_LOAD_CONTEXT_FLAGS Flags);

NTSTATUS
NTAPI
LdrpSearchPath(IN PUNICODE_STRING DllName,
               IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
               IN BOOLEAN SearchInFirstSegmentOnly,
               IN OUT PLDRP_LAZY_PATH_EVALUATION_CONTEXT* LazyEvaluationContext OPTIONAL,
               IN OUT PLDRP_UNICODE_STRING_BUNDLE NtName,
               OUT PUNICODE_STRING BaseDllName,
               OUT PUNICODE_STRING FullDosName);


NTSTATUS
NTAPI
LdrpFindLoadedDllByNameLockHeld(IN PUNICODE_STRING BaseDllName,
                                IN PUNICODE_STRING FullDllName OPTIONAL,
                                IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                                IN ULONG32 BaseNameHashValue,
                                OUT PLDR_DATA_TABLE_ENTRY* Module);

NTSTATUS
NTAPI
LdrpFindLoadedDllByName(IN PUNICODE_STRING BaseDllName OPTIONAL,
                        IN PUNICODE_STRING FullDllName OPTIONAL,
                        IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                        OUT PLDR_DATA_TABLE_ENTRY* Module,
                        OUT LDR_DDAG_STATE* DdagState OPTIONAL);

NTSTATUS
NTAPI
LdrpFindExistingModule(IN PUNICODE_STRING BaseDllName OPTIONAL,
                       IN PUNICODE_STRING FullDllName OPTIONAL,
                       IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags OPTIONAL,
                       IN ULONG32 BaseNameHashValue,
                       OUT PLDR_DATA_TABLE_ENTRY* Module);

NTSTATUS
NTAPI
LdrpPreprocessDllName(IN PUNICODE_STRING DllName,
                      IN OUT PLDRP_UNICODE_STRING_BUNDLE OutputDllName,
                      IN PLDR_DATA_TABLE_ENTRY ParentEntry OPTIONAL,
                      OUT PLDRP_LOAD_CONTEXT_FLAGS LoadContextFlags);

NTSTATUS NTAPI LdrpInitializeTls(void);
NTSTATUS NTAPI LdrpAllocateTls(void);
void NTAPI LdrpFreeTls(void);
NTSTATUS NTAPI LdrpHandleTlsData(IN OUT PLDR_DATA_TABLE_ENTRY ModuleEntry);

/* ldrcor.c */
NTSTATUS
NTAPI
LdrpCorInitialize(OUT PLDR_DATA_TABLE_ENTRY* TargetEntry);

typedef __int32 (STDMETHODCALLTYPE LDRP_COREXEMAIN_FUNC)();

BOOL
NTAPI
LdrpIsILOnlyImage(PVOID BaseAddress);

NTSTATUS
NTAPI
LdrpCorValidateImage(IN PVOID ImageBase, IN LPCWSTR FileName);

NTSTATUS
NTAPI
LdrpCorProcessImports(IN PLDR_DATA_TABLE_ENTRY Module);

LDRP_COREXEMAIN_FUNC*
NTAPI
LdrpCorReplaceStartContext(IN PCONTEXT Context);

#if defined(_M_AMD64) || defined(_WIN64)
NTSTATUS
NTAPI
LdrpCorFixupImage(IN PVOID BaseAddress);
#endif

/* ldrinit.c */
#if 0
NTSTATUS NTAPI LdrpRunInitializeRoutines(IN PCONTEXT Context OPTIONAL);
#endif
void NTAPI LdrpInitializeThread(IN PCONTEXT Context);
void NTAPI LdrpCallTlsInitializers(IN PLDR_DATA_TABLE_ENTRY Module, IN ULONG Reason);
EXTERN_C BOOLEAN NTAPI LdrpCallInitRoutine(IN PDLL_INIT_ROUTINE EntryPoint, IN PVOID BaseAddress, IN ULONG Reason,
                                  IN PVOID Context);
NTSTATUS NTAPI LdrpInitializeProcess(IN PCONTEXT Context, IN PVOID SystemDllBaseAddress);
void NTAPI LdrpInitFailure(NTSTATUS Status);

/* It actually returns void in Windows */
NTSTATUS NTAPI LdrpValidateImageForMp(IN PLDR_DATA_TABLE_ENTRY Module);

void NTAPI LdrpEnsureLoaderLockIsHeld(void);

NTSTATUS
NTAPI
LdrpBuildSystem32FileName(IN PLDRP_UNICODE_STRING_BUNDLE DestinationString,
                          IN PUNICODE_STRING FileName OPTIONAL);


NTSTATUS
NTAPI
LdrpSnapModule(IN PLDRP_LOAD_CONTEXT LoadContext);


/* ldrutils.c */
EXTERN_C
NTSTATUS
NTAPI
LdrpGetProcedureAddress(IN PVOID BaseAddress,
                        IN PSTR Name,
                        IN ULONG Ordinal,
                        OUT PVOID* ProcedureAddress);

void NTAPI
LdrpInsertDataTableEntry(IN PLDR_DATA_TABLE_ENTRY Module);

NTSTATUS
NTAPI
LdrpLoadDll(IN PUNICODE_STRING RawDllName,
            IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
            IN LDRP_LOAD_CONTEXT_FLAGS LoaderFlags,
            OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry);

void
NTAPI
LdrpProcessWork(PLDRP_LOAD_CONTEXT LoadContext, LDRP_PROCESS_WORK_MODE Mode);

ULONG NTAPI
LdrpClearLoadInProgress(void);

NTSTATUS
NTAPI
LdrpSetProtection(PVOID ViewBase,
                  BOOLEAN Restore);


ULONG
NTAPI
LdrpRelocateImage(IN PVOID NewBase,
                  IN PCCH LoaderName,
                  IN PIMAGE_NT_HEADERS NtHeaders,
                  IN ULONG Success,
                  IN ULONG Conflict,
                  IN ULONG Invalid);
ULONG
NTAPI
LdrpProtectAndRelocateImage(IN PVOID NewBase,
                            IN PCCH LoaderName,
                            IN ULONG Success,
                            IN ULONG Conflict,
                            IN ULONG Invalid);

NTSTATUS
NTAPI
LdrpFindLoadedDllByAddress(IN PVOID Base,
                           OUT PLDR_DATA_TABLE_ENTRY* Module,
                           LDR_DDAG_STATE* DdagState);
NTSTATUS
NTAPI
LdrpFindLoadedDllByHandle(IN PVOID Base,
                          OUT PLDR_DATA_TABLE_ENTRY* Module,
                          LDR_DDAG_STATE* DdagState);

NTSTATUS
NTAPI
LdrpApplyFileNameRedirection(IN PUNICODE_STRING DllName,
                             IN OUT PLDRP_UNICODE_STRING_BUNDLE RedirectedDllName,
                             OUT PBOOLEAN RedirectedSxS);

NTSTATUS
NTAPI
LdrpLoadKnownDll(IN PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpMapDllNtFileName(IN PLDRP_LOAD_CONTEXT LoadContext,
                     IN PUNICODE_STRING NtPathDllName);

NTSTATUS
NTAPI
LdrpMapDllFullPath(IN PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpMapDllSearchPath(IN PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpMapDllRetry(IN PLDRP_LOAD_CONTEXT LoadContext);

EXTERN_C void NTAPI
LdrpRecordUnloadEvent(_In_ PLDR_DATA_TABLE_ENTRY Module);

void NTAPI
LdrpGetShimEngineInterface(void);

void
NTAPI
LdrpLoadShimEngine(IN PWSTR ImageName,
                   IN PUNICODE_STRING ProcessImage,
                   IN PVOID pShimData);

void NTAPI
LdrpUnloadShimEngine(void);

NTSTATUS
NTAPI
LdrpCodeAuthzCheckDllAllowed(IN PUNICODE_STRING FullName,
                             IN HANDLE DllHandle);

LDRP_LOAD_CONTEXT_FLAGS
NTAPI
LdrpDllCharacteristicsToLoadFlags(ULONG DllCharacteristics);

NTSTATUS
NTAPI
LdrpFindOrPrepareLoadingModule(IN PUNICODE_STRING BaseDllName,
                               IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                               IN LDRP_LOAD_CONTEXT_FLAGS ContextFlags,
                               IN LDR_DLL_LOAD_REASON LoadReason,
                               IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                               OUT PLDR_DATA_TABLE_ENTRY* OutLdrEntry,
                               OUT NTSTATUS* OutStatus);

void
NTAPI
LdrpLoadContextReplaceModule(IN PLDRP_LOAD_CONTEXT LoadContext, PLDR_DATA_TABLE_ENTRY LoadedEntry);

/* verifier.c */

EXTERN_C NTSTATUS NTAPI
LdrpInitializeApplicationVerifierPackage(IN HANDLE KeyHandle,
                                         IN PPEB Peb,
                                         IN BOOLEAN SystemWide,
                                         IN BOOLEAN ReadAdvancedOptions);

EXTERN_C NTSTATUS NTAPI
AVrfInitializeVerifier(void);

EXTERN_C void NTAPI
AVrfDllLoadNotification(IN PLDR_DATA_TABLE_ENTRY Module);

EXTERN_C void NTAPI
AVrfDllUnloadNotification(IN PLDR_DATA_TABLE_ENTRY Module);

EXTERN_C void NTAPI
AVrfPageHeapDllNotification(IN PLDR_DATA_TABLE_ENTRY Module);


/* FIXME: Cleanup this mess */
typedef NTSTATUS (NTAPI *PEPFUNC)(PPEB);
NTSTATUS LdrMapSections(HANDLE ProcessHandle,
                        PVOID ImageBase,
                        HANDLE SectionHandle,
                        PIMAGE_NT_HEADERS NTHeaders);
NTSTATUS LdrMapNTDllForProcess(HANDLE ProcessHandle,
                               PHANDLE NTDllSectionHandle);
ULONG
LdrpGetResidentSize(PIMAGE_NT_HEADERS NTHeaders);

NTSTATUS
NTAPI
LdrpLoadDependentModule(IN PANSI_STRING RawDllName,
                        IN PLDRP_LOAD_CONTEXT LoadContext,
                        IN PLDR_DATA_TABLE_ENTRY ParentModule,
                        IN LDR_DLL_LOAD_REASON LoadReason,
                        OUT PLDR_DATA_TABLE_ENTRY* DataTableEntry,
                        IN OUT PLDRP_CSLIST_ENTRY* DependencyEntryPlaceholder);

void
NTAPI
LdrpDestroyNode(IN PLDR_DDAG_NODE Node);

NTSTATUS
NTAPI
LdrpUnmapModule(IN PLDR_DATA_TABLE_ENTRY Module);

void
NTAPI
LdrpDereferenceModule(IN PLDR_DATA_TABLE_ENTRY Module);


NTSTATUS
NTAPI
LdrpDecrementNodeLoadCountLockHeld(IN PLDR_DDAG_NODE Node, IN BOOLEAN DisallowOrphaning, OUT BOOLEAN* BecameOrphan);

NTSTATUS
NTAPI
LdrpDecrementModuleLoadCountEx(IN PLDR_DATA_TABLE_ENTRY Module, IN BOOLEAN DisallowOrphaning);

NTSTATUS
NTAPI
LdrpUnloadNode(IN PLDR_DDAG_NODE Node);

NTSTATUS
NTAPI
LdrpPinModule(IN PLDR_DATA_TABLE_ENTRY Module);

NTSTATUS
NTAPI
LdrpGetFullPath(IN PUNICODE_STRING OriginalName,
                IN OUT PLDRP_UNICODE_STRING_BUNDLE ExpandedName);

NTSTATUS
NTAPI
LdrpInitializeNode(IN PLDR_DDAG_NODE DdagNode);

NTSTATUS
NTAPI
// ReSharper disable once IdentifierTypo
LdrpFastpthReloadedDll(IN PUNICODE_STRING BaseDllName,
                       IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                       IN PLDR_DATA_TABLE_ENTRY ForwarderSource OPTIONAL,
                       OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry);

NTSTATUS
NTAPI
LdrpMinimalMapModule(PLDRP_LOAD_CONTEXT LoadContext, HANDLE SectionHandle, PSIZE_T ViewSize);

NTSTATUS
NTAPI
LdrpCompleteMapModule(PLDRP_LOAD_CONTEXT LoadContext, PIMAGE_NT_HEADERS NtHeaders, NTSTATUS ImageStatus,
                      SIZE_T ViewSize);

NTSTATUS
NTAPI
LdrpRunInitializeRoutine(IN PLDR_DATA_TABLE_ENTRY Module);

void
LdrpLoadDllInternal(IN PUNICODE_STRING DllName,
                    IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                    IN LDRP_LOAD_CONTEXT_FLAGS LoadContextFlags,
                    IN LDR_DLL_LOAD_REASON LoadReason,
                    IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                    IN PLDR_DATA_TABLE_ENTRY ForwarderSource OPTIONAL,
                    OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry,
                    OUT NTSTATUS* OutStatus);

NTSTATUS
NTAPI
LdrpFindKnownDll(PUNICODE_STRING DllName,
                 PUNICODE_STRING BaseDllName,
                 PUNICODE_STRING FullDllName,
                 HANDLE* SectionHandle);

NTSTATUS
NTAPI
LdrpMapDllWithSectionHandle(PLDRP_LOAD_CONTEXT LoadContext, HANDLE SectionHandle);


NTSTATUS
NTAPI
LdrpInitializeGraphRecurse(IN PLDR_DDAG_NODE DdagNode,
                           IN NTSTATUS* StatusResponse,
                           IN OUT BOOLEAN* HasInitializing);


NTSTATUS
NTAPI
LdrpGetNtPathFromDosPath(IN PUNICODE_STRING DosPath,
                         OUT PLDRP_UNICODE_STRING_BUNDLE NtPath);


PVOID* LdrpGetNewTlsVector(IN ULONG TlsBitmapLength);

NTSTATUS
LdrpAllocateTlsEntry(
    IN PIMAGE_TLS_DIRECTORY TlsDirectory,
    IN PLDR_DATA_TABLE_ENTRY ModuleEntry,
    OUT PULONG TlsIndex,
    OUT PBOOLEAN AllocatedBitmap OPTIONAL,
    OUT PTLS_ENTRY* TlsEntry
);

void
LdrpQueueDeferredTlsData(
    IN OUT PVOID TlsVector,
    IN OUT PVOID ThreadId
);

BOOLEAN
NTAPI
LdrpIsBaseNameOnly(IN PUNICODE_STRING DllName);


NTSTATUS
NTAPI
LdrpCreateDllSection(IN PUNICODE_STRING FullName,
                     IN LDRP_LOAD_CONTEXT_FLAGS LoaderFlags,
                     OUT PHANDLE FileHandle,
                     OUT PHANDLE SectionHandle);


UINT32
NTAPI
LdrpNameToOrdinal(IN LPSTR ImportName,
                  IN ULONG NumberOfNames,
                  IN PVOID ExportBase,
                  IN PULONG NameTable,
                  IN PUSHORT OrdinalTable);


NTSTATUS
NTAPI
LdrpHandleOldFormatImportDescriptors(IN PLDRP_LOAD_CONTEXT LoadContext,
                                     IN PLDR_DATA_TABLE_ENTRY Module,
                                     IN PIMAGE_IMPORT_DESCRIPTOR ImportEntry);


NTSTATUS
NTAPI
LdrpHandleNewFormatImportDescriptors(IN PLDRP_LOAD_CONTEXT LoadContext,
                                     IN PLDR_DATA_TABLE_ENTRY Module,
                                     IN PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry);


void
NTAPI
LdrpCondenseGraph(PLDR_DDAG_NODE Node);

void
NTAPI
LdrpFreeReplacedModule(PLDR_DATA_TABLE_ENTRY Module);

void
NTAPI
LdrpFreeLoadContext(PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpBuildForwarderLink(IN PLDR_DATA_TABLE_ENTRY Source OPTIONAL,
                       IN PLDR_DATA_TABLE_ENTRY Target);

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpHandleReplacedModule(PLDR_DATA_TABLE_ENTRY Entry);

NTSTATUS
NTAPI
LdrpFindLoadedDllByMapping(PVOID ViewBase, PIMAGE_NT_HEADERS NtHeader, PLDR_DATA_TABLE_ENTRY* Module,
                           LDR_DDAG_STATE* DdagState);

NTSTATUS
NTAPI
LdrpFindLoadedDllByMappingFile(PUNICODE_STRING NtPathName, PLDR_DATA_TABLE_ENTRY* Module, LDR_DDAG_STATE* DdagState);

NTSTATUS
NTAPI
LdrpFindLoadedDll(PUNICODE_STRING RawDllName,
                  PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                  PLDR_DATA_TABLE_ENTRY* Module);

void
NTAPI
LdrpSignalModuleMapped(PLDR_DATA_TABLE_ENTRY Module);

BOOLEAN
NTAPI
LdrpIsExtensionPresent(IN PUNICODE_STRING DllName);

void
NTAPI
LdrpTrimTrailingDots(IN OUT PUNICODE_STRING DllName);

NTSTATUS
NTAPI
LdrpParseForwarderDescription(IN PVOID ForwarderPointer,
                              IN OUT PANSI_STRING ForwardedDllName,
                              IN OUT PSTR* ForwardedFunctionName,
                              IN OUT PULONG ForwardedFunctionOrdinal);

typedef union
{
    UINT8 Flags;

    struct
    {
        UINT8 DoSecurityVerification : 1;
        UINT8 NoForwarderResolution : 1;
    };
} LDRP_RESOLVE_PROCEDURE_ADDRESS_FLAGS;

NTSTATUS
NTAPI
LdrpResolveProcedureAddress(IN PLDR_DATA_TABLE_ENTRY Module,
                            IN PSTR Name,
                            IN ULONG Ordinal,
                            IN LDRP_RESOLVE_PROCEDURE_ADDRESS_FLAGS Flags,
                            OUT PVOID* ProcedureAddress);

NTSTATUS
NTAPI
LdrpLoadForwardedDll(IN PANSI_STRING RawDllName,
                     IN PLDRP_PATH_SEARCH_CONTEXT PathSearchContext,
                     IN PLDR_DATA_TABLE_ENTRY ForwarderSource,
                     IN PLDR_DATA_TABLE_ENTRY ParentEntry,
                     IN LDR_DLL_LOAD_REASON LoadReason,
                     OUT PLDR_DATA_TABLE_ENTRY* OutputLdrEntry);

void
NTAPI
LdrpReportSnapError(IN PVOID BaseAddress OPTIONAL,
                    IN PUNICODE_STRING ModuleName OPTIONAL,
                    IN PANSI_STRING FunctionName OPTIONAL,
                    IN ULONG Ordinal OPTIONAL,
                    NTSTATUS Status,
                    BOOLEAN Static);

NTSTATUS
NTAPI
LdrpResolveForwarder(IN PVOID ForwarderPointer,
                     IN PLDR_DATA_TABLE_ENTRY ExportLdrEntry,
                     IN PLDR_DATA_TABLE_ENTRY ImportLdrEntry,
                     OUT PVOID* ProcedureAddress);


_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
LdrpHeapAlloc(_In_opt_ ULONG Flags,
              _In_ SIZE_T Size);

_Success_(return != 0)
BOOLEAN
NTAPI
LdrpHeapFree(_In_opt_ ULONG Flags,
             _In_ _Post_invalid_ PVOID BaseAddress);

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
LdrpHeapReAlloc(_In_opt_ ULONG Flags,
                _In_ _Post_invalid_ PVOID Ptr,
                _In_ SIZE_T Size);

NTSTATUS
NTAPI
LdrpAppendUnicodeStringToFilenameBuffer(PLDRP_UNICODE_STRING_BUNDLE Destination, UNICODE_STRING* Source);

NTSTATUS
NTAPI
LdrpAppendAnsiStringToFilenameBuffer(PLDRP_UNICODE_STRING_BUNDLE Destination, ANSI_STRING* Source);

NTSTATUS
NTAPI
LdrpAllocateFileNameBufferIfNeeded(PLDRP_UNICODE_STRING_BUNDLE Destination, ULONG Length);

#define LdrpCreateUnicodeStringBundlePtr(Variable) RtlInitEmptyUnicodeString(&(Variable)->String, (Variable)->StaticBuffer, sizeof((Variable)->StaticBuffer)), (Variable)->String.Buffer[0] = UNICODE_NULL;
#define LdrpCreateUnicodeStringBundle(Variable) LdrpCreateUnicodeStringBundlePtr(&(Variable))
#define LdrpIsUsingUnicodeStringBundleBufferPtr(Variable) ((Variable)->String.Buffer == (Variable)->StaticBuffer)
#define LdrpFreeUnicodeStringBundlePtr(Variable) if (!LdrpIsUsingUnicodeStringBundleBufferPtr((Variable))) LdrpHeapFree(0, (Variable)->String.Buffer); LdrpCreateUnicodeStringBundlePtr(Variable);
#define LdrpFreeUnicodeStringBundle(Variable) LdrpFreeUnicodeStringBundlePtr(&(Variable))

void
NTAPI
LdrpFreeLoadContextOfNode(PLDR_DDAG_NODE Node, NTSTATUS* StatusResponse);

BOOLEAN
NTAPI
LdrpDependencyExist(PLDR_DDAG_NODE Node1, PLDR_DDAG_NODE Node2);

PLDRP_CSLIST_ENTRY
NTAPI
LdrpRecordModuleDependency(PLDR_DATA_TABLE_ENTRY LdrEntry1,
                           PLDR_DATA_TABLE_ENTRY LdrEntry2,
                           PLDRP_CSLIST_ENTRY Dependency,
                           NTSTATUS* StatusResponse);

PIMAGE_IMPORT_DESCRIPTOR
NTAPI
LdrpGetImportDescriptorForSnap(PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpFindDllActivationContext(PLDR_DATA_TABLE_ENTRY Module);

NTSTATUS
NTAPI
LdrpPrepareImportAddressTableForSnap(PLDRP_LOAD_CONTEXT LoadContext);

NTSTATUS
NTAPI
LdrpNotifyLoadOfGraph(PLDR_DDAG_NODE Node);

NTSTATUS
NTAPI
LdrpSendPostSnapNotifications(PLDR_DDAG_NODE Node);

void
NTAPI
LdrpHandlePendingModuleReplaced(PLDRP_LOAD_CONTEXT LoadContext);


NTSTATUS
NTAPI
LdrpGetProcedureAddressImpl(IN PIMAGE_EXPORT_DIRECTORY ExportDir,
                            IN ULONG ExportDirSize,
                            IN PVOID ExportDirPointer,
                            IN PVOID BaseAddress,
                            IN PSTR Name,
                            IN ULONG Ordinal,
                            OUT PVOID* ProcedureAddress);

void
NTAPI
LdrpDecrementUnmappedChildCount(IN PLDRP_LOAD_CONTEXT ParentContext,
                           IN PLDR_DATA_TABLE_ENTRY ParentModule,
                           IN PLDRP_LOAD_CONTEXT ChildContext,
                           IN PLDR_DATA_TABLE_ENTRY ChildModule);

void
NTAPI
LdrpRemoveDependencyEntry(PLDRP_CSLIST List, PSINGLE_LIST_ENTRY TargetEntry);

NTSTATUS
NTAPI
LdrpInitParallelLoadingSupport(void);

void
NTAPI
LdrpDropLastInProgressCount(void);

void
NTAPI
LdrpDoDebuggerBreak(void);

BOOLEAN
NTAPI
LdrpCheckForRetryLoading(IN PLDRP_LOAD_CONTEXT LoadContext, IN BOOLEAN InsertIntoRetryQueue);

NTSTATUS
NTAPI
LdrpAppCompatRedirect(PLDRP_LOAD_CONTEXT LoadContext, PUNICODE_STRING FullDosPath, PUNICODE_STRING BaseDllName, PLDRP_UNICODE_STRING_BUNDLE NtPath, NTSTATUS Status);

LONG
NTAPI
LdrpDebugExceptionFilter(unsigned int code, PEXCEPTION_POINTERS ep);

void
NTAPI
LdrpInsertModuleToIndexLockHeld(IN PLDR_DATA_TABLE_ENTRY Module, IN PIMAGE_NT_HEADERS NtHeader);

void
NTAPI
LdrpInsertModuleToIndex(IN PLDR_DATA_TABLE_ENTRY Module, IN PIMAGE_NT_HEADERS NtHeader);

/* EOF */


