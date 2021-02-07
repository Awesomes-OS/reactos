/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    rtlfuncs.inl

Abstract:

    Inline function definitions for the Run-Time Library

Authors:

    Alex Ionescu
    Andrew Boyarshin

--*/

#ifndef _RTLFUNCS_INL
#define _RTLFUNCS_INL

#ifdef __cplusplus
extern "C" {
#endif

#define FAST_FAIL_LEGACY_GS_VIOLATION           0
#define FAST_FAIL_VTGUARD_CHECK_FAILURE         1
#define FAST_FAIL_STACK_COOKIE_CHECK_FAILURE    2
#define FAST_FAIL_CORRUPT_LIST_ENTRY            3
#define FAST_FAIL_INCORRECT_STACK               4
#define FAST_FAIL_INVALID_ARG                   5
#define FAST_FAIL_GS_COOKIE_INIT                6
#define FAST_FAIL_FATAL_APP_EXIT                7
#define FAST_FAIL_RANGE_CHECK_FAILURE           8
#define FAST_FAIL_UNSAFE_REGISTRY_ACCESS        9
#define FAST_FAIL_GUARD_ICALL_CHECK_FAILURE     10
#define FAST_FAIL_GUARD_WRITE_CHECK_FAILURE     11
#define FAST_FAIL_INVALID_FIBER_SWITCH          12
#define FAST_FAIL_INVALID_SET_OF_CONTEXT        13
#define FAST_FAIL_INVALID_REFERENCE_COUNT       14
#define FAST_FAIL_INVALID_JUMP_BUFFER           18
#define FAST_FAIL_MRDATA_MODIFIED               19
#define FAST_FAIL_INVALID_FAST_FAIL_CODE        0xFFFFFFFF

#if !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && (defined(_M_CEE_PURE) || defined(_M_CEE_SAFE))
#define NO_KERNEL_LIST_ENTRY_CHECKS
#endif

#if !defined(EXTRA_KERNEL_LIST_ENTRY_CHECKS) && !defined(NO_KERNEL_LIST_ENTRY_CHECKS) && !defined(NTOS_MODE_USER) && defined(__REACTOS__)
#define EXTRA_KERNEL_LIST_ENTRY_CHECKS
#endif

#ifndef RTLFUNCS_COMPILER_PREPROCESSOR
#if defined(MIDL_PASS) || defined(SORTPP_PASS) || defined(RC_INVOKED) || defined(Q_MOC_RUN) || defined(__midl)
#define RTLFUNCS_COMPILER_PREPROCESSOR 0
#else
#define RTLFUNCS_COMPILER_PREPROCESSOR 1
#endif
#endif

//
// List Functions
//

#define RTL_STATIC_LIST_HEAD(x) LIST_ENTRY x = { &x, &x }

#if RTLFUNCS_COMPILER_PREPROCESSOR

DECLSPEC_NORETURN
FORCEINLINE
void
RtlFailFast(_In_ ULONG Code)
{
    __fastfail(Code);
}

DECLSPEC_NORETURN
FORCEINLINE
void
FatalListEntryError(
    _In_opt_ PVOID P1,
    _In_opt_ PVOID P2,
    _In_opt_ PVOID P3
)
{
    UNREFERENCED_PARAMETER(P1);
    UNREFERENCED_PARAMETER(P2);
    UNREFERENCED_PARAMETER(P3);

#ifdef _IN_KERNEL_
    void NTAPI KiVBoxPrint(const char *s);

    KiVBoxPrint("FatalListEntryError\n");
#endif

    RtlFailFast(FAST_FAIL_CORRUPT_LIST_ENTRY);
}

FORCEINLINE
void
RtlpCheckListEntry(_In_ PLIST_ENTRY Entry)
{
    if (!Entry || !Entry->Flink || !Entry->Blink)
        FatalListEntryError(Entry->Blink, Entry, Entry->Flink);
    if (Entry->Flink->Blink != Entry || Entry->Blink->Flink != Entry)
        FatalListEntryError(Entry->Blink, Entry, Entry->Flink);
}

FORCEINLINE
void
InitializeListHead(_Out_ PLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

//
//  void
//  InitializeListHead32(PLIST_ENTRY32 ListHead);
//
#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

FORCEINLINE
void
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldFlink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    RtlpCheckListEntry(ListHead);
#endif
    OldFlink = ListHead->Flink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    if (OldFlink->Blink != ListHead)
        FatalListEntryError(ListHead, OldFlink, OldFlink->Blink);
#endif
    Entry->Flink = OldFlink;
    Entry->Blink = ListHead;
    OldFlink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
void
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
)
{
    PLIST_ENTRY OldBlink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    RtlpCheckListEntry(ListHead);
#endif
    OldBlink = ListHead->Blink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    if (OldBlink->Flink != ListHead)
        FatalListEntryError(OldBlink, ListHead, OldBlink->Flink);
#endif
    Entry->Flink = ListHead;
    Entry->Blink = OldBlink;
    OldBlink->Flink = Entry;
    ListHead->Blink = Entry;
}

_Must_inspect_result_
FORCEINLINE
BOOLEAN
IsListEmpty(_In_ const LIST_ENTRY * ListHead)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead
)
{
    PSINGLE_LIST_ENTRY FirstEntry;
    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL) {
        ListHead->Next = FirstEntry->Next;
    }

    return FirstEntry;
}

FORCEINLINE
void
PushEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY Entry
)
{
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
}

FORCEINLINE
BOOLEAN
RemoveEntryListUnsafe(_In_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY OldFlink;
    PLIST_ENTRY OldBlink;

    OldFlink = Entry->Flink;
    OldBlink = Entry->Blink;
    OldFlink->Blink = OldBlink;
    OldBlink->Flink = OldFlink;
    return (BOOLEAN)(OldFlink == OldBlink);
}

FORCEINLINE
BOOLEAN
RemoveEntryList(_In_ PLIST_ENTRY Entry)
{
    PLIST_ENTRY OldFlink;
    PLIST_ENTRY OldBlink;

    OldFlink = Entry->Flink;
    OldBlink = Entry->Blink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
#ifdef EXTRA_KERNEL_LIST_ENTRY_CHECKS
    if (OldFlink == Entry || OldBlink == Entry)
        FatalListEntryError(OldBlink, Entry, OldFlink);
#endif
    if (OldFlink->Blink != Entry || OldBlink->Flink != Entry)
        FatalListEntryError(OldBlink, Entry, OldFlink);
#endif
    OldFlink->Blink = OldBlink;
    OldBlink->Flink = OldFlink;
    return (BOOLEAN)(OldFlink == OldBlink);
}

FORCEINLINE
PLIST_ENTRY
RemoveHeadList(_Inout_ PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    RtlpCheckListEntry(ListHead);
#ifdef EXTRA_KERNEL_LIST_ENTRY_CHECKS
    if (ListHead->Flink == ListHead || ListHead->Blink == ListHead)
        FatalListEntryError(ListHead->Blink, ListHead, ListHead->Flink);
#endif
#endif
    Entry = ListHead->Flink;
    Flink = Entry->Flink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    if (Entry->Blink != ListHead || Flink->Blink != Entry)
        FatalListEntryError(ListHead, Entry, Flink);
#endif
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

FORCEINLINE
PLIST_ENTRY
RemoveTailList(_Inout_ PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    RtlpCheckListEntry(ListHead);
#ifdef EXTRA_KERNEL_LIST_ENTRY_CHECKS
    if (ListHead->Flink == ListHead || ListHead->Blink == ListHead)
        FatalListEntryError(ListHead->Blink, ListHead, ListHead->Flink);
#endif
#endif
    Entry = ListHead->Blink;
    Blink = Entry->Blink;
#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    if (Blink->Flink != Entry || Entry->Flink != ListHead)
        FatalListEntryError(Blink, Entry, ListHead);
#endif
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

FORCEINLINE
void
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
)
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

#ifndef NO_KERNEL_LIST_ENTRY_CHECKS
    RtlpCheckListEntry(ListHead);
    RtlpCheckListEntry(ListToAppend);
#endif
    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
}

#endif /* RTLFUNCS_COMPILER_PREPROCESSOR */

//
// LUID Macros
//

//
// BOOLEAN
// RtlEqualLuid(PLUID L1, IN PLUID L2);
//
#define RtlEqualLuid(L1, L2) (((L1)->HighPart == (L2)->HighPart) && \
                              ((L1)->LowPart  == (L2)->LowPart))

//
// BOOLEAN
// RtlIsZeroLuid(IN PLUID L1);
//
#define RtlIsZeroLuid(_L1) \
    ((BOOLEAN) ((!(_L1)->LowPart) && (!(_L1)->HighPart)))

#if RTLFUNCS_COMPILER_PREPROCESSOR

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(_In_ LONG Val)
{
    LUID Luid;
    LARGE_INTEGER Temp;

    Temp.QuadPart = Val;
    Luid.LowPart = Temp.u.LowPart;
    Luid.HighPart = Temp.u.HighPart;
    return Luid;
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(_In_ ULONG Val)
{
    LUID Luid;

    Luid.LowPart = Val;
    Luid.HighPart = 0;
    return Luid;
}

#endif /* RTLFUNCS_COMPILER_PREPROCESSOR */

//
// ASSERT Macros
//

#ifndef ASSERT

#if DBG

#define RTL_VERIFY(exp) \
  ((!(exp)) ? \
    (RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE) : TRUE)

#define RTL_VERIFYMSG(msg, exp) \
  ((!(exp)) ? \
    (RtlAssert( (PVOID)#exp, (PVOID)__FILE__, __LINE__, (PCHAR)msg ), FALSE) : TRUE)

/* The ASSERTs must be cast to void to avoid warnings about unused results. */
#define ASSERT                 (void)RTL_VERIFY
#define ASSERTMSG              (void)RTL_VERIFYMSG

#else /* !DBG */

#define RTL_VERIFY(exp)              ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG(msg, exp)      ((exp) ? TRUE : FALSE)

#define ASSERT(exp)                  ((void)0)
#define ASSERTMSG(msg, exp)          ((void)0)

#endif /* DBG */

#endif /* ASSERT */

//
// RTL_PAGED_CODE
//

#ifdef NTOS_KERNEL_RUNTIME

//
// Executing RTL functions at DISPATCH_LEVEL or higher will result in a
// bugcheck.
//
#define RTL_PAGED_CODE PAGED_CODE

#else

//
// This macro does nothing in user mode
//
#define RTL_PAGED_CODE()

#endif

//
// Deprecated LARGE_INTEGER routines
//

#if RTLFUNCS_COMPILER_PREPROCESSOR

#define RtlLargeIntegerGreaterThan(X,Y) (                              \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                      \
)

#define RtlLargeIntegerGreaterThanOrEqualTo(X,Y) (                      \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                       \
)

#define RtlLargeIntegerEqualTo(X,Y) (                              \
    !(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define RtlLargeIntegerNotEqualTo(X,Y) (                          \
    (((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define RtlLargeIntegerLessThan(X,Y) (                                 \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                      \
)

#define RtlLargeIntegerLessThanOrEqualTo(X,Y) (                         \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                       \
)

#define RtlLargeIntegerGreaterThanZero(X) (       \
    (((X).HighPart == 0) && ((X).LowPart > 0)) || \
    ((X).HighPart > 0 )                           \
)

#define RtlLargeIntegerGreaterOrEqualToZero(X) ( \
    (X).HighPart >= 0                            \
)

#define RtlLargeIntegerEqualToZero(X) ( \
    !((X).LowPart | (X).HighPart)       \
)

#define RtlLargeIntegerNotEqualToZero(X) ( \
    ((X).LowPart | (X).HighPart)           \
)

#define RtlLargeIntegerLessThanZero(X) ( \
    ((X).HighPart < 0)                   \
)

#define RtlLargeIntegerLessOrEqualToZero(X) (           \
    ((X).HighPart < 0) || !((X).LowPart | (X).HighPart) \
)

#endif /* RTLFUNCS_COMPILER_PREPROCESSOR */

//
// ANSI/OEM/Unicode string routines
//

#define RtlUnicodeStringToOemSize(STRING) (                   \
    NLS_MB_OEM_CODE_PAGE_TAG ?                                \
    RtlxUnicodeStringToOemSize(STRING) :                      \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

#define RtlUnicodeStringToCountedOemSize(STRING) (                      \
    (ULONG)(RtlUnicodeStringToOemSize(STRING) - sizeof(ANSI_NULL))      \
)

#define RtlOemStringToUnicodeSize(STRING) (                 \
    NLS_MB_OEM_CODE_PAGE_TAG ?                              \
    RtlxOemStringToUnicodeSize(STRING) :                    \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)  \
)

#define RtlOemStringToCountedUnicodeSize(STRING) (                    \
    (ULONG)(RtlOemStringToUnicodeSize(STRING) - sizeof(UNICODE_NULL)) \
)

#define RtlAnsiStringToUnicodeSize(STRING) (                        \
    NLS_MB_CODE_PAGE_TAG ?                                          \
    RtlxAnsiStringToUnicodeSize(STRING) :                           \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)          \
)

#define RtlUnicodeStringToAnsiSize(STRING) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(STRING) :                     \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

#ifdef _WIN64
#define RtlIntPtrToUnicodeString(Value, Base, String) \
    RtlInt64ToUnicodeString(Value, Base, String)
#else
#define RtlIntPtrToUnicodeString(Value, Base, String) \
    RtlIntegerToUnicodeString(Value, Base, String)
#endif

//
// Unsigned unaligned integral values read/write routines
//

#define SHORT_SIZE     (sizeof(USHORT))
#define SHORT_MASK     (SHORT_SIZE - 1)
#define LONG_SIZE      (sizeof(LONG))
#define LONGLONG_SIZE  (sizeof(LONGLONG))
#define LONG_MASK      (LONG_SIZE - 1)
#define LONGLONG_MASK  (LONGLONG_SIZE - 1)
#define LOWBYTE_MASK   0x00FF

#define FIRSTBYTE(VALUE)  ((VALUE) & LOWBYTE_MASK)
#define SECONDBYTE(VALUE) (((VALUE) >> 8) & LOWBYTE_MASK)
#define THIRDBYTE(VALUE)  (((VALUE) >> 16) & LOWBYTE_MASK)
#define FOURTHBYTE(VALUE) (((VALUE) >> 24) & LOWBYTE_MASK)

#define SHORT_LEAST_SIGNIFICANT_BIT   0
#define SHORT_MOST_SIGNIFICANT_BIT    1

#define LONG_LEAST_SIGNIFICANT_BIT    0
#define LONG_3RD_MOST_SIGNIFICANT_BIT 1
#define LONG_2ND_MOST_SIGNIFICANT_BIT 2
#define LONG_MOST_SIGNIFICANT_BIT     3

#if defined(_AMD64_)

/* void
 * RtlStoreUlong(
 *     IN PULONG Address,
 *     IN ULONG Value);
 */
#define RtlStoreUlong(Address,Value) \
    *(ULONG UNALIGNED *)(Address) = (Value)

/* void
 * RtlStoreUlonglong(
 *     IN OUT PULONGLONG Address,
 *     ULONGLONG Value);
 */
#define RtlStoreUlonglong(Address,Value) \
    *(ULONGLONG UNALIGNED *)(Address) = (Value)

/* void
 * RtlStoreUshort(
 *     IN PUSHORT Address,
 *     IN USHORT Value);
 */
#define RtlStoreUshort(Address,Value) \
    *(USHORT UNALIGNED *)(Address) = (Value)

/* void
 * RtlRetrieveUshort(
 *     PUSHORT DestinationAddress,
 *    PUSHORT SourceAddress);
 */
#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    *(USHORT UNALIGNED *)(DestAddress) = *(USHORT)(SrcAddress)

/* void
 * RtlRetrieveUlong(
 *    PULONG DestinationAddress,
 *    PULONG SourceAddress);
 */
#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    *(ULONG UNALIGNED *)(DestAddress) = *(PULONG)(SrcAddress)

#else

#define RtlStoreUlong(Address,Value)                      \
    if ((ULONG_PTR)(Address) & LONG_MASK) { \
        ((PUCHAR) (Address))[LONG_LEAST_SIGNIFICANT_BIT]    = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_3RD_MOST_SIGNIFICANT_BIT] = (UCHAR)(SECONDBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_2ND_MOST_SIGNIFICANT_BIT] = (UCHAR)(THIRDBYTE(Value)); \
        ((PUCHAR) (Address))[LONG_MOST_SIGNIFICANT_BIT]     = (UCHAR)(FOURTHBYTE(Value)); \
    } \
    else { \
        *((PULONG)(Address)) = (ULONG) (Value); \
    }

#define RtlStoreUlonglong(Address,Value) \
    if ((ULONG_PTR)(Address) & LONGLONG_MASK) { \
        RtlStoreUlong((ULONG_PTR)(Address), \
                      (ULONGLONG)(Value) & 0xFFFFFFFF); \
        RtlStoreUlong((ULONG_PTR)(Address)+sizeof(ULONG), \
                      (ULONGLONG)(Value) >> 32); \
    } else { \
        *((PULONGLONG)(Address)) = (ULONGLONG)(Value); \
    }

#define RtlStoreUshort(Address,Value) \
    if ((ULONG_PTR)(Address) & SHORT_MASK) { \
        ((PUCHAR) (Address))[SHORT_LEAST_SIGNIFICANT_BIT] = (UCHAR)(FIRSTBYTE(Value)); \
        ((PUCHAR) (Address))[SHORT_MOST_SIGNIFICANT_BIT ] = (UCHAR)(SECONDBYTE(Value)); \
    } \
    else { \
        *((PUSHORT) (Address)) = (USHORT)Value; \
    }

#define RtlRetrieveUshort(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
    { \
        ((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
        ((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
    } \
    else \
    { \
        *((PUSHORT)(DestAddress))=*((PUSHORT)(SrcAddress)); \
    }

#define RtlRetrieveUlong(DestAddress,SrcAddress) \
    if ((ULONG_PTR)(SrcAddress) & LONG_MASK) \
    { \
        ((PUCHAR)(DestAddress))[0]=((PUCHAR)(SrcAddress))[0]; \
        ((PUCHAR)(DestAddress))[1]=((PUCHAR)(SrcAddress))[1]; \
        ((PUCHAR)(DestAddress))[2]=((PUCHAR)(SrcAddress))[2]; \
        ((PUCHAR)(DestAddress))[3]=((PUCHAR)(SrcAddress))[3]; \
    } \
    else \
    { \
        *((PULONG)(DestAddress))=*((PULONG)(SrcAddress)); \
    }

#endif /* defined(_AMD64_) */

#ifdef _WIN64
/* void
 * RtlStoreUlongPtr(
 *     IN OUT PULONG_PTR Address,
 *     IN ULONG_PTR Value);
 */
#define RtlStoreUlongPtr(Address,Value) RtlStoreUlonglong(Address,Value)
#else
#define RtlStoreUlongPtr(Address,Value) RtlStoreUlong(Address,Value)
#endif /* _WIN64 */

//
// Bit fiddling
//

#define RtlInterlockedSetBits(Flags, Flag) \
    InterlockedOr((PLONG)(Flags), Flag)

#define RtlInterlockedAndBits(Flags, Flag) \
    InterlockedAnd((PLONG)(Flags), Flag)

#define RtlInterlockedClearBits(Flags, Flag) \
    RtlInterlockedAndBits(Flags, ~(Flag))

#define RtlInterlockedXorBits(Flags, Flag) \
    InterlockedXor(Flags, Flag)

#define RtlInterlockedSetBitsDiscardReturn(Flags, Flag) \
    (void) RtlInterlockedSetBits(Flags, Flag)

#define RtlInterlockedAndBitsDiscardReturn(Flags, Flag) \
    (void) RtlInterlockedAndBits(Flags, Flag)

#define RtlInterlockedClearBitsDiscardReturn(Flags, Flag) \
    RtlInterlockedAndBitsDiscardReturn(Flags, ~(Flag))


// TODO: Wine checks SizeOfBitMap here

#if defined(_M_AMD64) && RTLFUNCS_COMPILER_PREPROCESSOR
_Must_inspect_result_
FORCEINLINE
BOOLEAN
RtlCheckBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitPosition)
{
    return BitTest64((LONG64 CONST*)BitMapHeader->Buffer, (LONG64)BitPosition);
}
#else
#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP)/32]) >> ((BP)%32)) & 0x1)
#endif /* defined(_M_AMD64) */

//
// Memory routines
//

#define RtlCopyBytes RtlCopyMemory
#define RtlFillBytes RtlFillMemory
#define RtlZeroBytes RtlZeroMemory

#if RTLFUNCS_COMPILER_PREPROCESSOR

FORCEINLINE
PVOID
RtlSecureZeroMemory(
    _Out_writes_bytes_all_(Size) PVOID Pointer,
    _In_ SIZE_T Size
)
{
    /* Get a volatile pointer to prevent most compiler optimizations */
    volatile char* vptr = (volatile char*)Pointer;

#if defined(_M_AMD64)
    __stosb((PUCHAR)((ULONG64)vptr), 0, Size);
#else
    /* Loop the whole buffer */
    while (Size)
    {
        /* Zero the current byte */
#if !defined(_M_CEE) && (defined(_M_ARM) || defined(_M_ARM64))
        __iso_volatile_store8(vptr, 0);
#else
        *vptr = 0;
#endif

        /* Move to the next byte */
        vptr++;
        Size--;
    }
#endif

    /* Return the pointer to ensure the compiler won't optimize this away */
    return Pointer;
}

#endif /* RTLFUNCS_COMPILER_PREPROCESSOR */

//
// CONTEXT and CONTEXT_EX helper routines
//

#define RTL_CONTEXT_EX_OFFSET(ContextEx, Chunk) ((ContextEx)->Chunk.Offset)
#define RTL_CONTEXT_EX_LENGTH(ContextEx, Chunk) ((ContextEx)->Chunk.Length)
#define RTL_CONTEXT_EX_CHUNK(Base, Layout, Chunk)       \
    ((PVOID)((PCHAR)(Base) + RTL_CONTEXT_EX_OFFSET(Layout, Chunk)))
#define RTL_CONTEXT_OFFSET(Context, Chunk)              \
    RTL_CONTEXT_EX_OFFSET((PCONTEXT_EX)(Context + 1), Chunk)
#define RTL_CONTEXT_LENGTH(Context, Chunk)              \
    RTL_CONTEXT_EX_LENGTH((PCONTEXT_EX)(Context + 1), Chunk)
#define RTL_CONTEXT_CHUNK(Context, Chunk)               \
    RTL_CONTEXT_EX_CHUNK((PCONTEXT_EX)(Context + 1),    \
                         (PCONTEXT_EX)(Context + 1),    \
                         Chunk)

//
// Pointer arithmetics
//

#define RtlOffsetToPointer(B,O) ((PCHAR)(((PCHAR)(B)) + ((ULONG_PTR)(O))))
#define RtlPointerToOffset(B,P) ((ULONG)(((PCHAR)(P)) - ((PCHAR)(B))))

//
// Splay link tree routines
//

#define RtlIsLeftChild(Links) \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlIsRightChild(Links) \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links))

#define RtlRightChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->RightChild

#define RtlIsRoot(Links) \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links))

#define RtlLeftChild(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->LeftChild

#define RtlParent(Links) \
    ((PRTL_SPLAY_LINKS)(Links))->Parent

// FIXME: use inline functions instead of the next 3 macros
//        (will need wrapping with RTLFUNCS_COMPILER_PREPROCESSOR guard)

#define RtlInitializeSplayLinks(Links)                  \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayLinks;                   \
        _SplayLinks = (PRTL_SPLAY_LINKS)(Links);        \
        _SplayLinks->Parent = _SplayLinks;              \
        _SplayLinks->LeftChild = NULL;                  \
        _SplayLinks->RightChild = NULL;                 \
    }

#define RtlInsertAsLeftChild(ParentLinks,ChildLinks)    \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->LeftChild = _SplayChild;          \
        _SplayChild->Parent = _SplayParent;             \
    }

#define RtlInsertAsRightChild(ParentLinks,ChildLinks)   \
    {                                                   \
        PRTL_SPLAY_LINKS _SplayParent;                  \
        PRTL_SPLAY_LINKS _SplayChild;                   \
        _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
        _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks);   \
        _SplayParent->RightChild = _SplayChild;         \
        _SplayChild->Parent = _SplayParent;             \
    }

#ifdef __cplusplus
}
#endif

#endif
