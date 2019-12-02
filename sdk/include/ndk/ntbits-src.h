#pragma once

// ==================================================

FORCEINLINE BOOLEAN KiTestThreadSpare1Flag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 0)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadSpare1Flag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 0);
}

FORCEINLINE void KiSetThreadSpare1FlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 0));
}

FORCEINLINE BOOLEAN KiClearThreadSpare1Flag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 0);
}

FORCEINLINE void KiClearThreadSpare1FlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 0));
}

FORCEINLINE BOOLEAN KiAssignThreadSpare1Flag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 0) : InterlockedBitTestAndReset(&o->ThreadFlags, 0);
}

FORCEINLINE BOOLEAN KiTestThreadAutoAlignmentFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 1)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadAutoAlignmentFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 1);
}

FORCEINLINE void KiSetThreadAutoAlignmentFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 1));
}

FORCEINLINE BOOLEAN KiClearThreadAutoAlignmentFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 1);
}

FORCEINLINE void KiClearThreadAutoAlignmentFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 1));
}

FORCEINLINE BOOLEAN KiAssignThreadAutoAlignmentFlag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 1) : InterlockedBitTestAndReset(&o->ThreadFlags, 1);
}

FORCEINLINE BOOLEAN KiTestThreadDisableBoostFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 2)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadDisableBoostFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 2);
}

FORCEINLINE void KiSetThreadDisableBoostFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 2));
}

FORCEINLINE BOOLEAN KiClearThreadDisableBoostFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 2);
}

FORCEINLINE void KiClearThreadDisableBoostFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 2));
}

FORCEINLINE BOOLEAN KiAssignThreadDisableBoostFlag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 2) : InterlockedBitTestAndReset(&o->ThreadFlags, 2);
}

FORCEINLINE BOOLEAN KiTestThreadForceDeferScheduleFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 3)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadForceDeferScheduleFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 3);
}

FORCEINLINE void KiSetThreadForceDeferScheduleFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 3));
}

FORCEINLINE BOOLEAN KiClearThreadForceDeferScheduleFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 3);
}

FORCEINLINE void KiClearThreadForceDeferScheduleFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 3));
}

FORCEINLINE BOOLEAN KiAssignThreadForceDeferScheduleFlag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 3) : InterlockedBitTestAndReset(&o->ThreadFlags, 3);
}

FORCEINLINE BOOLEAN KiTestThreadAlertedByThreadIdFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 4)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadAlertedByThreadIdFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 4);
}

FORCEINLINE void KiSetThreadAlertedByThreadIdFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 4));
}

FORCEINLINE BOOLEAN KiClearThreadAlertedByThreadIdFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 4);
}

FORCEINLINE void KiClearThreadAlertedByThreadIdFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 4));
}

FORCEINLINE BOOLEAN KiAssignThreadAlertedByThreadIdFlag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 4) : InterlockedBitTestAndReset(&o->ThreadFlags, 4);
}

FORCEINLINE BOOLEAN KiTestThreadQuantumDonationFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 5)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadQuantumDonationFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 5);
}

FORCEINLINE void KiSetThreadQuantumDonationFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 5));
}

FORCEINLINE BOOLEAN KiClearThreadQuantumDonationFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 5);
}

FORCEINLINE void KiClearThreadQuantumDonationFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 5));
}

FORCEINLINE BOOLEAN KiAssignThreadQuantumDonationFlag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 5) : InterlockedBitTestAndReset(&o->ThreadFlags, 5);
}

FORCEINLINE BOOLEAN KiTestThreadSystemThreadFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return ((o->ThreadFlags) & (1 << 6)) != 0;
}

FORCEINLINE BOOLEAN KiSetThreadSystemThreadFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndSet(&o->ThreadFlags, 6);
}

FORCEINLINE void KiSetThreadSystemThreadFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ThreadFlags, 6));
}

FORCEINLINE BOOLEAN KiClearThreadSystemThreadFlag(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return InterlockedBitTestAndReset(&o->ThreadFlags, 6);
}

FORCEINLINE void KiClearThreadSystemThreadFlagAssert(KTHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    ASSERT(InterlockedBitTestAndReset(&o->ThreadFlags, 6));
}

FORCEINLINE BOOLEAN KiAssignThreadSystemThreadFlag(KTHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(o);
    return value ? InterlockedBitTestAndSet(&o->ThreadFlags, 6) : InterlockedBitTestAndReset(&o->ThreadFlags, 6);
}

// ==================================================

FORCEINLINE BOOLEAN KiTestProcessAutoAlignmentFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return ((o->ProcessFlags) & (1 << 0)) != 0;
}

FORCEINLINE BOOLEAN KiSetProcessAutoAlignmentFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndSet(&o->ProcessFlags, 0);
}

FORCEINLINE void KiSetProcessAutoAlignmentFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ProcessFlags, 0));
}

FORCEINLINE BOOLEAN KiClearProcessAutoAlignmentFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndReset(&o->ProcessFlags, 0);
}

FORCEINLINE void KiClearProcessAutoAlignmentFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(InterlockedBitTestAndReset(&o->ProcessFlags, 0));
}

FORCEINLINE BOOLEAN KiAssignProcessAutoAlignmentFlag(KPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return value ? InterlockedBitTestAndSet(&o->ProcessFlags, 0) : InterlockedBitTestAndReset(&o->ProcessFlags, 0);
}

FORCEINLINE BOOLEAN KiTestProcessDisableBoostFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return ((o->ProcessFlags) & (1 << 1)) != 0;
}

FORCEINLINE BOOLEAN KiSetProcessDisableBoostFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndSet(&o->ProcessFlags, 1);
}

FORCEINLINE void KiSetProcessDisableBoostFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ProcessFlags, 1));
}

FORCEINLINE BOOLEAN KiClearProcessDisableBoostFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndReset(&o->ProcessFlags, 1);
}

FORCEINLINE void KiClearProcessDisableBoostFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(InterlockedBitTestAndReset(&o->ProcessFlags, 1));
}

FORCEINLINE BOOLEAN KiAssignProcessDisableBoostFlag(KPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return value ? InterlockedBitTestAndSet(&o->ProcessFlags, 1) : InterlockedBitTestAndReset(&o->ProcessFlags, 1);
}

FORCEINLINE BOOLEAN KiTestProcessDisableQuantumFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return ((o->ProcessFlags) & (1 << 2)) != 0;
}

FORCEINLINE BOOLEAN KiSetProcessDisableQuantumFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndSet(&o->ProcessFlags, 2);
}

FORCEINLINE void KiSetProcessDisableQuantumFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ProcessFlags, 2));
}

FORCEINLINE BOOLEAN KiClearProcessDisableQuantumFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndReset(&o->ProcessFlags, 2);
}

FORCEINLINE void KiClearProcessDisableQuantumFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(InterlockedBitTestAndReset(&o->ProcessFlags, 2));
}

FORCEINLINE BOOLEAN KiAssignProcessDisableQuantumFlag(KPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return value ? InterlockedBitTestAndSet(&o->ProcessFlags, 2) : InterlockedBitTestAndReset(&o->ProcessFlags, 2);
}

FORCEINLINE BOOLEAN KiTestProcessDeepFreezeFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return ((o->ProcessFlags) & (1 << 3)) != 0;
}

FORCEINLINE BOOLEAN KiSetProcessDeepFreezeFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndSet(&o->ProcessFlags, 3);
}

FORCEINLINE void KiSetProcessDeepFreezeFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(!InterlockedBitTestAndSet(&o->ProcessFlags, 3));
}

FORCEINLINE BOOLEAN KiClearProcessDeepFreezeFlag(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return InterlockedBitTestAndReset(&o->ProcessFlags, 3);
}

FORCEINLINE void KiClearProcessDeepFreezeFlagAssert(KPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    ASSERT(InterlockedBitTestAndReset(&o->ProcessFlags, 3));
}

FORCEINLINE BOOLEAN KiAssignProcessDeepFreezeFlag(KPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(o);
    return value ? InterlockedBitTestAndSet(&o->ProcessFlags, 3) : InterlockedBitTestAndReset(&o->ProcessFlags, 3);
}

// ==================================================

FORCEINLINE BOOLEAN PspTestThreadTerminatedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 0)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadTerminatedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 0);
}

FORCEINLINE void PspSetThreadTerminatedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 0));
}

FORCEINLINE BOOLEAN PspClearThreadTerminatedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 0);
}

FORCEINLINE void PspClearThreadTerminatedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 0));
}

FORCEINLINE BOOLEAN PspAssignThreadTerminatedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 0) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 0);
}

FORCEINLINE BOOLEAN PspTestThreadInsertedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 1)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadInsertedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 1);
}

FORCEINLINE void PspSetThreadInsertedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 1));
}

FORCEINLINE BOOLEAN PspClearThreadInsertedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 1);
}

FORCEINLINE void PspClearThreadInsertedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 1));
}

FORCEINLINE BOOLEAN PspAssignThreadInsertedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 1) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 1);
}

FORCEINLINE BOOLEAN PspTestThreadHideFromDebuggerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 2)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadHideFromDebuggerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 2);
}

FORCEINLINE void PspSetThreadHideFromDebuggerFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 2));
}

FORCEINLINE BOOLEAN PspClearThreadHideFromDebuggerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 2);
}

FORCEINLINE void PspClearThreadHideFromDebuggerFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 2));
}

FORCEINLINE BOOLEAN PspAssignThreadHideFromDebuggerFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 2) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 2);
}

FORCEINLINE BOOLEAN PspTestThreadActiveImpersonationInfoFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 3)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadActiveImpersonationInfoFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 3);
}

FORCEINLINE void PspSetThreadActiveImpersonationInfoFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 3));
}

FORCEINLINE BOOLEAN PspClearThreadActiveImpersonationInfoFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 3);
}

FORCEINLINE void PspClearThreadActiveImpersonationInfoFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 3));
}

FORCEINLINE BOOLEAN PspAssignThreadActiveImpersonationInfoFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 3) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 3);
}

FORCEINLINE BOOLEAN PspTestThreadHardErrorsAreDisabledFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 4)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadHardErrorsAreDisabledFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 4);
}

FORCEINLINE void PspSetThreadHardErrorsAreDisabledFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 4));
}

FORCEINLINE BOOLEAN PspClearThreadHardErrorsAreDisabledFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 4);
}

FORCEINLINE void PspClearThreadHardErrorsAreDisabledFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 4));
}

FORCEINLINE BOOLEAN PspAssignThreadHardErrorsAreDisabledFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 4) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 4);
}

FORCEINLINE BOOLEAN PspTestThreadBreakOnTerminationFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 5)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadBreakOnTerminationFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 5);
}

FORCEINLINE void PspSetThreadBreakOnTerminationFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 5));
}

FORCEINLINE BOOLEAN PspClearThreadBreakOnTerminationFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 5);
}

FORCEINLINE void PspClearThreadBreakOnTerminationFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 5));
}

FORCEINLINE BOOLEAN PspAssignThreadBreakOnTerminationFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 5) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 5);
}

FORCEINLINE BOOLEAN PspTestThreadSkipCreationMsgFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 6)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadSkipCreationMsgFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 6);
}

FORCEINLINE void PspSetThreadSkipCreationMsgFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 6));
}

FORCEINLINE BOOLEAN PspClearThreadSkipCreationMsgFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 6);
}

FORCEINLINE void PspClearThreadSkipCreationMsgFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 6));
}

FORCEINLINE BOOLEAN PspAssignThreadSkipCreationMsgFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 6) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 6);
}

FORCEINLINE BOOLEAN PspTestThreadSkipTerminationMsgFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->CrossThreadFlags) & (1 << 7)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadSkipTerminationMsgFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->CrossThreadFlags, 7);
}

FORCEINLINE void PspSetThreadSkipTerminationMsgFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->CrossThreadFlags, 7));
}

FORCEINLINE BOOLEAN PspClearThreadSkipTerminationMsgFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->CrossThreadFlags, 7);
}

FORCEINLINE void PspClearThreadSkipTerminationMsgFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->CrossThreadFlags, 7));
}

FORCEINLINE BOOLEAN PspAssignThreadSkipTerminationMsgFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->CrossThreadFlags, 7) : InterlockedBitTestAndReset(&o->CrossThreadFlags, 7);
}

// ==================================================

FORCEINLINE BOOLEAN PspTestThreadActiveExWorkerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadPassiveFlags) & (1 << 0)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadActiveExWorkerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 0);
}

FORCEINLINE void PspSetThreadActiveExWorkerFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 0));
}

FORCEINLINE BOOLEAN PspClearThreadActiveExWorkerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 0);
}

FORCEINLINE void PspClearThreadActiveExWorkerFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 0));
}

FORCEINLINE BOOLEAN PspAssignThreadActiveExWorkerFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 0) : InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 0);
}

FORCEINLINE BOOLEAN PspTestThreadExWorkerCanWaitUserFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadPassiveFlags) & (1 << 1)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadExWorkerCanWaitUserFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 1);
}

FORCEINLINE void PspSetThreadExWorkerCanWaitUserFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 1));
}

FORCEINLINE BOOLEAN PspClearThreadExWorkerCanWaitUserFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 1);
}

FORCEINLINE void PspClearThreadExWorkerCanWaitUserFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 1));
}

FORCEINLINE BOOLEAN PspAssignThreadExWorkerCanWaitUserFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 1) : InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 1);
}

FORCEINLINE BOOLEAN PspTestThreadMemoryMakerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadPassiveFlags) & (1 << 2)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadMemoryMakerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 2);
}

FORCEINLINE void PspSetThreadMemoryMakerFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 2));
}

FORCEINLINE BOOLEAN PspClearThreadMemoryMakerFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 2);
}

FORCEINLINE void PspClearThreadMemoryMakerFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 2));
}

FORCEINLINE BOOLEAN PspAssignThreadMemoryMakerFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 2) : InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 2);
}

FORCEINLINE BOOLEAN PspTestThreadKeyedEventInUseFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadPassiveFlags) & (1 << 3)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadKeyedEventInUseFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 3);
}

FORCEINLINE void PspSetThreadKeyedEventInUseFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 3));
}

FORCEINLINE BOOLEAN PspClearThreadKeyedEventInUseFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 3);
}

FORCEINLINE void PspClearThreadKeyedEventInUseFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 3));
}

FORCEINLINE BOOLEAN PspAssignThreadKeyedEventInUseFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadPassiveFlags, 3) : InterlockedBitTestAndReset(&o->SameThreadPassiveFlags, 3);
}

// ==================================================

FORCEINLINE BOOLEAN PspTestThreadLpcReceivedMsgIdValidFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 0)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadLpcReceivedMsgIdValidFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 0);
}

FORCEINLINE void PspSetThreadLpcReceivedMsgIdValidFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 0));
}

FORCEINLINE BOOLEAN PspClearThreadLpcReceivedMsgIdValidFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 0);
}

FORCEINLINE void PspClearThreadLpcReceivedMsgIdValidFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 0));
}

FORCEINLINE BOOLEAN PspAssignThreadLpcReceivedMsgIdValidFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 0) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 0);
}

FORCEINLINE BOOLEAN PspTestThreadLpcExitThreadCalledFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 1)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadLpcExitThreadCalledFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 1);
}

FORCEINLINE void PspSetThreadLpcExitThreadCalledFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 1));
}

FORCEINLINE BOOLEAN PspClearThreadLpcExitThreadCalledFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 1);
}

FORCEINLINE void PspClearThreadLpcExitThreadCalledFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 1));
}

FORCEINLINE BOOLEAN PspAssignThreadLpcExitThreadCalledFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 1) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 1);
}

FORCEINLINE BOOLEAN PspTestThreadApcNeededFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 2)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadApcNeededFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 2);
}

FORCEINLINE void PspSetThreadApcNeededFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 2));
}

FORCEINLINE BOOLEAN PspClearThreadApcNeededFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 2);
}

FORCEINLINE void PspClearThreadApcNeededFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 2));
}

FORCEINLINE BOOLEAN PspAssignThreadApcNeededFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 2) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 2);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsProcessAddressSpaceExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 3)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsProcessAddressSpaceExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 3);
}

FORCEINLINE void PspSetThreadOwnsProcessAddressSpaceExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 3));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsProcessAddressSpaceExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 3);
}

FORCEINLINE void PspClearThreadOwnsProcessAddressSpaceExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 3));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsProcessAddressSpaceExclusiveFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 3) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 3);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsProcessAddressSpaceSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 4)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsProcessAddressSpaceSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 4);
}

FORCEINLINE void PspSetThreadOwnsProcessAddressSpaceSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 4));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsProcessAddressSpaceSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 4);
}

FORCEINLINE void PspClearThreadOwnsProcessAddressSpaceSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 4));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsProcessAddressSpaceSharedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 4) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 4);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsProcessWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 5)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsProcessWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 5);
}

FORCEINLINE void PspSetThreadOwnsProcessWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 5));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsProcessWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 5);
}

FORCEINLINE void PspClearThreadOwnsProcessWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 5));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsProcessWorkingSetExclusiveFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 5) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 5);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsProcessWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 6)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsProcessWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 6);
}

FORCEINLINE void PspSetThreadOwnsProcessWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 6));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsProcessWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 6);
}

FORCEINLINE void PspClearThreadOwnsProcessWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 6));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsProcessWorkingSetSharedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 6) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 6);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsSystemCacheWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 7)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsSystemCacheWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 7);
}

FORCEINLINE void PspSetThreadOwnsSystemCacheWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 7));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsSystemCacheWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 7);
}

FORCEINLINE void PspClearThreadOwnsSystemCacheWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 7));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsSystemCacheWorkingSetExclusiveFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 7) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 7);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsSystemCacheWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 8)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsSystemCacheWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 8);
}

FORCEINLINE void PspSetThreadOwnsSystemCacheWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 8));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsSystemCacheWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 8);
}

FORCEINLINE void PspClearThreadOwnsSystemCacheWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 8));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsSystemCacheWorkingSetSharedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 8) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 8);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsSessionWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 9)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsSessionWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 9);
}

FORCEINLINE void PspSetThreadOwnsSessionWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 9));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsSessionWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 9);
}

FORCEINLINE void PspClearThreadOwnsSessionWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 9));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsSessionWorkingSetExclusiveFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 9) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 9);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsSessionWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 10)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsSessionWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 10);
}

FORCEINLINE void PspSetThreadOwnsSessionWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 10));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsSessionWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 10);
}

FORCEINLINE void PspClearThreadOwnsSessionWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 10));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsSessionWorkingSetSharedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 10) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 10);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsPagedPoolWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 11)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsPagedPoolWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 11);
}

FORCEINLINE void PspSetThreadOwnsPagedPoolWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 11));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsPagedPoolWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 11);
}

FORCEINLINE void PspClearThreadOwnsPagedPoolWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 11));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsPagedPoolWorkingSetExclusiveFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 11) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 11);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsPagedPoolWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 12)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsPagedPoolWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 12);
}

FORCEINLINE void PspSetThreadOwnsPagedPoolWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 12));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsPagedPoolWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 12);
}

FORCEINLINE void PspClearThreadOwnsPagedPoolWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 12));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsPagedPoolWorkingSetSharedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 12) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 12);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsSystemPtesWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 13)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsSystemPtesWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 13);
}

FORCEINLINE void PspSetThreadOwnsSystemPtesWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 13));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsSystemPtesWorkingSetExclusiveFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 13);
}

FORCEINLINE void PspClearThreadOwnsSystemPtesWorkingSetExclusiveFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 13));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsSystemPtesWorkingSetExclusiveFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 13) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 13);
}

FORCEINLINE BOOLEAN PspTestThreadOwnsSystemPtesWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return ((o->SameThreadApcFlags) & (1 << 14)) != 0;
}

FORCEINLINE BOOLEAN PspSetThreadOwnsSystemPtesWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndSet(&o->SameThreadApcFlags, 14);
}

FORCEINLINE void PspSetThreadOwnsSystemPtesWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(!InterlockedBitTestAndSet(&o->SameThreadApcFlags, 14));
}

FORCEINLINE BOOLEAN PspClearThreadOwnsSystemPtesWorkingSetSharedFlag(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return InterlockedBitTestAndReset(&o->SameThreadApcFlags, 14);
}

FORCEINLINE void PspClearThreadOwnsSystemPtesWorkingSetSharedFlagAssert(ETHREAD* o)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    ASSERT(InterlockedBitTestAndReset(&o->SameThreadApcFlags, 14));
}

FORCEINLINE BOOLEAN PspAssignThreadOwnsSystemPtesWorkingSetSharedFlag(ETHREAD* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_THREAD(&o->Tcb);
    return value ? InterlockedBitTestAndSet(&o->SameThreadApcFlags, 14) : InterlockedBitTestAndReset(&o->SameThreadApcFlags, 14);
}

// ==================================================

FORCEINLINE BOOLEAN PspTestProcessCreateReportedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 0)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessCreateReportedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 0);
}

FORCEINLINE void PspSetProcessCreateReportedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 0));
}

FORCEINLINE BOOLEAN PspClearProcessCreateReportedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 0);
}

FORCEINLINE void PspClearProcessCreateReportedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 0));
}

FORCEINLINE BOOLEAN PspAssignProcessCreateReportedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 0) : InterlockedBitTestAndReset(&o->Flags, 0);
}

FORCEINLINE BOOLEAN PspTestProcessNoDebugInheritFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 1)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessNoDebugInheritFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 1);
}

FORCEINLINE void PspSetProcessNoDebugInheritFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 1));
}

FORCEINLINE BOOLEAN PspClearProcessNoDebugInheritFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 1);
}

FORCEINLINE void PspClearProcessNoDebugInheritFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 1));
}

FORCEINLINE BOOLEAN PspAssignProcessNoDebugInheritFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 1) : InterlockedBitTestAndReset(&o->Flags, 1);
}

FORCEINLINE BOOLEAN PspTestProcessExitingFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 2)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessExitingFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 2);
}

FORCEINLINE void PspSetProcessExitingFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 2));
}

FORCEINLINE BOOLEAN PspClearProcessExitingFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 2);
}

FORCEINLINE void PspClearProcessExitingFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 2));
}

FORCEINLINE BOOLEAN PspAssignProcessExitingFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 2) : InterlockedBitTestAndReset(&o->Flags, 2);
}

FORCEINLINE BOOLEAN PspTestProcessDeleteFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 3)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessDeleteFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 3);
}

FORCEINLINE void PspSetProcessDeleteFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 3));
}

FORCEINLINE BOOLEAN PspClearProcessDeleteFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 3);
}

FORCEINLINE void PspClearProcessDeleteFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 3));
}

FORCEINLINE BOOLEAN PspAssignProcessDeleteFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 3) : InterlockedBitTestAndReset(&o->Flags, 3);
}

FORCEINLINE BOOLEAN PspTestProcessWow64SplitPagesFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 4)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessWow64SplitPagesFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 4);
}

FORCEINLINE void PspSetProcessWow64SplitPagesFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 4));
}

FORCEINLINE BOOLEAN PspClearProcessWow64SplitPagesFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 4);
}

FORCEINLINE void PspClearProcessWow64SplitPagesFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 4));
}

FORCEINLINE BOOLEAN PspAssignProcessWow64SplitPagesFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 4) : InterlockedBitTestAndReset(&o->Flags, 4);
}

FORCEINLINE BOOLEAN PspTestProcessVmDeletedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 5)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessVmDeletedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 5);
}

FORCEINLINE void PspSetProcessVmDeletedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 5));
}

FORCEINLINE BOOLEAN PspClearProcessVmDeletedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 5);
}

FORCEINLINE void PspClearProcessVmDeletedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 5));
}

FORCEINLINE BOOLEAN PspAssignProcessVmDeletedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 5) : InterlockedBitTestAndReset(&o->Flags, 5);
}

FORCEINLINE BOOLEAN PspTestProcessOutswapEnabledFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 6)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessOutswapEnabledFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 6);
}

FORCEINLINE void PspSetProcessOutswapEnabledFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 6));
}

FORCEINLINE BOOLEAN PspClearProcessOutswapEnabledFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 6);
}

FORCEINLINE void PspClearProcessOutswapEnabledFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 6));
}

FORCEINLINE BOOLEAN PspAssignProcessOutswapEnabledFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 6) : InterlockedBitTestAndReset(&o->Flags, 6);
}

FORCEINLINE BOOLEAN PspTestProcessOutswappedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 7)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessOutswappedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 7);
}

FORCEINLINE void PspSetProcessOutswappedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 7));
}

FORCEINLINE BOOLEAN PspClearProcessOutswappedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 7);
}

FORCEINLINE void PspClearProcessOutswappedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 7));
}

FORCEINLINE BOOLEAN PspAssignProcessOutswappedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 7) : InterlockedBitTestAndReset(&o->Flags, 7);
}

FORCEINLINE BOOLEAN PspTestProcessForkFailedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 8)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessForkFailedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 8);
}

FORCEINLINE void PspSetProcessForkFailedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 8));
}

FORCEINLINE BOOLEAN PspClearProcessForkFailedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 8);
}

FORCEINLINE void PspClearProcessForkFailedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 8));
}

FORCEINLINE BOOLEAN PspAssignProcessForkFailedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 8) : InterlockedBitTestAndReset(&o->Flags, 8);
}

FORCEINLINE BOOLEAN PspTestProcessWow64VaSpace4gbFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 9)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessWow64VaSpace4gbFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 9);
}

FORCEINLINE void PspSetProcessWow64VaSpace4gbFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 9));
}

FORCEINLINE BOOLEAN PspClearProcessWow64VaSpace4gbFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 9);
}

FORCEINLINE void PspClearProcessWow64VaSpace4gbFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 9));
}

FORCEINLINE BOOLEAN PspAssignProcessWow64VaSpace4gbFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 9) : InterlockedBitTestAndReset(&o->Flags, 9);
}

FORCEINLINE ULONG PspGetProcessAddressSpaceInitialized(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return (o->Flags >> 10) & ((1 << 2) - 1);
}

FORCEINLINE ULONG PspSetProcessAddressSpaceInitialized(EPROCESS* o, ULONG value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    LONG comparand;
    ULONG v, mask = ~(((1 << 2) - 1) << 10);
    value <<= 10;
    do
    {
        comparand = o->Flags;
        v = (comparand & mask) | value;
    } while (InterlockedCompareExchange(&o->Flags, v, comparand) != comparand);
    return (comparand >> 10) & ((1 << 2) - 1);
}

FORCEINLINE BOOLEAN PspTestProcessSetTimerResolutionFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 12)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessSetTimerResolutionFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 12);
}

FORCEINLINE void PspSetProcessSetTimerResolutionFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 12));
}

FORCEINLINE BOOLEAN PspClearProcessSetTimerResolutionFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 12);
}

FORCEINLINE void PspClearProcessSetTimerResolutionFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 12));
}

FORCEINLINE BOOLEAN PspAssignProcessSetTimerResolutionFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 12) : InterlockedBitTestAndReset(&o->Flags, 12);
}

FORCEINLINE BOOLEAN PspTestProcessBreakOnTerminationFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 13)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessBreakOnTerminationFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 13);
}

FORCEINLINE void PspSetProcessBreakOnTerminationFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 13));
}

FORCEINLINE BOOLEAN PspClearProcessBreakOnTerminationFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 13);
}

FORCEINLINE void PspClearProcessBreakOnTerminationFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 13));
}

FORCEINLINE BOOLEAN PspAssignProcessBreakOnTerminationFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 13) : InterlockedBitTestAndReset(&o->Flags, 13);
}

FORCEINLINE BOOLEAN PspTestProcessSessionCreationUnderwayFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 14)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessSessionCreationUnderwayFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 14);
}

FORCEINLINE void PspSetProcessSessionCreationUnderwayFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 14));
}

FORCEINLINE BOOLEAN PspClearProcessSessionCreationUnderwayFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 14);
}

FORCEINLINE void PspClearProcessSessionCreationUnderwayFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 14));
}

FORCEINLINE BOOLEAN PspAssignProcessSessionCreationUnderwayFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 14) : InterlockedBitTestAndReset(&o->Flags, 14);
}

FORCEINLINE BOOLEAN PspTestProcessWriteWatchFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 15)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessWriteWatchFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 15);
}

FORCEINLINE void PspSetProcessWriteWatchFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 15));
}

FORCEINLINE BOOLEAN PspClearProcessWriteWatchFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 15);
}

FORCEINLINE void PspClearProcessWriteWatchFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 15));
}

FORCEINLINE BOOLEAN PspAssignProcessWriteWatchFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 15) : InterlockedBitTestAndReset(&o->Flags, 15);
}

FORCEINLINE BOOLEAN PspTestProcessInSessionFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 16)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessInSessionFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 16);
}

FORCEINLINE void PspSetProcessInSessionFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 16));
}

FORCEINLINE BOOLEAN PspClearProcessInSessionFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 16);
}

FORCEINLINE void PspClearProcessInSessionFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 16));
}

FORCEINLINE BOOLEAN PspAssignProcessInSessionFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 16) : InterlockedBitTestAndReset(&o->Flags, 16);
}

FORCEINLINE BOOLEAN PspTestProcessOverrideAddressSpaceFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 17)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessOverrideAddressSpaceFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 17);
}

FORCEINLINE void PspSetProcessOverrideAddressSpaceFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 17));
}

FORCEINLINE BOOLEAN PspClearProcessOverrideAddressSpaceFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 17);
}

FORCEINLINE void PspClearProcessOverrideAddressSpaceFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 17));
}

FORCEINLINE BOOLEAN PspAssignProcessOverrideAddressSpaceFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 17) : InterlockedBitTestAndReset(&o->Flags, 17);
}

FORCEINLINE BOOLEAN PspTestProcessHasAddressSpaceFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 18)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessHasAddressSpaceFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 18);
}

FORCEINLINE void PspSetProcessHasAddressSpaceFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 18));
}

FORCEINLINE BOOLEAN PspClearProcessHasAddressSpaceFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 18);
}

FORCEINLINE void PspClearProcessHasAddressSpaceFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 18));
}

FORCEINLINE BOOLEAN PspAssignProcessHasAddressSpaceFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 18) : InterlockedBitTestAndReset(&o->Flags, 18);
}

FORCEINLINE BOOLEAN PspTestProcessLaunchPrefetchedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 19)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessLaunchPrefetchedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 19);
}

FORCEINLINE void PspSetProcessLaunchPrefetchedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 19));
}

FORCEINLINE BOOLEAN PspClearProcessLaunchPrefetchedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 19);
}

FORCEINLINE void PspClearProcessLaunchPrefetchedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 19));
}

FORCEINLINE BOOLEAN PspAssignProcessLaunchPrefetchedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 19) : InterlockedBitTestAndReset(&o->Flags, 19);
}

FORCEINLINE BOOLEAN PspTestProcessInjectInpageErrorsFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 20)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessInjectInpageErrorsFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 20);
}

FORCEINLINE void PspSetProcessInjectInpageErrorsFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 20));
}

FORCEINLINE BOOLEAN PspClearProcessInjectInpageErrorsFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 20);
}

FORCEINLINE void PspClearProcessInjectInpageErrorsFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 20));
}

FORCEINLINE BOOLEAN PspAssignProcessInjectInpageErrorsFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 20) : InterlockedBitTestAndReset(&o->Flags, 20);
}

FORCEINLINE BOOLEAN PspTestProcessVmTopDownFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 21)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessVmTopDownFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 21);
}

FORCEINLINE void PspSetProcessVmTopDownFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 21));
}

FORCEINLINE BOOLEAN PspClearProcessVmTopDownFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 21);
}

FORCEINLINE void PspClearProcessVmTopDownFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 21));
}

FORCEINLINE BOOLEAN PspAssignProcessVmTopDownFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 21) : InterlockedBitTestAndReset(&o->Flags, 21);
}

FORCEINLINE BOOLEAN PspTestProcessImageNotifyDoneFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 22)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessImageNotifyDoneFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 22);
}

FORCEINLINE void PspSetProcessImageNotifyDoneFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 22));
}

FORCEINLINE BOOLEAN PspClearProcessImageNotifyDoneFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 22);
}

FORCEINLINE void PspClearProcessImageNotifyDoneFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 22));
}

FORCEINLINE BOOLEAN PspAssignProcessImageNotifyDoneFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 22) : InterlockedBitTestAndReset(&o->Flags, 22);
}

FORCEINLINE BOOLEAN PspTestProcessPdeUpdateNeededFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 23)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessPdeUpdateNeededFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 23);
}

FORCEINLINE void PspSetProcessPdeUpdateNeededFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 23));
}

FORCEINLINE BOOLEAN PspClearProcessPdeUpdateNeededFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 23);
}

FORCEINLINE void PspClearProcessPdeUpdateNeededFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 23));
}

FORCEINLINE BOOLEAN PspAssignProcessPdeUpdateNeededFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 23) : InterlockedBitTestAndReset(&o->Flags, 23);
}

FORCEINLINE BOOLEAN PspTestProcessVdmAllowedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 24)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessVdmAllowedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 24);
}

FORCEINLINE void PspSetProcessVdmAllowedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 24));
}

FORCEINLINE BOOLEAN PspClearProcessVdmAllowedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 24);
}

FORCEINLINE void PspClearProcessVdmAllowedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 24));
}

FORCEINLINE BOOLEAN PspAssignProcessVdmAllowedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 24) : InterlockedBitTestAndReset(&o->Flags, 24);
}

FORCEINLINE BOOLEAN PspTestProcessSwapAllowedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 25)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessSwapAllowedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 25);
}

FORCEINLINE void PspSetProcessSwapAllowedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 25));
}

FORCEINLINE BOOLEAN PspClearProcessSwapAllowedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 25);
}

FORCEINLINE void PspClearProcessSwapAllowedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 25));
}

FORCEINLINE BOOLEAN PspAssignProcessSwapAllowedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 25) : InterlockedBitTestAndReset(&o->Flags, 25);
}

FORCEINLINE BOOLEAN PspTestProcessCreateFailedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 26)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessCreateFailedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 26);
}

FORCEINLINE void PspSetProcessCreateFailedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 26));
}

FORCEINLINE BOOLEAN PspClearProcessCreateFailedFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 26);
}

FORCEINLINE void PspClearProcessCreateFailedFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 26));
}

FORCEINLINE BOOLEAN PspAssignProcessCreateFailedFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 26) : InterlockedBitTestAndReset(&o->Flags, 26);
}

FORCEINLINE ULONG PspGetProcessDefaultIoPriority(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return (o->Flags >> 27) & ((1 << 3) - 1);
}

FORCEINLINE ULONG PspSetProcessDefaultIoPriority(EPROCESS* o, ULONG value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    LONG comparand;
    ULONG v, mask = ~(((1 << 3) - 1) << 27);
    value <<= 27;
    do
    {
        comparand = o->Flags;
        v = (comparand & mask) | value;
    } while (InterlockedCompareExchange(&o->Flags, v, comparand) != comparand);
    return (comparand >> 27) & ((1 << 3) - 1);
}

FORCEINLINE BOOLEAN PspTestProcessSystemProcessFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 30)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessSystemProcessFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 30);
}

FORCEINLINE void PspSetProcessSystemProcessFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 30));
}

FORCEINLINE BOOLEAN PspClearProcessSystemProcessFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 30);
}

FORCEINLINE void PspClearProcessSystemProcessFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 30));
}

FORCEINLINE BOOLEAN PspAssignProcessSystemProcessFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 30) : InterlockedBitTestAndReset(&o->Flags, 30);
}

FORCEINLINE BOOLEAN PspTestProcessPropagateNodeFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return ((o->Flags) & (1 << 31)) != 0;
}

FORCEINLINE BOOLEAN PspSetProcessPropagateNodeFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndSet(&o->Flags, 31);
}

FORCEINLINE void PspSetProcessPropagateNodeFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(!InterlockedBitTestAndSet(&o->Flags, 31));
}

FORCEINLINE BOOLEAN PspClearProcessPropagateNodeFlag(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return InterlockedBitTestAndReset(&o->Flags, 31);
}

FORCEINLINE void PspClearProcessPropagateNodeFlagAssert(EPROCESS* o)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    ASSERT(InterlockedBitTestAndReset(&o->Flags, 31));
}

FORCEINLINE BOOLEAN PspAssignProcessPropagateNodeFlag(EPROCESS* o, BOOLEAN value)
{
    ASSERT(o);
    ASSERT_PROCESS(&o->Pcb);
    return value ? InterlockedBitTestAndSet(&o->Flags, 31) : InterlockedBitTestAndReset(&o->Flags, 31);
}

