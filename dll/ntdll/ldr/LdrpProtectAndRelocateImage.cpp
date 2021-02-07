#include <ldrp.h>


ULONG
NTAPI
LdrpProtectAndRelocateImage(
    IN PVOID BaseAddress,
    IN PCCH LoaderName,
    IN ULONG Success,
    IN ULONG Conflict,
    IN ULONG Invalid)
{
    NTSTATUS Status;
    BOOL DoProtect = TRUE; /* Process can disable protection during relocations, not implemented */

    if (DoProtect)
    {
        Status = LdrpSetProtection(BaseAddress, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Changing the protection [FALSE] of the executable at " LDR_HEXPTR_FMT " failed [%#x].\n", BaseAddress, Status);
            return Status;
        }
    }

    /* Do the relocation */
    Status = LdrRelocateImageWithBias(BaseAddress,
                                      0,
                                      LoaderName,
                                      Success,
                                      Conflict,
                                      Invalid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LdrRelocateImageWithBias() failed [%#x]\n", Status);
        return Status;
    }

    /* Restore the protection */
    if (DoProtect)
    {
        Status = LdrpSetProtection(BaseAddress, TRUE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Changing the protection [TRUE] of the executable at " LDR_HEXPTR_FMT " failed [%#x].\n", BaseAddress, Status);
            return Status;
        }
    }

    return Status;
}