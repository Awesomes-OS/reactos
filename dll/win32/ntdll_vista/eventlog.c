/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Windows Event Tracing implementation
 */

#include <rtl_vista.h>

#include <wmistr.h>
#include <evntprov.h>
#include <evntrace.h>
#include <malloc.h>

#define NDEBUG
#include <debug.h>

#define FIXME DPRINT1
#define debugstr_a
#define debugstr_w
#define debugstr_wn
#define wine_dbgstr_w
#define wine_dbgstr_longlong

/* WINE ROUTINES COPY (ntdll can't depend on wine) */
static __inline const char *ros_dbg_sprintf(const char *format, ...)
{
    static const int max_size = 200;
    va_list valist;
    char* ret;
    int len;

    va_start(valist, format);
    len = snprintf(NULL, 0, format, valist);
    ++len;
    // never freed (in Wine buffers are taken from pool)
    ret = RtlAllocateHeap(
        NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap,
        HEAP_ZERO_MEMORY,
        len * sizeof(char));
    if (!ret) return ret;
    len = snprintf(ret, len, format, valist);
    if (len == -1 || len >= max_size) ret[max_size - 1] = 0;
    va_end(valist);
    return ret;
}

static __inline const char *ros_dbgstr_guid(const GUID *id)
{
    if (!id) return "(null)";
    if (!((ULONG_PTR)id >> 16)) return ros_dbg_sprintf("<guid-0x%04lx>", (ULONG_PTR)id & 0xffff);
    return ros_dbg_sprintf("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        id->Data1, id->Data2, id->Data3,
        id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
        id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
}

/******************************************************************************
 *                  EtwEventRegister (NTDLL.@)
 */
ULONG WINAPI EtwEventRegister( LPCGUID provider, PENABLECALLBACK callback, PVOID context,
                PREGHANDLE handle )
{
    FIXME("(%s, %p, %p, %p) stub.\n", ros_dbgstr_guid(provider), callback, context, handle);

    *handle = 0xdeadbeef;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventUnregister (NTDLL.@)
 */
ULONG WINAPI EtwEventUnregister( REGHANDLE handle )
{
    FIXME("(%s) stub.\n", wine_dbgstr_longlong(handle));
    return ERROR_SUCCESS;
}

/*********************************************************************
 *                  EtwEventSetInformation   (NTDLL.@)
 */
ULONG WINAPI EtwEventSetInformation( REGHANDLE handle, EVENT_INFO_CLASS class, void *info,
                                     ULONG length )
{
    FIXME("(%s, %u, %p, %u) stub\n", wine_dbgstr_longlong(handle), class, info, length);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwRegisterTraceGuidsW (NTDLL.@)
 *
 * Register an event trace provider and the event trace classes that it uses
 * to generate events.
 *
 * PARAMS
 *  RequestAddress     [I]   ControlCallback function
 *  RequestContext     [I]   Optional provider-defined context
 *  ControlGuid        [I]   GUID of the registering provider
 *  GuidCount          [I]   Number of elements in the TraceGuidReg array
 *  TraceGuidReg       [I/O] Array of TRACE_GUID_REGISTRATION structures
 *  MofImagePath       [I]   not supported, set to NULL
 *  MofResourceName    [I]   not supported, set to NULL
 *  RegistrationHandle [O]   Provider's registration handle
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: System error code
 */
ULONG WINAPI EtwRegisterTraceGuidsW( WMIDPREQUEST RequestAddress,
                void *RequestContext, const GUID *ControlGuid, ULONG GuidCount,
                TRACE_GUID_REGISTRATION *TraceGuidReg, const WCHAR *MofImagePath,
                const WCHAR *MofResourceName, TRACEHANDLE *RegistrationHandle )
{
    FIXME("(%p, %p, %s, %u, %p, %s, %s, %p): stub\n", RequestAddress, RequestContext,
        ros_dbgstr_guid(ControlGuid), GuidCount, TraceGuidReg, debugstr_w(MofImagePath),
          debugstr_w(MofResourceName), RegistrationHandle);

    if (TraceGuidReg)
    {
        ULONG i;
        for (i = 0; i < GuidCount; i++)
        {
            FIXME("  register trace class %s\n", ros_dbgstr_guid(TraceGuidReg[i].Guid));
            TraceGuidReg[i].RegHandle = (HANDLE)0xdeadbeef;
        }
    }
    *RegistrationHandle = (TRACEHANDLE)0xdeadbeef;
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwRegisterTraceGuidsA (NTDLL.@)
 */
ULONG WINAPI EtwRegisterTraceGuidsA( WMIDPREQUEST RequestAddress,
                void *RequestContext, const GUID *ControlGuid, ULONG GuidCount,
                TRACE_GUID_REGISTRATION *TraceGuidReg, const char *MofImagePath,
                const char *MofResourceName, TRACEHANDLE *RegistrationHandle )
{
    FIXME("(%p, %p, %s, %u, %p, %s, %s, %p): stub\n", RequestAddress, RequestContext,
        ros_dbgstr_guid(ControlGuid), GuidCount, TraceGuidReg, debugstr_a(MofImagePath),
          debugstr_a(MofResourceName), RegistrationHandle);
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwUnregisterTraceGuids (NTDLL.@)
 */
ULONG WINAPI EtwUnregisterTraceGuids( TRACEHANDLE RegistrationHandle )
{
    if (!RegistrationHandle)
         return ERROR_INVALID_PARAMETER;

    FIXME("%s: stub\n", wine_dbgstr_longlong(RegistrationHandle));
    return ERROR_SUCCESS;
}

/******************************************************************************
 *                  EtwEventEnabled (NTDLL.@)
 */
BOOLEAN WINAPI EtwEventEnabled( REGHANDLE handle, const EVENT_DESCRIPTOR *descriptor )
{
    FIXME("(%s, %p): stub\n", wine_dbgstr_longlong(handle), descriptor);
    return FALSE;
}

/******************************************************************************
 *                  EtwEventWrite (NTDLL.@)
 */
ULONG WINAPI EtwEventWrite( REGHANDLE handle, const EVENT_DESCRIPTOR *descriptor, ULONG count,
    EVENT_DATA_DESCRIPTOR *data )
{
    FIXME("(%s, %p, %u, %p): stub\n", wine_dbgstr_longlong(handle), descriptor, count, data);
    return ERROR_SUCCESS;
}