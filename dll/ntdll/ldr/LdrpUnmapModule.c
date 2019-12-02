#include <ldrp.h>
#include <ndk/mmfuncs.h>

NTSTATUS
NTAPI
LdrpUnmapModule(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (LdrEntry->DllBase)
    {
        Status = NtUnmapViewOfSection(NtCurrentProcess(),
            LdrEntry->DllBase);
        LdrEntry->DllBase = NULL;
    }
    return Status;
}