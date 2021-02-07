#pragma once

#if __cplusplus
EXTERN_C
{
#endif

#include <ndk/ldrtypes.h>

#if __cplusplus
}
#endif

typedef PVOID(NTAPI LDRP_HEAPALLOC_FUNC)(IN ULONG Flags OPTIONAL, IN SIZE_T Size);
typedef BOOLEAN(NTAPI LDRP_HEAPFREE_FUNC)(IN ULONG Flags OPTIONAL, IN PVOID BaseAddress);
typedef PVOID(NTAPI LDRP_HEAPREALLOC_FUNC)(IN ULONG Flags OPTIONAL, IN PVOID Ptr, IN SIZE_T Size);

EXTERN_C LDRP_HEAPALLOC_FUNC *LdrpHeapAllocProc;
EXTERN_C LDRP_HEAPFREE_FUNC *LdrpHeapFreeProc;
EXTERN_C LDRP_HEAPREALLOC_FUNC *LdrpHeapReAllocProc;

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
EXTERN_C
PVOID
NTAPI
LdrpAllocateHeap(_In_opt_ ULONG Flags,
                 _In_ SIZE_T Size);

_Success_(return != 0)
EXTERN_C
BOOLEAN
NTAPI
LdrpFreeHeap(_In_opt_ ULONG Flags,
             _In_ _Post_invalid_ PVOID BaseAddress);

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
EXTERN_C
PVOID
NTAPI
LdrpReAllocateHeap(_In_opt_ ULONG Flags,
                   _In_ _Post_invalid_ PVOID Ptr,
                   _In_ SIZE_T Size);

/* HEAP */
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
EXTERN_C
PVOID
NTAPI
LdrpRtlHeapAlloc(_In_opt_ ULONG Flags,
                 _In_ SIZE_T Size);

_Success_(return != 0)
EXTERN_C
BOOLEAN
NTAPI
LdrpRtlHeapFree(_In_opt_ ULONG Flags,
                _In_ _Post_invalid_ PVOID BaseAddress);

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
EXTERN_C
PVOID
NTAPI
LdrpRtlHeapReAlloc(_In_opt_ ULONG Flags,
                   _In_ _Post_invalid_ PVOID Ptr,
                   _In_ SIZE_T Size);

/* API */
_Success_(return >= 0)
EXTERN_C
NTSTATUS
NTAPI
LdrpAllocateUnicodeString(_Inout_ PUNICODE_STRING StringOut,
                          _In_ ULONG Length);

EXTERN_C
VOID
NTAPI
LdrpFreeUnicodeString(_Inout_ PUNICODE_STRING String);

/* Non-allocating routines */
EXTERN_C
PVOID
NTAPI
LdrpFetchAddressOfEntryPoint(_In_ PVOID ImageBase);

VOID
NTAPI
LdrpInitializeProcessCompat(PVOID pProcessActctx, PVOID* pOldShimData);
