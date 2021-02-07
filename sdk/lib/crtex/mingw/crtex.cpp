#include "crtex_mingw.h"

ULONG GetSystemPageSize()
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      0);

    if (!NT_SUCCESS(Status))
        return 0;

    return BasicInfo.PageSize;
}
