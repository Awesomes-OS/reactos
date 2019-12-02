
@ stdcall InitOnceExecuteOnce(ptr ptr ptr ptr)
@ stdcall GetFileInformationByHandleEx(long long ptr long)
@ stdcall -ret64 GetTickCount64()

@ stdcall InitializeSRWLock(ptr)
@ stdcall AcquireSRWLockExclusive(ptr)
@ stdcall AcquireSRWLockShared(ptr)
@ stdcall ReleaseSRWLockExclusive(ptr)
@ stdcall ReleaseSRWLockShared(ptr)

@ stdcall InitializeConditionVariable(ptr)
@ stdcall SleepConditionVariableCS(ptr ptr long)
@ stdcall SleepConditionVariableSRW(ptr ptr long long)
@ stdcall WakeAllConditionVariable(ptr)
@ stdcall WakeConditionVariable(ptr)

@ stdcall InitializeCriticalSectionEx(ptr long long)

@ stdcall GetNLSVersionEx(long wstr ptr)

@ stdcall WerRegisterFile(wstr long long)
@ stdcall WerRegisterMemoryBlock(ptr long)
@ stdcall WerRegisterRuntimeExceptionModule(wstr ptr)
@ stdcall WerSetFlags(long)
# @ stdcall -stub WerUnregisterFile
@ stdcall WerUnregisterMemoryBlock(ptr)
# @ stdcall -stub WerUnregisterRuntimeExceptionModule
@ stdcall -stub FlushProcessWriteBuffers()

@ stdcall CallbackMayRunLong(ptr)
@ stdcall CreateThreadpool(ptr)
@ stdcall CreateThreadpoolCleanupGroup()
@ stdcall CreateThreadpoolIo(ptr ptr ptr ptr)
@ stdcall CreateThreadpoolTimer(ptr ptr ptr)
@ stdcall CreateThreadpoolWait(ptr ptr ptr)
@ stdcall CreateThreadpoolWork(ptr ptr ptr)
@ stdcall SetThreadpoolThreadMaximum(ptr long) ntdll.TpSetPoolMaxThreads
@ stdcall SetThreadpoolThreadMinimum(ptr long) ntdll.TpSetPoolMinThreads
@ stdcall SetThreadpoolTimer(ptr ptr long long)
@ stdcall SetThreadpoolWait(ptr long ptr)
@ stdcall TrySubmitThreadpoolCallback(ptr ptr ptr)
@ stdcall CloseThreadpool(ptr) ntdll.TpReleasePool
@ stdcall CloseThreadpoolCleanupGroup(ptr) ntdll.TpReleaseCleanupGroup
@ stdcall CloseThreadpoolCleanupGroupMembers(ptr long ptr) ntdll.TpReleaseCleanupGroupMembers
@ stdcall CloseThreadpoolTimer(ptr) ntdll.TpReleaseTimer
@ stdcall CloseThreadpoolWait(ptr) ntdll.TpReleaseWait
@ stdcall CloseThreadpoolWork(ptr) ntdll.TpReleaseWork

@ stdcall CreateMutexExA(ptr str long long)
@ stdcall CreateMutexExW(ptr wstr long long)
@ stdcall CreateSemaphoreExA(ptr long long str long long)
@ stdcall CreateSemaphoreExW(ptr long long wstr long long)