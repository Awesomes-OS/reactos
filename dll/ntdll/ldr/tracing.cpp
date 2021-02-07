#include "ldrp.h"
#include "tracing.hpp"
#include <unordered_map>
#include <forward_list>

std::unordered_map<ULONG_PTR, std::forward_list<ILoaderDataWatch*>> watches;


ILoaderDataWatch* LdrpGetDataWatch(ULONG_PTR pointer)
{
    auto&& it = watches.find(static_cast<ULONG_PTR>(pointer));
    if (it == watches.end())
        return nullptr;

    auto* watch = it->second.front();

    ASSERT(watch);

    if (watch->freed())
        return nullptr;

    return watch;
}

template <typename T>
LoaderDataWatch<T>::LoaderDataWatch(T* pointer) : ILoaderDataWatch{}, pointer{ pointer }
{
    if (auto&&[it, success] = watches.try_emplace(static_cast<ULONG_PTR>(pointer), { this }); !success)
    {
        it->second.push_front(this);
    }

    copy();
}

template <typename T>
void LoaderDataWatch<T>::free()
{
    freed_ = true;
}

volatile ULONG32 LdrpFlsBlockIndex = 0;

PVOID NTAPI LdrpFlsGetValue(PredefinedTlsSlots slot)
{
    PVOID* Table = nullptr;
    ULONG32 Index = LdrpFlsBlockIndex;

    if (Index)
    {
        if (NT_SUCCESS(RtlFlsGetValue(Index, reinterpret_cast<PVOID*>(&Table))))
        {
            return Table[static_cast<ULONG_PTR>(slot)];
        }

        return nullptr;
    }

    Table = new (std::nothrow) PVOID[static_cast<ULONG_PTR>(PredefinedTlsSlots::MAX_PREDEFINED_TLS_SLOT)];
    if (Table)
    {
        if (NT_SUCCESS(RtlFlsAlloc(nullptr, &Index)) && Index)
        {
            if (NT_SUCCESS(RtlFlsSetValue(Index, Table)))
            {
                if (InterlockedCompareExchange(reinterpret_cast<volatile long*>(&LdrpFlsBlockIndex), Index, 0) == 0)
                {
                    return nullptr;
                }
            }

            RtlFlsFree(Index);
        }

        delete[] Table;
    }

    return nullptr;
}

BOOL NTAPI LdrpFlsSetValue(PredefinedTlsSlots slot, PVOID data)
{
    PVOID* Table = nullptr;
    ULONG32 Index = LdrpFlsBlockIndex;
    const ULONG_PTR SlotIndex = static_cast<ULONG_PTR>(slot);

    if (Index)
    {
        if (NT_SUCCESS(RtlFlsGetValue(Index, reinterpret_cast<PVOID*>(&Table))))
        {
            Table[SlotIndex] = data;
            return TRUE;
        }

        return FALSE;
    }

    Table = new (std::nothrow) PVOID[static_cast<ULONG_PTR>(PredefinedTlsSlots::MAX_PREDEFINED_TLS_SLOT)];
    if (Table)
    {
        if (NT_SUCCESS(RtlFlsAlloc(nullptr, &Index)) && Index)
        {
            if (NT_SUCCESS(RtlFlsSetValue(Index, Table)))
            {
                Table[SlotIndex] = data;
                LONG32 NewIndex = InterlockedCompareExchange(reinterpret_cast<volatile long*>(&LdrpFlsBlockIndex), Index, 0);
                if (NewIndex == 0)
                {
                    return TRUE;
                }

                RtlFlsFree(Index);
                delete[] Table;

                Index = NewIndex;

                if (NT_SUCCESS(RtlFlsGetValue(Index, reinterpret_cast<PVOID*>(&Table))))
                {
                    Table[SlotIndex] = data;
                    return TRUE;
                }

                return FALSE;
            }

            RtlFlsFree(Index);
        }

        delete[] Table;
    }

    return FALSE;
}

// This is a drastic shutoff toggle that forces all new threads to fail their CLRInitDebugState calls.
// We only invoke this if FLS can't allocate its master block, preventing us from tracking the shutoff
// on a per-thread basis.
BYTE* GetGlobalContractShutoffFlag()
{
    static BYTE gGlobalContractShutoffFlag = 0;
    return &gGlobalContractShutoffFlag;
}

static BOOL AreContractsShutoff()
{
    BYTE* pShutoff = GetGlobalContractShutoffFlag();
    if (!pShutoff)
    {
        return FALSE;
    }

    return *pShutoff != FALSE;
}

static VOID ShutoffContracts()
{
    BYTE* pShutoff = GetGlobalContractShutoffFlag();
    if (pShutoff)
    {
        *pShutoff = TRUE;
    }
}

ClrDebugState* CLRInitDebugState()
{
    // workaround!
    //
    // The existing Fls apis didn't provide the support we need and adding support cleanly is
    // messy because of the brittleness of IExecutionEngine.
    //
    // To understand this function, you need to know that the Fls routines have special semantics
    // for the TlsIdx_ClrDebugState slot:
    //
    //  - FlsSetValue will never throw. If it fails due to OOM on creation of the slot storage,
    //    it will silently bail. Thus, we must do a confirming FlsGetValue before we can conclude
    //    that the SetValue succeeded.
    //
    //  - FlsAssociateCallback will not complain about multiple sets of the callback.
    //
    //  - The mscorwks implemention of FlsAssociateCallback will ignore the passed in value
    //    and use the version of FreeClrDebugState compiled into mscorwks. This is needed to
    //    avoid dangling pointer races on shutdown.


    // This is our global "bad" debug state that thread use when they OOM on CLRInitDebugState.
    // We really only need to initialize it once but initializing each time is convenient
    // and has low perf impact.
    static ClrDebugState gBadClrDebugState;
    ClrDebugState* pNewClrDebugState = NULL;
    ClrDebugState* pClrDebugState = NULL;

    if (AreContractsShutoff())
    {
        pNewClrDebugState = NULL;
    }
    else
    {
        pNewClrDebugState = new (std::nothrow) ClrDebugState{};
        if (pNewClrDebugState != NULL)
        {
            // Both allocations succeeded, so initialize the structures, and have
            // pNewClrDebugState point to pNewLockData.  If either of the allocations
            // failed, we'll use gBadClrDebugState for this thread, and free whichever of
            // pNewClrDebugState or pNewLockData actually did get allocated (if either did).
            // (See code in this function below, outside this block.)
        }
    }
    // This is getting really diseased. All the one-time host init stuff inside the ClrFlsStuff could actually
    // have caused mscorwks contracts to be executed since the last time we actually checked to see if the ClrDebugState
    // needed creating.
    //
    // So we must make one last check to see if the ClrDebugState still needs creating.
    //
    auto* pTmp = static_cast<ClrDebugState*>(LdrpFlsGetValue(PredefinedTlsSlots::TlsIdx_ClrDebugState));
    if (pTmp != NULL)
    {
        // Recursive call set up ClrDebugState for us
        pClrDebugState = pTmp;
    }
    else if ((pNewClrDebugState != NULL))
    {
        // Normal case: our new ClrDebugState will be the one we just allocated.
        // Note that we require BOTH the ClrDebugState and the DbgStateLockData
        // structures to have been successfully allocated for contracts to be
        // enabled for this thread.
        pClrDebugState = pNewClrDebugState;
    }
    else
    {
        // OOM case: HeapAlloc of newClrDebugState failed.
        pClrDebugState = &gBadClrDebugState;
    }

    _ASSERTE(pClrDebugState != NULL);

    LdrpFlsSetValue(PredefinedTlsSlots::TlsIdx_ClrDebugState, static_cast<PVOID>(pClrDebugState));

    // For the ClrDebugState index, ClrFlsSetValue does *not* throw on OOM.
    // Instead, it silently throws away the value. So we must now do a confirming
    // FlsGet to learn if our Set succeeded.
    if (LdrpFlsGetValue(PredefinedTlsSlots::TlsIdx_ClrDebugState) == nullptr)
    {
        // Our FlsSet didn't work. That means it couldn't allocate the master FLS block for our thread.
        // Now we're a bad state because not only can't we succeed, we can't record that we didn't succeed.
        // And it's invalid to return a BadClrDebugState here only to return a good debug state later.
        //
        // So we now take the drastic step of forcing all future ClrInitDebugState calls to return the OOM state.
        ShutoffContracts();
        pClrDebugState = &gBadClrDebugState;

        // Try once more time to set the FLS (if it doesn't work, the next call will keep cycling through here
        // until it does succeed.)
        LdrpFlsSetValue(PredefinedTlsSlots::TlsIdx_ClrDebugState, &gBadClrDebugState);
    }

#if defined(_DEBUG)
    // The ClrDebugState we allocated above made it into FLS iff
    //      the DbgStateLockData we allocated above made it into
    //      the FLS's ClrDebugState::m_pLockData
    // These debug-only checks enforce this invariant
    if (pClrDebugState != NULL)
    {
        // If we're here, then typically pClrDebugState is what's in FLS.  However,
        // it's possible that pClrDebugState is gBadClrDebugState, and FLS is NULL
        // (if the last ClrFlsSetValue() failed).  Either way, our checks below
        // are valid ones to make.
    }
#endif //_DEBUG
    if (pNewClrDebugState != NULL && pClrDebugState != pNewClrDebugState)
    {
        // We allocated a ClrDebugState which didn't make it into FLS, so free it.
        delete pNewClrDebugState;
    }
    // Not necessary as TLS slots are born NULL and potentially problematic for OOM cases as we can't
    // take an exception here.
    //ClrFlsSetValue(TlsIdx_OwnedCrstsChain, NULL);
    return pClrDebugState;
}

#ifdef _DEBUG
size_t CHECK::s_cLeakedBytes = 0;
size_t CHECK::s_cNumFailures = 0;

// todo(andrew.boyarshin): thread_local to static
//thread_local LONG CHECK::t_count;
#endif

BOOL CHECK::s_neverEnforceAsserts = 0;


// Trigger triggers the actual check failure.  The trigger may provide a reason
// to include in the failure message.

void CHECK::Trigger(LPCSTR reason)
{
#if 0
    const char *messageString = NULL;
    NewHolder<StackScratchBuffer> pScratch(NULL);
    NewHolder<StackSString> pMessage(NULL);

    EX_TRY
    {
        FAULT_NOT_FATAL();

        pScratch = new StackScratchBuffer();
        pMessage = new StackSString();

        pMessage->AppendASCII(reason);
        pMessage->AppendASCII(": ");
        if (m_message != NULL)
            pMessage->AppendASCII((m_message != (LPCSTR)1) ? m_message : "<runtime check failure>");

#if _DEBUG
        pMessage->AppendASCII("FAILED: ");
        pMessage->AppendASCII(m_condition);
#endif

        messageString = pMessage->GetANSI(*pScratch);
    }
    EX_CATCH
    {
        messageString = "<exception occurred while building failure description>";
    }
    EX_END_CATCH(SwallowAllExceptions);

#if _DEBUG
    DbgAssertDialog((char*)m_file, m_line, (char *)messageString);
#else
    OutputDebugStringA(messageString);
    DebugBreak();
#endif
#endif
}

#ifdef _DEBUG
// Setup records context info after a failure.

void CHECK::Setup(LPCSTR message, LPCSTR condition, LPCSTR file, INT line)
{
#if 0
    //
    // It might be nice to collect all of the message here.  But for now, we will just
    // retain the innermost one.
    //

    if (m_message == NULL)
    {
        m_message = message;
        m_condition = condition;
        m_file = file;
        m_line = line;
    }

#ifdef _DEBUG
    else if (IsInAssert())
    {
        EX_TRY
        {
            FAULT_NOT_FATAL();
            // Try to build a stack of condition failures

            StackSString context;
            context.Printf("%s\n\t%s%s FAILED: %s\n\t\t%s, line: %d",
                           m_condition,
                           message && *message ? message : "",
                           message && *message ? ": " : "",
                           condition,
                           file, line);

            m_condition = AllocateDynamicMessage(context);
        }
        EX_CATCH
        {
            // If anything goes wrong, we don't push extra context
        }
        EX_END_CATCH(SwallowAllExceptions)
    }
#endif

#if defined(_DEBUG_IMPL)
    if (IsInAssert() && IsDebuggerPresent())
    {
        DebugBreak();
    }
#endif
#endif
}

LPCSTR CHECK::FormatMessage(LPCSTR messageFormat, ...)
{
    LPCSTR result = NULL;
#if 0

    // We never delete this allocated string. A dtor would conflict with places
    // we use this around SEH stuff. We could have some fancy stack-based allocator,
    // but that's too much work for now. In fact we believe that leaking is a reasonable
    // policy, since allocations will only happen on a failed assert, and a failed assert
    // will generally be fatal to the process.

    // The most common place for format strings will be in an assert; in
    // which case we don't care about leaking.
    // But to be safe, if we're not-inside an assert, then we'll just use
    // the format string literal to avoid allocated (and leaking) any memory.

    CHECK chk;
    if (!chk.IsInAssert())
        result = messageFormat;
    else
    {
        // This path is only run in debug.  TakesLockViolation suppresses
        // problems with SString below.
        CONTRACT_VIOLATION(FaultNotFatal|TakesLockViolation);

        EX_TRY
        {
            SUPPRESS_ALLOCATION_ASSERTS_IN_THIS_SCOPE;

            // Format it.
            va_list args;
            va_start( args, messageFormat);

            SString s;
            s.VPrintf(messageFormat, args);

            va_end(args);

            result = AllocateDynamicMessage(s);

        }
        EX_CATCH
        {
            // If anything goes wrong, just use the format string.
            result = messageFormat;
        }
        EX_END_CATCH(SwallowAllExceptions)
    }
#endif

    return result;
}

LPCSTR CHECK::AllocateDynamicMessage(const SString &s)
{
#if 0
    // Make a copy of it.
    StackScratchBuffer buffer;
    const char * pMsg = s.GetANSI(buffer);

    // Must copy that into our own field.
    size_t len = strlen(pMsg) + 1;
    char * p = new char[len];
    strcpy(p, pMsg);

    // But we'll keep counters of how much we're leaking for diagnostic purposes.
    s_cLeakedBytes += len;
    s_cNumFailures ++;

    // This should never fire.
    // Note use an ASSERTE (not a check) to avoid a recursive deadlock.
    _ASSERTE(s_cLeakedBytes < 10000 || !"Warning - check misuse - leaked over 10,000B due to unexpected usage pattern");
    return p;
#else
    return NULL;
#endif
}

#endif
