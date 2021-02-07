#include <rtl.h>

#define NDEBUG
#include <debug.h>

C_ASSERT(RTL_FLS_MAXIMUM_AVAILABLE == FLS_MAXIMUM_AVAILABLE);

typedef struct _RTLP_FLS_CONTEXT
{
    RTL_SRWLOCK Lock;
    PFLS_CALLBACK_FUNCTION* FlsCallback;
    LIST_ENTRY FlsListHead;
    RTL_BITMAP FlsBitmap;
    ULONG FlsBitmapBits[4];
    ULONG FlsHighIndex;
} RTLP_FLS_CONTEXT, *PRTLP_FLS_CONTEXT;

typedef struct _RTLP_FLS_DATA
{
    LIST_ENTRY ListEntry;
    PVOID Data[FLS_MAXIMUM_AVAILABLE];
} RTLP_FLS_DATA, *PRTLP_FLS_DATA;

RTLP_FLS_CONTEXT RtlpFlsContext = { RTL_SRWLOCK_INIT, 0 };

void
NTAPI
RtlpFlsInitialize(void)
{
    /* Initialize FLS Bitmap */
    RtlInitializeBitMap(&RtlpFlsContext.FlsBitmap,
                        RtlpFlsContext.FlsBitmapBits,
                        FLS_MAXIMUM_AVAILABLE);
    RtlSetBit(&RtlpFlsContext.FlsBitmap, 0);
    InitializeListHead(&RtlpFlsContext.FlsListHead);
}

void
NTAPI
RtlpFlsDataCleanup(IN PRTLP_FLS_DATA FlsData, IN RTLP_FLS_DATA_CLEANUP_MODE Mode)
{
    if (Mode & RTLP_FLS_DATA_CLEANUP_RUN_CALLBACKS)
    {
        ULONG FlsHighIndex;

        RtlAcquireSRWLockExclusive(&RtlpFlsContext.Lock);

        _SEH2_TRY
        {
            FlsHighIndex = RtlpFlsContext.FlsHighIndex;
            RemoveEntryList(&FlsData->ListEntry);
        }
        _SEH2_FINALLY
        {
            RtlReleaseSRWLockExclusive(&RtlpFlsContext.Lock);
        }
        _SEH2_END;

        ASSERT(RtlpFlsContext.FlsCallback);

        for (ULONG n = 1; n <= FlsHighIndex; ++n)
        {
            PFLS_CALLBACK_FUNCTION* pCallback = &RtlpFlsContext.FlsCallback[n];
            PVOID pData;

            if (!*pCallback)
                continue;
            
            pData = FlsData->Data[n];

            if (!pData)
                continue;

            /* todo: lock a callback array item (SRW Shared) */

            /*
             * N.B. Should an exception stop iteration, or just ignore current one?
             *      Not clear.
             */

            _SEH2_TRY
            {
                if (!*pCallback)
                    _SEH2_LEAVE;
                (*pCallback)(pData);
                *pCallback = NULL;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                DbgPrint("FLS callback [0x%p](0x%p) failed [0x%08lX]\n", *pCallback, pData, _SEH2_GetExceptionCode());
            }
            _SEH2_END;
        }
    }

    if (Mode & RTLP_FLS_DATA_CLEANUP_DEALLOCATE)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FlsData);
    }
}

void
NTAPI
RtlProcessFlsData(IN PVOID FlsData, IN RTLP_FLS_DATA_CLEANUP_MODE Mode)
{
    RtlpFlsDataCleanup(FlsData, Mode);
}

NTSTATUS
NTAPI
RtlFlsAlloc(PFLS_CALLBACK_FUNCTION pCallback, ULONG32* pFlsIndex)
{
    ULONG32 FlsIndex;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlAcquireSRWLockExclusive(&RtlpFlsContext.Lock);

    FlsIndex = RtlFindClearBitsAndSet(&RtlpFlsContext.FlsBitmap, 1, 1);
    if (FlsIndex != FLS_OUT_OF_INDEXES)
    {
        if (!RtlpFlsContext.FlsCallback)
        {
            RtlpFlsContext.FlsCallback = RtlAllocateHeap(RtlGetProcessHeap(),
                                                         HEAP_ZERO_MEMORY,
                                                         FLS_MAXIMUM_AVAILABLE * sizeof(PVOID));

            if (!RtlpFlsContext.FlsCallback)
            {
                RtlClearBits(&RtlpFlsContext.FlsBitmap, FlsIndex, 1);
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }
        }

        RtlpFlsContext.FlsCallback[FlsIndex] = pCallback;

        if (FlsIndex > RtlpFlsContext.FlsHighIndex)
            RtlpFlsContext.FlsHighIndex = FlsIndex;
    }

Exit:

    RtlReleaseSRWLockExclusive(&RtlpFlsContext.Lock);

    *pFlsIndex = NT_SUCCESS(Status) ? FlsIndex : FLS_OUT_OF_INDEXES;

    return Status;
}


NTSTATUS
NTAPI
RtlFlsFree(ULONG32 FlsIndex)
{
    NTSTATUS Status;

    if (FlsIndex >= FLS_MAXIMUM_AVAILABLE)
        return STATUS_INVALID_PARAMETER;

    RtlAcquireSRWLockExclusive(&RtlpFlsContext.Lock);

    if (RtlAreBitsSet(&RtlpFlsContext.FlsBitmap, FlsIndex, 1))
    {
        PLIST_ENTRY Entry;
        PFLS_CALLBACK_FUNCTION pCallback;

        Status = STATUS_SUCCESS;

        ASSERT(RtlpFlsContext.FlsCallback);

        RtlClearBits(&RtlpFlsContext.FlsBitmap, FlsIndex, 1);
        pCallback = RtlpFlsContext.FlsCallback[FlsIndex];

        for (Entry = RtlpFlsContext.FlsListHead.Flink; Entry != &RtlpFlsContext.FlsListHead; Entry = Entry->Flink)
        {
            PRTLP_FLS_DATA pFlsData = CONTAINING_RECORD(Entry, RTLP_FLS_DATA, ListEntry);
            PVOID pData = pFlsData->Data[FlsIndex];

            if (!pData)
                continue;

            if (pCallback)
            {
                _SEH2_TRY
                {
                    pCallback(pData);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    DbgPrint("FLS callback [0x%p](0x%p) failed [0x%08lX]\n", *pCallback, pData, _SEH2_GetExceptionCode());
                }
                _SEH2_END;
            }

            pFlsData->Data[FlsIndex] = NULL;
        }

        RtlpFlsContext.FlsCallback[FlsIndex] = NULL;
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    RtlReleaseSRWLockExclusive(&RtlpFlsContext.Lock);

    return Status;
}


NTSTATUS
NTAPI
RtlFlsGetValue(ULONG32 FlsIndex, PVOID* pFlsValue)
{
    /*
     * N.B. Windows 10 21H1 introduces kernel-mode FLS (=TLS).
     *      Much like TEB.FlsData, it is stored in ETHREAD.FlsData.
     *      ReactOS doesn't support kmode FLS for now.
     */

    PRTLP_FLS_DATA pFlsData = NtCurrentTeb()->FlsData;

    if (!FlsIndex || FlsIndex >= FLS_MAXIMUM_AVAILABLE || !pFlsData)
    {
        *pFlsValue = NULL;

        return STATUS_INVALID_PARAMETER;
    }

    *pFlsValue = pFlsData->Data[FlsIndex];

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlFlsSetValue(ULONG32 FlsIndex, PVOID FlsValue)
{
    PRTLP_FLS_DATA pFlsData;

    if (!FlsIndex || FlsIndex >= FLS_MAXIMUM_AVAILABLE)
        return STATUS_INVALID_PARAMETER;

    pFlsData = NtCurrentTeb()->FlsData;

    if (!pFlsData)
    {
        pFlsData = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   sizeof(RTLP_FLS_DATA));

        if (!pFlsData)
        {
            return STATUS_NO_MEMORY;
        }

        NtCurrentTeb()->FlsData = pFlsData;

        RtlAcquireSRWLockExclusive(&RtlpFlsContext.Lock);

        InsertTailList(&RtlpFlsContext.FlsListHead, &pFlsData->ListEntry);

        RtlReleaseSRWLockExclusive(&RtlpFlsContext.Lock);
    }

    if (!RtlpFlsContext.FlsCallback)
    {
        RtlAcquireSRWLockExclusive(&RtlpFlsContext.Lock);

        RtlpFlsContext.FlsCallback = RtlAllocateHeap(RtlGetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     FLS_MAXIMUM_AVAILABLE * sizeof(PVOID));

        RtlReleaseSRWLockExclusive(&RtlpFlsContext.Lock);

        if (!RtlpFlsContext.FlsCallback)
        {
            return STATUS_NO_MEMORY;
        }
    }

    pFlsData->Data[FlsIndex] = FlsValue;

    return STATUS_SUCCESS;
}
