#include <ldrp.h>
#include <reactos/ldrp.h>

NTSTATUS
NTAPI
LdrpApplyFileNameRedirection(IN PUNICODE_STRING DllName,
                             IN OUT PLDRP_UNICODE_STRING_BUNDLE RedirectedDllName,
                             OUT PBOOLEAN RedirectedSxS)
{
    NTSTATUS Status;
    UNICODE_STRING DllNameSxS;

    *RedirectedSxS = FALSE;

    // todo: API Sets redirection support (mapping to host system)

    /* Initialize the strings */
    RtlInitEmptyUnicodeString(&DllNameSxS, NULL, 0);

    /* Check if the SxS Assemblies specify another file */
    Status = RtlDosApplyFileIsolationRedirection_Ustr(TRUE,
                                                      DllName,
                                                      &LdrpDefaultExtension,
                                                      NULL,
                                                      &DllNameSxS,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      NULL);

    if (NT_SUCCESS(Status))
    {
        *RedirectedSxS = TRUE;
        LdrpGetFullPath(&DllNameSxS, RedirectedDllName);
        LdrpFreeUnicodeString(&DllNameSxS);
    }
    else if (Status == STATUS_SXS_KEY_NOT_FOUND)
    {
        // This function returns success even if no redirection was applied
        Status = STATUS_SUCCESS;
    }

    ASSERT(!DllNameSxS.Buffer);

    return Status;
}
