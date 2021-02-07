/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode DLL
 * FILE:            lib/ntdll/rtl/libsup.c
 * PURPOSE:         RTL Support Routines
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#include <stdlib.h>

#define NDEBUG
#include <debug.h>

SIZE_T RtlpAllocDeallocQueryBufferSize = PAGE_SIZE;
PTEB LdrpTopLevelDllBeingLoadedTeb = NULL;
PVOID MmHighestUserAddress = (PVOID)MI_HIGHEST_USER_ADDRESS;

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOL
NTAPI
RtlIsProcessorFeaturePresent(IN DWORD ProcessorFeature)
{
    if (ProcessorFeature >= PROCESSOR_FEATURE_MAX) return FALSE;
    return ((BOOL)SharedUserData->ProcessorFeatures[ProcessorFeature]);
}

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(VOID)
{
    /* Return the flag in the PEB */
    return NtCurrentPeb()->BeingDebugged;
}

BOOLEAN
NTAPI
RtlpSetInDbgPrint(VOID)
{
    /* Check if it's already set and return TRUE if so */
    if (NtCurrentTeb()->InDbgPrint) return TRUE;

    /* Set it and return */
    NtCurrentTeb()->InDbgPrint = TRUE;
    return FALSE;
}

VOID
NTAPI
RtlpClearInDbgPrint(VOID)
{
    /* Clear the flag */
    NtCurrentTeb()->InDbgPrint = FALSE;
}

KPROCESSOR_MODE
NTAPI
RtlpGetMode(VOID)
{
   return UserMode;
}

/*
 * @implemented
 */
PPEB
NTAPI
RtlGetCurrentPeb(VOID)
{
    return NtCurrentPeb();
}

/*
 * @implemented
 */
VOID NTAPI
RtlAcquirePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   RtlEnterCriticalSection(Peb->FastPebLock);
}

/*
 * @implemented
 */
VOID NTAPI
RtlReleasePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   RtlLeaveCriticalSection(Peb->FastPebLock);
}

/*
* @implemented
*/
ULONG
NTAPI
RtlGetNtGlobalFlags(VOID)
{
    PPEB pPeb = NtCurrentPeb();
    return pPeb->NtGlobalFlag;
}

NTSTATUS
NTAPI
RtlDeleteHeapLock(IN OUT PHEAP_LOCK Lock)
{
    return RtlDeleteCriticalSection(&Lock->CriticalSection);
}

NTSTATUS
NTAPI
RtlEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive)
{
    UNREFERENCED_PARAMETER(Exclusive);

    return RtlEnterCriticalSection(&Lock->CriticalSection);
}

BOOLEAN
NTAPI
RtlTryEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive)
{
    UNREFERENCED_PARAMETER(Exclusive);

    return RtlTryEnterCriticalSection(&Lock->CriticalSection);
}

NTSTATUS
NTAPI
RtlInitializeHeapLock(IN OUT PHEAP_LOCK *Lock)
{
    return RtlInitializeCriticalSection(&(*Lock)->CriticalSection);
}

NTSTATUS
NTAPI
RtlLeaveHeapLock(IN OUT PHEAP_LOCK Lock)
{
    return RtlLeaveCriticalSection(&Lock->CriticalSection);
}

PVOID
NTAPI
RtlpAllocateMemory(UINT Bytes,
                   ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);

    return RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           Bytes);
}


VOID
NTAPI
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                Mem);
}


#if DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  /* meaningless in user mode */
}
#endif

VOID
NTAPI
RtlpSetHeapParameters(IN PRTL_HEAP_PARAMETERS Parameters)
{
    PPEB Peb;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Apply defaults for non-set parameters */
    if (!Parameters->SegmentCommit) Parameters->SegmentCommit = Peb->HeapSegmentCommit;
    if (!Parameters->SegmentReserve) Parameters->SegmentReserve = Peb->HeapSegmentReserve;
    if (!Parameters->DeCommitFreeBlockThreshold) Parameters->DeCommitFreeBlockThreshold = Peb->HeapDeCommitFreeBlockThreshold;
    if (!Parameters->DeCommitTotalFreeThreshold) Parameters->DeCommitTotalFreeThreshold = Peb->HeapDeCommitTotalFreeThreshold;
}

BOOLEAN
NTAPI
RtlpHandleDpcStackException(IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN ULONG_PTR RegistrationFrameEnd,
                            IN OUT PULONG_PTR StackLow,
                            IN OUT PULONG_PTR StackHigh)
{
    /* There's no such thing as a DPC stack in user-mode */
    return FALSE;
}

VOID
NTAPI
RtlpCheckLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                      IN PCONTEXT ContextRecord,
                      IN PVOID ContextData,
                      IN ULONG Size)
{
    /* Exception logging is not done in user-mode */
}

BOOLEAN
NTAPI
RtlpCaptureStackLimits(IN ULONG_PTR Ebp,
                       IN ULONG_PTR *StackBegin,
                       IN ULONG_PTR *StackEnd)
{
    /* FIXME: Verify */
    *StackBegin = (ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit;
    *StackEnd = (ULONG_PTR)NtCurrentTeb()->NtTib.StackBase;
    return TRUE;
}

#ifndef _M_AMD64
/*
 * @implemented
 */
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    ULONG_PTR Stack, NewStack, StackBegin, StackEnd = 0;
    ULONG Eip;
    BOOLEAN Result, StopSearch = FALSE;
    ULONG i = 0;

    /* Get current EBP */
#if defined(_M_IX86)
#if defined __GNUC__
    __asm__("mov %%ebp, %0" : "=r" (Stack) : );
#elif defined(_MSC_VER)
    __asm mov Stack, ebp
#endif
#elif defined(_M_MIPS)
        __asm__("move $sp, %0" : "=r" (Stack) : );
#elif defined(_M_PPC)
    __asm__("mr %0,1" : "=r" (Stack) : );
#elif defined(_M_ARM)
#if defined __GNUC__
    __asm__("mov sp, %0" : "=r"(Stack) : );
#elif defined(_MSC_VER)
    // FIXME: Hack. Probably won't work if this ever actually manages to run someday.
    Stack = (ULONG_PTR)&Stack;
#endif
#else
#error Unknown architecture
#endif

    /* Set it as the stack begin limit as well */
    StackBegin = (ULONG_PTR)Stack;

    /* Check if we're called for non-logging mode */
    if (!Flags)
    {
        /* Get the actual safe limits */
        Result = RtlpCaptureStackLimits((ULONG_PTR)Stack,
                                        &StackBegin,
                                        &StackEnd);
        if (!Result) return 0;
    }

    /* Use a SEH block for maximum protection */
    _SEH2_TRY
    {
        /* Loop the frames */
        for (i = 0; i < Count; i++)
        {
            /*
             * Leave if we're past the stack,
             * if we're before the stack,
             * or if we've reached ourselves.
             */
            if ((Stack >= StackEnd) ||
                (!i ? (Stack < StackBegin) : (Stack <= StackBegin)) ||
                ((StackEnd - Stack) < (2 * sizeof(ULONG_PTR))))
            {
                /* We're done or hit a bad address */
                break;
            }

            /* Get new stack and EIP */
            NewStack = *(PULONG_PTR)Stack;
            Eip = *(PULONG_PTR)(Stack + sizeof(ULONG_PTR));

            /* Check if the new pointer is above the oldone and past the end */
            if (!((Stack < NewStack) && (NewStack < StackEnd)))
            {
                /* Stop searching after this entry */
                StopSearch = TRUE;
            }

            /* Also make sure that the EIP isn't a stack address */
            if ((StackBegin < Eip) && (Eip < StackEnd)) break;

            /* FIXME: Check that EIP is inside a loaded module */

            /* Save this frame */
            Callers[i] = (PVOID)Eip;

            /* Check if we should continue */
            if (StopSearch)
            {
                /* Return the next index */
                i++;
                break;
            }

            /* Move to the next stack */
            Stack = NewStack;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* No index */
        i = 0;
    }
    _SEH2_END;

    /* Return frames parsed */
    return i;
}
#endif

#ifdef _AMD64_
VOID
NTAPI
RtlpGetStackLimits(
    OUT PULONG_PTR LowLimit,
    OUT PULONG_PTR HighLimit)
{
    *LowLimit = (ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit;
    *HighLimit = (ULONG_PTR)NtCurrentTeb()->NtTib.StackBase;
    return;
}
#endif

BOOLEAN
NTAPI
RtlIsThreadWithinLoaderCallout(VOID)
{
    return LdrpTopLevelDllBeingLoadedTeb == NtCurrentTeb();
}

/* RTL Atom Tables ************************************************************/

typedef struct _RTL_ATOM_HANDLE
{
   RTL_HANDLE_TABLE_ENTRY Handle;
   PRTL_ATOM_TABLE_ENTRY AtomEntry;
} RTL_ATOM_HANDLE, *PRTL_ATOM_HANDLE;

NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   RtlInitializeCriticalSection(&AtomTable->CriticalSection);
   return STATUS_SUCCESS;
}


VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   RtlDeleteCriticalSection(&AtomTable->CriticalSection);
}


BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlEnterCriticalSection(&AtomTable->CriticalSection);
   return TRUE;
}


VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlLeaveCriticalSection(&AtomTable->CriticalSection);
}


/* handle functions */

BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlInitializeHandleTable(0xCFFF,
                            sizeof(RTL_ATOM_HANDLE),
                            &AtomTable->RtlHandleTable);

   return TRUE;
}

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlDestroyHandleTable(&AtomTable->RtlHandleTable);
}

PRTL_ATOM_TABLE
RtlpAllocAtomTable(ULONG Size)
{
   return (PRTL_ATOM_TABLE)RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           Size);
}

VOID
RtlpFreeAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               AtomTable);
}

PRTL_ATOM_TABLE_ENTRY
RtlpAllocAtomTableEntry(ULONG Size)
{
   return (PRTL_ATOM_TABLE_ENTRY)RtlAllocateHeap(RtlGetProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 Size);
}

VOID
RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry)
{
   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               Entry);
}

VOID
RtlpFreeAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   PRTL_HANDLE_TABLE_ENTRY RtlHandleEntry;

   if (RtlIsValidIndexHandle(&AtomTable->RtlHandleTable,
                             (ULONG)Entry->HandleIndex,
                             &RtlHandleEntry))
   {
      RtlFreeHandle(&AtomTable->RtlHandleTable,
                    RtlHandleEntry);
   }
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   ULONG HandleIndex;
   PRTL_HANDLE_TABLE_ENTRY RtlHandle;

   RtlHandle = RtlAllocateHandle(&AtomTable->RtlHandleTable,
                                 &HandleIndex);
   if (RtlHandle != NULL)
   {
      PRTL_ATOM_HANDLE AtomHandle = (PRTL_ATOM_HANDLE)RtlHandle;

      /* FIXME - Handle Indexes >= 0xC000 ?! */
      if (HandleIndex < 0xC000)
      {
         Entry->HandleIndex = (USHORT)HandleIndex;
         Entry->Atom = 0xC000 + (USHORT)HandleIndex;

         AtomHandle->AtomEntry = Entry;
         AtomHandle->Handle.Flags = RTL_HANDLE_VALID;

         return TRUE;
      }
      else
      {
         /* set the valid flag, otherwise RtlFreeHandle will fail! */
         AtomHandle->Handle.Flags = RTL_HANDLE_VALID;

         RtlFreeHandle(&AtomTable->RtlHandleTable,
                       RtlHandle);
      }
   }

   return FALSE;
}

PRTL_ATOM_TABLE_ENTRY
RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index)
{
   PRTL_HANDLE_TABLE_ENTRY RtlHandle;

   if (RtlIsValidIndexHandle(&AtomTable->RtlHandleTable,
                             Index,
                             &RtlHandle))
   {
      PRTL_ATOM_HANDLE AtomHandle = (PRTL_ATOM_HANDLE)RtlHandle;

      return AtomHandle->AtomEntry;
   }

   return NULL;
}

/* Ldr SEH-Protected access to IMAGE_NT_HEADERS */

/* Rtl SEH-Free version of this */
NTSTATUS
NTAPI
RtlpImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders);


/*
 * @implemented
 * @note: This is here, so that we do not drag SEH into rosload, freeldr and bootmgfw
 */
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders)
{
    NTSTATUS Status;

    /* Assume failure. This is also done in RtlpImageNtHeaderEx, but this is guarded by SEH. */
    if (OutHeaders != NULL)
        *OutHeaders = NULL;

    _SEH2_TRY
    {
        Status = RtlpImageNtHeaderEx(Flags, Base, Size, OutHeaders);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail with the SEH error */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

/*
 * Ldr Resource support code
 */

IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( IMAGE_RESOURCE_DIRECTORY *dir,
                                              LPCWSTR name, void *root,
                                              int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( IMAGE_RESOURCE_DIRECTORY *dir,
                                            WORD id, void *root, int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_first_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                            void *root, int want_dir );
int push_language( USHORT *list, ULONG pos, WORD lang );

/**********************************************************************
 *  find_entry
 *
 * Find a resource entry
 */
NTSTATUS find_entry( PVOID BaseAddress, LDR_RESOURCE_INFO *info,
                     ULONG level, void **ret, int want_dir )
{
    ULONG size;
    void *root;
    IMAGE_RESOURCE_DIRECTORY *resdirptr;
    USHORT list[9];  /* list of languages to try */
    int i, pos = 0;
    LCID user_lcid, system_lcid;

    root = RtlImageDirectoryEntryToData( BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size );
    if (!root) return STATUS_RESOURCE_DATA_NOT_FOUND;
    if (size < sizeof(*resdirptr)) return STATUS_RESOURCE_DATA_NOT_FOUND;
    resdirptr = root;

    if (!level--) goto done;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Type, root, want_dir || level )))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;

    resdirptr = *ret;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Name, root, want_dir || level )))
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;
    if (level) return STATUS_INVALID_PARAMETER;  /* level > 3 */

    /* 1. specified language */
    pos = push_language( list, pos, info->Language );

    /* 2. specified language with neutral sublanguage */
    pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(info->Language), SUBLANG_NEUTRAL ) );

    /* 3. neutral language with neutral sublanguage */
    pos = push_language( list, pos, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );

    /* if no explicitly specified language, try some defaults */
    if (PRIMARYLANGID(info->Language) == LANG_NEUTRAL)
    {
        /* user defaults, unless SYS_DEFAULT sublanguage specified  */
        if (SUBLANGID(info->Language) != SUBLANG_SYS_DEFAULT)
        {
            /* 4. current thread locale language */
            pos = push_language( list, pos, LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale) );

            if (NT_SUCCESS(NtQueryDefaultLocale(TRUE, &user_lcid)))
            {
                /* 5. user locale language */
                pos = push_language( list, pos, LANGIDFROMLCID(user_lcid) );

                /* 6. user locale language with neutral sublanguage  */
                pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(user_lcid), SUBLANG_NEUTRAL ) );
            }
        }

        /* now system defaults */

        if (NT_SUCCESS(NtQueryDefaultLocale(FALSE, &system_lcid)))
        {
            /* 7. system locale language */
            pos = push_language( list, pos, LANGIDFROMLCID( system_lcid ) );

            /* 8. system locale language with neutral sublanguage */
            pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(system_lcid), SUBLANG_NEUTRAL ) );
        }

        /* 9. English */
        pos = push_language( list, pos, MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    }

    resdirptr = *ret;
    for (i = 0; i < pos; i++)
        if ((*ret = find_entry_by_id( resdirptr, list[i], root, want_dir ))) return STATUS_SUCCESS;

    /* if no explicitly specified language, return the first entry */
    if (PRIMARYLANGID(info->Language) == LANG_NEUTRAL)
    {
        if ((*ret = find_first_entry( resdirptr, root, want_dir ))) return STATUS_SUCCESS;
    }
    return STATUS_RESOURCE_LANG_NOT_FOUND;

done:
    *ret = resdirptr;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PVOID NTAPI
RtlPcToFileHeader(IN PVOID PcValue, OUT PVOID* BaseOfImage)
{
    const PPEB Peb = NtCurrentPeb();
    PLIST_ENTRY ModuleListHead;
    PVOID ImageBase = NULL;

    RtlEnterCriticalSection(Peb->LoaderLock);

    ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
    for (PLIST_ENTRY Entry = ModuleListHead->Flink; Entry != ModuleListHead; Entry = Entry->Flink)
    {
        const PLDR_DATA_TABLE_ENTRY Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if ((ULONG_PTR)PcValue < (ULONG_PTR)Module->DllBase)
            continue;

        if ((ULONG_PTR)PcValue >= (ULONG_PTR)Module->DllBase + Module->SizeOfImage)
            continue;

        ImageBase = Module->DllBase;
        break;
    }

    RtlLeaveCriticalSection(Peb->LoaderLock);

    *BaseOfImage = ImageBase;
    return ImageBase;
}

NTSTATUS get_buffer(LPWSTR *buffer, SIZE_T needed, PUNICODE_STRING CallerBuffer, BOOLEAN bAllocateBuffer)
{
    WCHAR *p;

    if (CallerBuffer && CallerBuffer->MaximumLength > needed)
    {
        p = CallerBuffer->Buffer;
        CallerBuffer->Length = needed - sizeof(WCHAR);
    }
    else
    {
        if (!bAllocateBuffer)
            return STATUS_BUFFER_TOO_SMALL;

        if (CallerBuffer)
            CallerBuffer->Buffer[0] = 0;

        p = RtlAllocateHeap(RtlGetProcessHeap(), 0, needed );
        if (!p)
            return STATUS_NO_MEMORY;
    }
    *buffer = p;

    return STATUS_SUCCESS;
}

/* NOTE: Remove this one once our actctx support becomes better */
NTSTATUS find_actctx_dll( PUNICODE_STRING pnameW, LPWSTR *fullname, PUNICODE_STRING CallerBuffer, BOOLEAN bAllocateBuffer)
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\'};
    static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};

    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info;
    ACTCTX_SECTION_KEYED_DATA data;
    NTSTATUS status;
    SIZE_T needed, size = 1024;
    WCHAR *p;

    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    pnameW, &data );
    if (status != STATUS_SUCCESS)
    {
        //DPRINT1("RtlFindActivationContextSectionString returned 0x%x for %wZ\n", status, pnameW);
        return status;
    }

    for (;;)
    {
        if (!(info = RtlAllocateHeap( RtlGetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        RtlFreeHeap( RtlGetProcessHeap(), 0, info );
        size = needed;
    }

    DPRINT("manifestpath === %S\n", info->lpAssemblyManifestPath);
    DPRINT("DirectoryName === %S\n", info->lpAssemblyDirectoryName);
    if (!info->lpAssemblyManifestPath /*|| !info->lpAssemblyDirectoryName*/)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }    

    if ((p = wcsrchr( info->lpAssemblyManifestPath, '\\' )))
    {
        DWORD dirlen = info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);

        p++;
        if (!info->lpAssemblyDirectoryName || _wcsnicmp( p, info->lpAssemblyDirectoryName, dirlen ) || wcsicmp( p + dirlen, dotManifestW ))
        {
            /* manifest name does not match directory name, so it's not a global
             * windows/winsxs manifest; use the manifest directory name instead */
            dirlen = p - info->lpAssemblyManifestPath;
            needed = (dirlen + 1) * sizeof(WCHAR) + pnameW->Length;

            status = get_buffer(fullname, needed, CallerBuffer, bAllocateBuffer);
            if (!NT_SUCCESS(status))
                goto done;

            p = *fullname;

            memcpy( p, info->lpAssemblyManifestPath, dirlen * sizeof(WCHAR) );
            p += dirlen;
            memcpy( p, pnameW->Buffer, pnameW->Length);
            p += (pnameW->Length / sizeof(WCHAR));
            *p = L'\0';

            goto done;
        }
    }

    needed = (wcslen(SharedUserData->NtSystemRoot) * sizeof(WCHAR) +
              sizeof(winsxsW) + info->ulAssemblyDirectoryNameLength + pnameW->Length + 2*sizeof(WCHAR));

    status = get_buffer(fullname, needed, CallerBuffer, bAllocateBuffer);
    if (!NT_SUCCESS(status))
        goto done;

    p = *fullname;

    wcscpy( p, SharedUserData->NtSystemRoot );
    p += wcslen(p);
    memcpy( p, winsxsW, sizeof(winsxsW) );
    p += sizeof(winsxsW) / sizeof(WCHAR);
    memcpy( p, info->lpAssemblyDirectoryName, info->ulAssemblyDirectoryNameLength );
    p += info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
    *p++ = L'\\';
    memcpy( p, pnameW->Buffer, pnameW->Length);
    p += (pnameW->Length / sizeof(WCHAR));
    *p = L'\0';

done:
    RtlFreeHeap( RtlGetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    DPRINT("%S\n", fullname);
    return status;
}

/*
 * @unimplemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(IN ULONG Flags,
                                         IN PUNICODE_STRING OriginalName,
                                         IN PUNICODE_STRING Extension,
                                         IN OUT PUNICODE_STRING StaticString,
                                         IN OUT PUNICODE_STRING DynamicString,
                                         IN OUT PUNICODE_STRING *NewName OPTIONAL,
                                         IN PULONG NewFlags OPTIONAL,
                                         IN PSIZE_T FileNameSize OPTIONAL,
                                         IN PSIZE_T RequiredLength OPTIONAL)
{
    NTSTATUS Status;
    LPWSTR fullname;
    WCHAR buffer [MAX_PATH];
    UNICODE_STRING localStr, localStr2, *pstrParam;
    WCHAR *p;
    BOOLEAN GotExtension;
    WCHAR c;
    C_ASSERT(sizeof(UNICODE_NULL) == sizeof(WCHAR));


    /* Check for invalid parameters */
    if (!OriginalName)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!DynamicString && !StaticString)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((DynamicString) && (StaticString) && !(NewName))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!OriginalName->Buffer || OriginalName->Length == 0)
    {
        return STATUS_SXS_KEY_NOT_FOUND;
    }

    if (StaticString && (OriginalName == StaticString || OriginalName->Buffer == StaticString->Buffer))
    {
        return STATUS_SXS_KEY_NOT_FOUND;
    }

    if (NtCurrentPeb()->ProcessParameters &&
        (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PRIVATE_DLL_PATH))
    {
        UNICODE_STRING RealName, LocalName;
        WCHAR RealNameBuf[MAX_PATH], LocalNameBuf[MAX_PATH];

        RtlInitEmptyUnicodeString(&RealName, RealNameBuf, sizeof(RealNameBuf));
        RtlInitEmptyUnicodeString(&LocalName, LocalNameBuf, sizeof(LocalNameBuf));

        Status = RtlComputePrivatizedDllName_U(OriginalName, &RealName, &LocalName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlComputePrivatizedDllName_U failed for %wZ: 0x%lx\n", OriginalName, Status);
            return Status;
        }

        if (RtlDoesFileExists_UStr(&LocalName))
        {
            Status = get_buffer(&fullname, LocalName.Length + sizeof(UNICODE_NULL), StaticString, DynamicString != NULL);
            if (NT_SUCCESS(Status))
            {
                RtlCopyMemory(fullname, LocalName.Buffer, LocalName.Length + sizeof(UNICODE_NULL));
            }
            else
            {
                DPRINT1("Error while retrieving buffer for %wZ: 0x%lx\n", OriginalName, Status);
            }
        }
        else if (RtlDoesFileExists_UStr(&RealName))
        {
            Status = get_buffer(&fullname, RealName.Length + sizeof(UNICODE_NULL), StaticString, DynamicString != NULL);
            if (NT_SUCCESS(Status))
            {
                RtlCopyMemory(fullname, RealName.Buffer, RealName.Length + sizeof(UNICODE_NULL));
            }
            else
            {
                DPRINT1("Error while retrieving buffer for %wZ: 0x%lx\n", OriginalName, Status);
            }
        }
        else
        {
            Status = STATUS_NOT_FOUND;
        }

        if (RealName.Buffer != RealNameBuf)
            RtlFreeUnicodeString(&RealName);
        if (LocalName.Buffer != LocalNameBuf)
            RtlFreeUnicodeString(&LocalName);

        if (NT_SUCCESS(Status))
        {
            DPRINT("Redirecting %wZ to %S\n", OriginalName, fullname);
            if (!StaticString || StaticString->Buffer != fullname)
            {
                RtlInitUnicodeString(DynamicString, fullname);
                if (NewName)
                    *NewName = DynamicString;
            }
            else
            {
                if (NewName)
                    *NewName = StaticString;
            }
            return Status;
        }
    }

    pstrParam = OriginalName;

    /* Get the file name with an extension */
    p = OriginalName->Buffer + OriginalName->Length / sizeof(WCHAR) - 1;
    GotExtension = FALSE;
    while (p >= OriginalName->Buffer)
    {
        c = *p--;
        if (c == L'.')
        {
            GotExtension = TRUE;
        }
        else if (c == L'\\')
        {
            localStr.Buffer = p + 2;
            localStr.Length = OriginalName->Length - ((ULONG_PTR)localStr.Buffer - (ULONG_PTR)OriginalName->Buffer);
            localStr.MaximumLength = OriginalName->MaximumLength - ((ULONG_PTR)localStr.Buffer - (ULONG_PTR)OriginalName->Buffer);
            pstrParam = &localStr;
            break;
        }
    }

    if (!GotExtension)
    {
        if (!Extension)
        {
            return STATUS_SXS_KEY_NOT_FOUND;
        }

        if (pstrParam->Length + Extension->Length > sizeof(buffer))
        {
            //FIXME!
            DPRINT1("%s doesn't support reallocation for now (%llu > %llu), failing!", __FUNCTION__, pstrParam->Length + Extension->Length, sizeof(buffer));
            return STATUS_NO_MEMORY;
        }

        RtlInitEmptyUnicodeString(&localStr2, buffer, sizeof(buffer));
        RtlAppendUnicodeStringToString(&localStr2, pstrParam);
        RtlAppendUnicodeStringToString(&localStr2, Extension);
        pstrParam = &localStr2;
    }

    /* Use wine's function as long as we use wine's sxs implementation in ntdll */
    Status = find_actctx_dll(pstrParam, &fullname, StaticString, DynamicString != NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("Redirecting %wZ to %S\n", OriginalName, fullname);

    if (!StaticString || StaticString->Buffer != fullname)
    {
        RtlInitUnicodeString(DynamicString, fullname);
        if (NewName)
            *NewName = DynamicString;
    }
    else
    {
        if (NewName)
            *NewName = StaticString;
    }

    return Status;
}

/*
 * @implemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirection(IN BOOLEAN Wow64FsEnableRedirection)
{
    /* This is what Windows returns on x86 */
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirectionEx(IN PVOID Wow64FsEnableRedirection,
                              OUT PVOID *OldFsRedirectionLevel)
{
    /* This is what Windows returns on x86 */
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlComputeImportTableHash(IN HANDLE FileHandle,
                          OUT PCHAR Hash,
                          IN ULONG ImportTableHashSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlpSafeCopyMemory(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
   _In_ SIZE_T Length)
{
    _SEH2_TRY
    {
        RtlCopyMemory(Destination, Source, Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

/* FIXME: code duplication with kernel32/client/time.c */
ULONG
NTAPI
RtlGetTickCount(VOID)
{
    ULARGE_INTEGER TickCount;

#ifdef _WIN64
    TickCount.QuadPart = *((volatile ULONG64*)&SharedUserData->TickCount);
#else
    while (TRUE)
    {
        TickCount.HighPart = (ULONG)SharedUserData->TickCount.High1Time;
        TickCount.LowPart = SharedUserData->TickCount.LowPart;

        if (TickCount.HighPart == (ULONG)SharedUserData->TickCount.High2Time)
            break;

        YieldProcessor();
    }
#endif

    return (ULONG)((UInt32x32To64(TickCount.LowPart,
                                  SharedUserData->TickCountMultiplier) >> 24) +
                    UInt32x32To64((TickCount.HighPart << 8) & 0xFFFFFFFF,
                                  SharedUserData->TickCountMultiplier));
}


/*
 * @implemented
 */
HANDLE
WINAPI
GetCurrentProcess(VOID)
{
    return (HANDLE)NtCurrentProcess();
}

/*
 * @implemented
 */
HANDLE
WINAPI
GetCurrentThread(VOID)
{
    return (HANDLE)NtCurrentThread();
}

/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentProcessId(VOID)
{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentThreadId(VOID)
{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread);
}

/*
 * @implemented
 */
HANDLE
WINAPI
GetProcessHeap(VOID)
{
    /* Call the RTL API */
    return RtlGetProcessHeap();
}


/*
 * @implemented
 */
VOID
WINAPI
RaiseException(IN DWORD dwExceptionCode,
               IN DWORD dwExceptionFlags,
               IN DWORD nNumberOfArguments,
               IN CONST ULONG_PTR *lpArguments OPTIONAL)
{
    EXCEPTION_RECORD ExceptionRecord;

    /* Setup the exception record */
    ExceptionRecord.ExceptionCode = dwExceptionCode;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)RaiseException;
    ExceptionRecord.ExceptionFlags = dwExceptionFlags & EXCEPTION_NONCONTINUABLE;

    /* Check if we have arguments */
    if (!lpArguments)
    {
        /* We don't */
        ExceptionRecord.NumberParameters = 0;
    }
    else
    {
        /* We do, normalize the count */
        if (nNumberOfArguments > EXCEPTION_MAXIMUM_PARAMETERS)
            nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;

        /* Set the count of parameters and copy them */
        ExceptionRecord.NumberParameters = nNumberOfArguments;
        RtlCopyMemory(ExceptionRecord.ExceptionInformation,
                      lpArguments,
                      nNumberOfArguments * sizeof(ULONG));
    }

    /* Better handling of Delphi Exceptions... a ReactOS Hack */
    if (dwExceptionCode == 0xeedface || dwExceptionCode == 0xeedfade)
    {
        DPRINT1("Delphi Exception at address: %p\n", ExceptionRecord.ExceptionInformation[0]);
        DPRINT1("Exception-Object: %p\n", ExceptionRecord.ExceptionInformation[1]);
        DPRINT1("Exception text: %lx\n", ExceptionRecord.ExceptionInformation[2]);
    }

    /* Trace the wine special error and show the modulename and functionname */
    if (dwExceptionCode == 0x80000100 /* EXCEPTION_WINE_STUB */)
    {
        /* Numbers of parameter must be equal to two */
        if (ExceptionRecord.NumberParameters == 2)
        {
            DPRINT1("Missing function in   : %s\n", ExceptionRecord.ExceptionInformation[0]);
            DPRINT1("with the functionname : %s\n", ExceptionRecord.ExceptionInformation[1]);
        }
    }

    /* Raise the exception */
    RtlRaiseException(&ExceptionRecord);
}

/*
 * @implemented
 */
VOID
WINAPI
OutputDebugStringW(IN LPCWSTR OutputString)
{
    DbgPrint("%ws\n", OutputString);
}

/*
 * @implemented
 */
VOID
WINAPI
OutputDebugStringA(IN LPCSTR _OutputString)
{
    DbgPrint("%s\n", _OutputString);
}

DWORD
WINAPI
GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
    return TIME_ZONE_ID_INVALID;
}

int __cdecl __acrt_WideCharToMultiByte(
    _In_                           UINT    _CodePage,
    _In_                           DWORD   _DWFlags,
    _In_                           LPCWSTR _LpWideCharStr,
    _In_                           int     _CchWideChar,
    _Out_writes_opt_(_CbMultiByte) LPSTR   _LpMultiByteStr,
    _In_                           int     _CbMultiByte,
    _In_opt_                       LPCSTR  _LpDefaultChar,
    _Out_opt_                      LPBOOL  _LpUsedDefaultChar
    )
{
    size_t Size = 0;

    return wcstombs_s(&Size, _LpMultiByteStr, _CbMultiByte, _LpWideCharStr, _CchWideChar)
        ? /* Failure */ 0
        : /* Success */ Size;
}

int __cdecl __acrt_MultiByteToWideChar(
    _In_                           UINT    _CodePage,
    _In_                           DWORD   _DWFlags,
    _In_                           LPCSTR  _LpMultiByteStr,
    _In_                           int     _CbMultiByte,
    _Out_writes_opt_(_CchWideChar) LPWSTR  _LpWideCharStr,
    _In_                           int     _CchWideChar
    )
{
    size_t Size = 0;

    return mbstowcs_s(&Size, _LpWideCharStr, _CchWideChar, _LpMultiByteStr, _CbMultiByte)
        ? /* Failure */ 0
        : /* Success */ Size;
}

/* EOF */
