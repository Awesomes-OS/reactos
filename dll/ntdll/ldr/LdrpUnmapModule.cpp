#include <ldrp.h>

EXTERN_C
{

#include <ndk/mmfuncs.h>

}

NTSTATUS
NTAPI
LdrpUnmapModule(IN PLDR_DATA_TABLE_ENTRY Module)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Module->DllBase)
    {
        Status = NtUnmapViewOfSection(NtCurrentProcess(), Module->DllBase);
        Module->DllBase = NULL;
    }

    return Status;
}
