#include <ldrp.h>

#include <ndk/obfuncs.h>

void
NTAPI
LdrpDrainWorkQueue(IN LDRP_DRAIN_WORK_QUEUE_TARGET Target)
{
    DPRINT1("LdrpDrainWorkQueue(%d)\n", Target);

    HANDLE TargetEvent;

    switch (Target)
    {
        case LoadComplete:
            TargetEvent = LdrpLoadCompleteEvent;
            break;

        case WorkComplete:
            TargetEvent = LdrpWorkCompleteEvent;
            break;

        DEFAULT_UNREACHABLE;
    }

    while (TRUE)
    {
        while (TRUE)
        {
            LDRP_PROCESS_WORK_MODE Mode = QueuedWork;
            BOOLEAN DrainedWorkQueue = FALSE;

            RtlEnterCriticalSection(&LdrpWorkQueueLock);

            PLIST_ENTRY QueuedEntry = NULL;

            if (IsListEmpty(&LdrpWorkQueue))
            {
                if (LdrpWorkInProgress == (Target == WorkComplete ? 1 : 0))
                {
                    LdrpWorkInProgress = 1;
                    DrainedWorkQueue = TRUE;
                }
            }
            else
            {
                QueuedEntry = RemoveHeadList(&LdrpWorkQueue);

                if (Mode == QueuedWork)
                    ++LdrpWorkInProgress;
            }

            RtlLeaveCriticalSection(&LdrpWorkQueueLock);

            if (DrainedWorkQueue)
                break;

            if (QueuedEntry)
            {
                DPRINT1("LdrpDrainWorkQueue(%d): calling LdrpProcessWork\n", Target);

                PLDRP_LOAD_CONTEXT QueuedContext = CONTAINING_RECORD(QueuedEntry, LDRP_LOAD_CONTEXT, QueueListEntry);

                LdrpProcessWork(QueuedContext, Mode);
            }
            else
            {
                DPRINT1("LdrpDrainWorkQueue(%d): Entering wait on TargetEvent\n", Target);
                NtWaitForSingleObject(TargetEvent, FALSE, NULL);
            }
        }

        if (Target == LoadComplete || IsListEmpty(&LdrpRetryQueue))
            break;

        DPRINT1("LdrpDrainWorkQueue(%d): LdrpRetryQueue is non-empty\n", Target);

        RtlEnterCriticalSection(&LdrpWorkQueueLock);

        AppendTailList(&LdrpWorkQueue, &LdrpRetryQueue);

        RtlLeaveCriticalSection(&LdrpWorkQueueLock);
    }

    NtCurrentTeb()->LoadOwner = TRUE;

    ASSERT(LdrpWorkInProgress == 1);
    ASSERT(IsListEmpty(&LdrpWorkQueue));
}
