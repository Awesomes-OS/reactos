// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

#include <string_view>
#include <typeinfo>
#include "fmt/format.h"

//--------------------------------------------------------------------------------
// CONTRACTS - User Reference
//
//   A CONTRACT is a container for a set of checked declarations about a
// function.  Besides giving developers a "laundry list" of checks to
// make checking more complete, contracts compile these checks
// as hidden annotations into the checked executable that our static scanner
// uses to detect violations automatically.
//
//   The following annotations can appear in contracts:
//
//
//      THROWS          an exception might be thrown out of the function
//      -or- NOTHROW    an exception will NOT be thrown out of the function
//
//      CAN_TAKE_LOCK           the function has a code path that takes a lock
//      _or_ (CAN_TAKE_LOCK and CANNOT_RETAKE_LOCK)
//                              the function has a code path that takes a lock, but never tries to reenter
//                              locks held at the time this function was called.
//      -or- CANNOT_TAKE_LOCK   the function will never allow a lock to be taken
//      -or-                    the default is WRAPPER(CAN_TAKE_LOCK).  i.e., if any callees take locks,
//                              then it's ok for this function to as well.  If LIMITED_METHOD_CONTRACT is specified,
//                              however, then CANNOT_TAKE_LOCK is assumed.
//
//      PRECONDITION(X) -   generic CHECK or BOOL expression which should be true
//                          on function entry
//
//      POSTCONDITION(X) -  generic CHECK or BOOL expression which should be true
//                          on function entry.  Note that variable RETVAL will be
//                          available for use in the expression.
//
//      INSTANCE_CHECK -    equivalent of:
//                          PRECONDITION(CheckPointer(this));
//                          POSTCONDITION(CheckInvariant(this));
//      INSTANCE_CHECK_NULL - equivalent of:
//                          PRECONDITION(CheckPointer(this, NULL_OK));
//                          POSTCONDITION(CheckInvariant(this, NULL_OK));
//      CONSTRUCTOR_CHECK - equivalent of:
//                          POSTCONDITION(CheckPointer(this));
//      DESTRUCTOR_CHECK -  equivalent of:
//                          PRECONDITION(CheckPointer(this));
//
//   Contracts come in the following flavors:
//
//     Dynamic:
//        CONTRACTL          the standard version used for all dynamic contracts
//                           except those including postconditions.
//
//        CONTRACT(rettype)  an uglier version of CONTRACTL that's unfortunately
//                           needed to support postconditions. You must specify
//                           the correct return type and it cannot be "void."
//                           (Use CONTRACT_VOID instead) You must use the
//                           RETURN macro rather than the "return" keyword.
//
//        CONTRACT_VOID      you can't supply "void" to a CONTRACT - use this
//                           instead.
//
//   WRAPPER(annotation)
//
// When a function does not explicitly caused a condition, use the WRAPPER macro around
// the declaration.  This implies that the function is dependent on the functions it calls
// for its behaviour, and guarantees nothing.
//
//   CONTRACT_VIOLATION(violationmask):
//
//        A bandaid used to suppress contract assertions. A contract violation
//        is always a bug and you're expected to remove it before shipping.
//        If a violation cannot be fixed immediately, however, it's better
//        to use this on the offending callsite than to disable a contract entirely.
//
//        The violationmask can be one or more of the following OR'd together.
//
//              ThrowsViolation
//              TakesLockViolation
//
//        The associated assertion will be suppressed until you leave the scope
//        containing the CONTRACT_VIOLATION. Note, however, that any called
//        function that redeclares the associated annotation reinstates
//        the assert for the scope of *its* call. This prevents a CONTRACT_VIOLATION
//        placed at the root of a calltree from decimating our entire protection.
//
//   WRAPPER_NO_CONTRACT
//
//        A do-nothing contract used by functions that trivially wrap another.
//
//--------------------------------------------------------------------------------

// We only enable contracts in _DEBUG builds
#if defined(_DEBUG) && !defined(DISABLE_CONTRACTS)
#define ENABLE_CONTRACTS_DATA
#endif

#ifndef NOINLINE
#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#elif defined __GNUC__
#define NOINLINE __attribute__ ((noinline))
#else
#define NOINLINE
#endif
#endif // !NOINLINE

#ifndef DEBUG_NOINLINE
#if defined(_DEBUG)
#define DEBUG_NOINLINE NOINLINE
#else
#define DEBUG_NOINLINE
#endif
#endif

#ifndef NOTHROW_DECL
#ifdef _MSC_VER
#define NOTHROW_DECL __declspec(nothrow)
#else
#define NOTHROW_DECL __attribute__((nothrow))
#endif // !_MSC_VER
#endif // !NOTHROW_DECL

#ifdef _DEBUG

#ifdef _MSC_VER
// Make sure we can recurse deep enough for FORCEINLINE
#pragma inline_recursion(on)
#pragma inline_depth(16)
#pragma warning(disable:4714)
#endif // _MSC_VER

#if !defined(DISABLE_CONTRACTS)
#define CHECK_INVARIANTS 1
#define VALIDATE_OBJECTS 1
#endif

#endif  // _DEBUG

#if defined(_DEBUG) && !defined(DACCESS_COMPILE)
#define _DEBUG_IMPL 1
#endif

#ifdef _DEBUG
#define DEBUG_ARG(x)  , x
#else
#define DEBUG_ARG(x)
#endif

void
NTAPI
DbgOutput(PCCH String);

#define DbgPrint FAIL
#define DbgPrintEx FAIL
#undef DPRINT
#undef DPRINT1
#undef RTL_VERIFY
#undef RTL_VERIFYMSG
#undef ASSERT
#undef ASSERTMSG

//--------------------------------------------------------------------------------
// A CHECK is an object which encapsulates a potential assertion
// failure.  It not only contains the result of the check, but if the check fails,
// also records information about the condition and call site.
//
// CHECK also serves as a holder to prevent recursive CHECKS. These can be
// particularly common when putting preconditions inside predicates, especially
// routines called by an invariant.
//
// Note that using CHECK is perfectly efficient in a free build - the CHECK becomes
// a simple string constant pointer (typically either NULL or (LPCSTR)1, although some
// check failures may include messages)
//
// NOTE: you should NEVER use the CHECK class API directly - use the macros below.
//--------------------------------------------------------------------------------

class SString;

class CHECK
{
protected:
    // On retail, this is a pointer to a string literal, null or (LPCSTR)1.
    // On debug, this is a pointer to dynamically allocated memory - that
    // lets us have formatted strings in debug builds.
    LPCSTR  m_message;

#ifdef _DEBUG
    LPCSTR  m_condition;
    LPCSTR  m_file;
    INT     m_line;
    LONG    *m_pCount;

    // Keep leakage counters.
    static  size_t s_cLeakedBytes;
    static  size_t s_cNumFailures;

    static thread_local LONG t_count;
#endif

    static BOOL s_neverEnforceAsserts;

public: // !!! NOTE: Called from macros only!!!

    // If we are not in a check, return TRUE and PushCheck; otherwise return FALSE
    BOOL EnterAssert();

    // Pops check count
    void LeaveAssert();

    // Just return if we are in a check
    BOOL IsInAssert();

    // Should we skip enforcing asserts
    static BOOL EnforceAssert();

    static BOOL EnforceAssert_StaticCheckOnly();

    static void ResetAssert();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4702) // Disable bogus unreachable code warning
#endif // _MSC_VER
    CHECK() : m_message(NULL)
#ifdef _DEBUG
              , m_condition (NULL)
              , m_file(NULL)
              , m_line(NULL)
              , m_pCount(NULL)
#endif
    {}
#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

    // Fail records the result of a condition check.  Can take either a
    // boolean value or another check result
    BOOL Fail(BOOL condition);
    BOOL Fail(const CHECK &check);

    // Setup records context info after a failure.
    void Setup(LPCSTR message DEBUG_ARG(LPCSTR condition) DEBUG_ARG(LPCSTR file) DEBUG_ARG(INT line));
    static LPCSTR FormatMessage(LPCSTR messageFormat, ...);

    // Trigger triggers the actual check failure.  The trigger may provide a reason
    // to include in the failure message.
    void Trigger(LPCSTR reason);

    // Finally, convert to a BOOL to allow just testing the result of a Check function
    operator BOOL();

    BOOL operator!();

    CHECK &operator()() { return *this; }

    static inline const CHECK OK() {
        return CHECK();
    }

    static void SetAssertEnforcement(BOOL value);

  private:
#ifdef _DEBUG
    static LPCSTR AllocateDynamicMessage(const SString &s);
#endif
};

//--------------------------------------------------------------------------------
// These CHECK macros are the correct way to propagate an assertion.  These
// routines are designed for use inside "Check" routines.  Such routines may
// be Invariants, Validate routines, or any other assertional predicates.
//
// A Check routine should return a value of type CHECK.
//
// It should consist of multiple CHECK or CHECK_MSG statements (along with appropritate
// control flow) and should end with CHECK_OK() if all other checks pass.
//
// It may contain a CONTRACT_CHECK contract, but this is only appropriate if the
// check is used for non-assertional purposes (otherwise the contract will never execute).
// Note that CONTRACT_CHECK contracts do not support postconditions.
//
// CHECK: Check the given condition, return a CHECK failure if FALSE
// CHECK_MSG: Same, but include a message paramter if the check fails
// CHECK_OK: Return a successful check value;
//--------------------------------------------------------------------------------

#ifdef _DEBUG
#define DEBUG_ONLY_MESSAGE(msg)     msg
#else
// On retail, we don't want to add a bunch of string literals to the image,
// so we just use the same one everywhere.
#define DEBUG_ONLY_MESSAGE(msg)     ((LPCSTR)1)
#endif

#define CHECK_MSG_EX(_condition, _message, _RESULT)                 \
do                                                                  \
{                                                                   \
    CHECK _check;                                                   \
    if (_check.Fail(_condition))                                    \
    {                                                               \
        ENTER_DEBUG_ONLY_CODE;                                      \
        _check.Setup(DEBUG_ONLY_MESSAGE(_message)                   \
            DEBUG_ARG(#_condition)                                  \
            DEBUG_ARG(__FILE__)                                     \
            DEBUG_ARG(__LINE__));                                   \
        _RESULT(_check);                                            \
        LEAVE_DEBUG_ONLY_CODE;                                      \
    }                                                               \
} while (0)

#define RETURN_RESULT(r) return r

#define CHECK_MSG(_condition, _message)                             \
    CHECK_MSG_EX(_condition, _message, RETURN_RESULT)

#define CHECK(_condition)                                           \
    CHECK_MSG(_condition, "")

#define CHECK_MSGF(_condition, _args)                               \
    CHECK_MSG(_condition, CHECK::FormatMessage _args)

#define CHECK_FAIL(_message)                                        \
    CHECK_MSG(FALSE, _message); UNREACHABLE()

#define CHECK_FAILF(_args)                                          \
    CHECK_MSGF(FALSE, _args); UNREACHABLE()

#define CHECK_OK                                                    \
    return CHECK::OK()

//--------------------------------------------------------------------------------
// ASSERT_CHECK is the proper way to trigger a check result.  If the CHECK
// has failed, the diagnostic assertion routines will fire with appropriate
// context information.
//
// Note that the condition may either be a raw boolean expression or a CHECK result
// returned from a Check routine.
//
// Recursion note: ASSERT_CHECKs are only performed if there is no current check in
// progress.
//--------------------------------------------------------------------------------

#define ASSERT_CHECK(_condition, _message, _reason)                 \
do                                                                  \
{                                                                   \
    CHECK _check;                                                   \
    if (_check.EnterAssert())                                       \
    {                                                               \
        ENTER_DEBUG_ONLY_CODE;                                      \
        if (_check.Fail(_condition))                                \
        {                                                           \
            _check.Setup(_message                                   \
                DEBUG_ARG(#_condition)                              \
                DEBUG_ARG(__FILE__)                                 \
                DEBUG_ARG(__LINE__));                               \
            _check.Trigger(_reason);                                \
        }                                                           \
        LEAVE_DEBUG_ONLY_CODE;                                      \
        _check.LeaveAssert();                                       \
    }                                                               \
} while (0)

// ex: ASSERT_CHECKF(1+2==4, "my reason", ("Woah %d", 1+3));
// note that the double parenthesis, the 'args' param below will include one pair of parens.
#define ASSERT_CHECKF(_condition, _reason, _args)                   \
    ASSERT_CHECK(_condition, CHECK::FormatMessage _args, _reason)

#ifdef ENABLE_CONTRACTS_DATA

// We chain these onto a stack to give us a stack trace of contract assertions (useful
// when the bug report doesn't contain valid symbols)

struct ContractStackRecord
{
    ContractStackRecord* m_pNext;
    std::string_view m_szFunction;
    std::string_view m_szFile;
    int m_lineNum;
    std::string_view m_construct; // The syntactic construct that pushed this thing
};

// And here are the predefined slots for accessing TLS from various DLLs of the CLR.
enum class PredefinedTlsSlots : unsigned char
{
    TlsIdx_Check,
    TlsIdx_ClrDebugState,         // Pointer to ClrDebugState* structure

    MAX_PREDEFINED_TLS_SLOT
};

PVOID NTAPI LdrpFlsGetValue(PredefinedTlsSlots);
BOOL NTAPI LdrpFlsSetValue(PredefinedTlsSlots, PVOID);

// Stored in the FLS under TlsIdx_ClrDebugState.
struct ClrDebugState
{
private:
    ContractStackRecord* m_pContractStackTrace = nullptr;

public:
    explicit ClrDebugState() = default;
    
    //--//
    void LinkContractStackTrace(ContractStackRecord* pContractStackTrace)
    {
        pContractStackTrace->m_pNext = m_pContractStackTrace;

        m_pContractStackTrace = pContractStackTrace;
    }

    ContractStackRecord* GetContractStackTrace()
    {
        return m_pContractStackTrace;
    }

    void SetContractStackTrace(ContractStackRecord* pContractStackTrace)
    {
        m_pContractStackTrace = pContractStackTrace;
    }
};

// Create ClrDebugState.
// This routine is not allowed to return NULL. If it can't allocate the memory needed,
// it should return a pointer to a global static ClrDebugState that indicates
// that debug assertions should be skipped.
ClrDebugState* CLRInitDebugState();
ClrDebugState* GetClrDebugState(BOOL fAlloc = TRUE);

// This function returns a ClrDebugState if one has been created, but will not create one itself.
inline ClrDebugState* CheckClrDebugState()
{
    return static_cast<ClrDebugState*>(LdrpFlsGetValue(PredefinedTlsSlots::TlsIdx_ClrDebugState));
}

// Note: This routine will create a ClrDebugState if called for the first time.
// It cannot return NULL (see comment for InitClrDebugState).
inline ClrDebugState* GetClrDebugState(BOOL fAlloc)
{
    ClrDebugState* pState = CheckClrDebugState();

    if (pState)
    {
        return pState;
    }

    if (fAlloc)
    {
        return CLRInitDebugState();
    }

    return NULL;
}

#ifdef __clang_analyzer__
#define ANALYZER_NORETURN __attribute((analyzer_noreturn))
#else
#define ANALYZER_NORETURN
#endif

#endif // ENABLE_CONTRACTS

template <typename T>
constexpr auto type_name() noexcept
{
    std::string_view name = "Error: unsupported compiler", prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void) noexcept";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}

#define LDR_HEX32_FMT "0x{:08X}"
#define LDR_HEX64_FMT "0x{:016X}"
#define LDR_HEXPTR_FMT "0x{:08X}"

template <>
struct fmt::formatter<LDR_DATA_TABLE_ENTRY*> {
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }
    
    template <typename FormatContext>
    auto format(const LDR_DATA_TABLE_ENTRY*& p, FormatContext& ctx) {
        std::wstring_view ws(p->BaseDllName.Buffer, p->BaseDllName.Length / sizeof(WCHAR));
        std::string s(ws);
        return format_to(
            ctx.out(),
            "{{" LDR_HEXPTR_FMT ":\"{}\"}}",
            (ULONG_PTR)p, s
        );
    }
};

enum class LoaderLogLevel : unsigned char
{
    Trace,
    Debug,
    Information,
    Warning,
    Error,
    Fatal
};

class LoaderRingBuffer final
{
public:
    LoaderRingBuffer() = default;
    ~LoaderRingBuffer() = default;
    LoaderRingBuffer(const LoaderRingBuffer& Other) = delete;
    LoaderRingBuffer(LoaderRingBuffer&& Other) noexcept = delete;
    LoaderRingBuffer& operator=(const LoaderRingBuffer& Other) = delete;
    LoaderRingBuffer& operator=(LoaderRingBuffer&& Other) noexcept = delete;
};

extern VOID ANALYZER_NORETURN DbgAssertDialog(const char* szFile, int iLine, const char* szExpr);

template <typename TBuffer>
void LdrpWriteStacktrace(ContractStackRecord* pRec, TBuffer& buffer)
{
    int count = 20;
    BOOL exceptionBuildingStack = FALSE;

    __try
    {
        while (pRec != NULL)
        {
            if (count != 0)
            {
                if (count != 0)
                {
                    count--;
                }
                else
                {
                    // Show that some lines have been skipped
                    fmt::format_to(buffer, "\n                        ...");
                }

                fmt::format_to(buffer,
                    "\n                        {} in {} at \"{}\" @ {}",
                    pRec->m_construct,
                    pRec->m_szFunction,
                    pRec->m_szFile,
                    pRec->m_lineNum
                );
            }

            pRec = pRec->m_pNext;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // We're done trying to walk the stack of contracts. We faulted trying to form the contract stack trace,
        // and that usually means that its corrupted. A common cause of this is having CONTRACTs in functions that
        // never return, but instead do a non-local goto.
        count = 0;
        exceptionBuildingStack = TRUE;
    }

    if (count == 0)
    {
        fmt::format_to(buffer, "\n                        ...");
    }

    if (exceptionBuildingStack)
    {
        fmt::format_to(buffer,
            "\n"
            "\nError forming contract stack. Any contract stack displayed above is correct,"
            "\nbut it's most probably truncated. This is probably due to a CONTRACT in a"
            "\nfunction that does a non-local goto. There are two bugs here:"
            "\n"
            "\n    1) the CONTRACT violation, and"
            "\n    2) the CONTRACT in the function with the non-local goto."
            "\n"
            "\nPlease fix both bugs!"
            "\n"
        );
    }

    fmt::format_to(buffer, "\n\n");
}

template <typename... Args>
void LdrpAssert(std::string_view szExpression, std::string_view szFunction, std::string_view szFile, int lineNum, std::string_view szElaboration, Args&&... args)
{
    // prevent recursion - we use the same mechanism as CHECK, so this will
    // also prevent mutual recursion involving ASSERT_CHECKs
    ULONG_PTR State = reinterpret_cast<ULONG_PTR>(LdrpFlsGetValue(PredefinedTlsSlots::TlsIdx_Check));
    
    if (State || !LdrpFlsSetValue(PredefinedTlsSlots::TlsIdx_Check, reinterpret_cast<PVOID>(1ul)))
        return;

    fmt::basic_memory_buffer<char, 512 * 20 + 2048 + 1024> buffer;

    const auto& vargs = fmt::make_args_checked<Args...>(szElaboration, args...);

    fmt::format_to(buffer, "ASSERTION FAILURE in {} at \"{}\" @ {}\n\n{}\n", szFunction, szFile, lineNum, szExpression);
    if (!szElaboration.empty())
    {
        const auto SizeBeforeElaboration = buffer.size();
        fmt::vformat_to(buffer, szElaboration, vargs);
        if (SizeBeforeElaboration != buffer.size())
            fmt::format_to(buffer, "\n");
    }

    ContractStackRecord* pRec = CheckClrDebugState() ? CheckClrDebugState()->GetContractStackTrace() : NULL;

    LdrpWriteStacktrace(pRec, buffer);

    DbgOutput(buffer.data());
        
    LdrpFlsSetValue(PredefinedTlsSlots::TlsIdx_Check, reinterpret_cast<PVOID>(0ul));
}

template <typename T>
inline T LdrpWrapResultReturn(T&& result)
{
    return std::forward<T>(result);
}

inline void LdrpWrapNoResultReturn()
{
}

class LoaderTypeIdentity final
{
    std::string_view name;
public:
};

class ILoaderDataWatch
{
protected:
    ILoaderDataWatch() = default;
public:
    virtual ~ILoaderDataWatch() = default;
    virtual void update() = 0;
    virtual void free() = 0;
    virtual bool freed() const = 0;
    virtual bool equal_type(const std::type_info&) const = 0;
};

ILoaderDataWatch* LdrpGetDataWatch(ULONG_PTR pointer);

template <typename T>
class LoaderDataWatch final : public ILoaderDataWatch
{
    T* pointer;
    std::aligned_storage_t<sizeof(T), alignof(T)> copied_value;
    bool freed_ = false;

    void copy() noexcept
    {
        *std::launder(reinterpret_cast<T*>(&copied_value)) = *pointer;
    }

    LoaderDataWatch(T* pointer);

    template <typename TP>
    friend LoaderDataWatch<std::remove_cvref_t<TP>>* LdrpGetOrCreateDataWatch(TP* pointer);

public:

    ~LoaderDataWatch() = default;
    LoaderDataWatch(const LoaderDataWatch& Other) = delete;
    LoaderDataWatch(LoaderDataWatch&& Other) noexcept = default;
    LoaderDataWatch& operator=(const LoaderDataWatch& Other) = delete;
    LoaderDataWatch& operator=(LoaderDataWatch&& Other) noexcept = default;

    void update() noexcept
    {
        copy();
        verify();
    }

    void free() override;

    bool equal_type(const std::type_info& type) const override
    {
        return type == typeid(T);
    }

    LoaderDataWatch& verify() noexcept
    {
        using Type = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<Type, LDRP_LOAD_CONTEXT>)
        {
            LDRP_ASSERT_LOAD_CONTEXT(pointer);
        }
        else if constexpr (std::is_same_v<Type, LDRP_CSLIST_ENTRY>)
        {
            LDRP_ASSERT_CSLIST_ENTRY(pointer);
        }
        else if constexpr (std::is_same_v<Type, LDR_DDAG_NODE>)
        {
            LDRP_ASSERT_DDAG_NODE(pointer);
        }
        else if constexpr (std::is_same_v<Type, LDR_DATA_TABLE_ENTRY>)
        {
            LDRP_ASSERT_MODULE_ENTRY(pointer);
        }
        return *this;
    }

    [[nodiscard]] bool freed() const override
    {
        return freed_;
    }
};

template <typename T>
LoaderDataWatch<std::remove_cvref_t<T>>* LdrpGetOrCreateDataWatch(T* pointer);

class ILoaderFunctionLocal
{
protected:
    ILoaderFunctionLocal() = default;
public:
    virtual ~ILoaderFunctionLocal() = default;
    virtual void update();
    virtual void free();
};

template <typename T>
class LoaderFunctionLocal final : public ILoaderFunctionLocal
{
    std::string_view name;
    std::string_view type_name;
    LoaderDataWatch<std::remove_cvref_t<T>>* watch;
public:
    LoaderFunctionLocal(std::string_view name, std::string_view type_name, T* value) : name{name},
        type_name{type_name},
        watch{LdrpGetOrCreateDataWatch<T>(value)}
    {
    }

    void update() override
    {
        watch->update();
    }

    void free() override
    {
        watch->free();
    }

    ~LoaderFunctionLocal() = default;
    LoaderFunctionLocal(const LoaderFunctionLocal& Other) = delete;
    LoaderFunctionLocal(LoaderFunctionLocal&& Other) noexcept = default;
    LoaderFunctionLocal& operator=(const LoaderFunctionLocal& Other) = delete;
    LoaderFunctionLocal& operator=(LoaderFunctionLocal&& Other) noexcept = default;
};

template <typename T>
class LoaderFunctionArgument final
{
    LoaderFunctionLocal<T> local;
public:
    LoaderFunctionArgument(std::string_view name, std::string_view type_name, T* value) : local{name, type_name, value}
    {
    }

    void update()
    {
        local.update();
    }

    void free()
    {
        local.free();
    }

    ~LoaderFunctionArgument() = default;
    LoaderFunctionArgument(const LoaderFunctionArgument& Other) = delete;
    LoaderFunctionArgument(LoaderFunctionArgument&& Other) noexcept = default;
    LoaderFunctionArgument& operator=(const LoaderFunctionArgument& Other) = delete;
    LoaderFunctionArgument& operator=(LoaderFunctionArgument&& Other) noexcept = default;
};

#define LDR_LOCAL(x) #x, type_name<decltype(x)>(), &x
#define LDR_ARG(x) LoaderFunctionArgument<decltype(x)>{ LDR_LOCAL(x) }

#define LDR_LOCATION_FMT_PREFIX "({}:{}) "

class LoaderLogContext final
{
    template <typename TBuffer>
    void write_stacktrace(TBuffer& buffer)
    {
        ContractStackRecord* pRec = CheckClrDebugState() ? CheckClrDebugState()->GetContractStackTrace() : nullptr;

        LdrpWriteStacktrace(pRec, buffer);
    }
public:
    explicit LoaderLogContext() = default;
    ~LoaderLogContext() = default;
    LoaderLogContext(const LoaderLogContext& Other) = delete;
    LoaderLogContext(LoaderLogContext&& Other) noexcept = delete;
    LoaderLogContext& operator=(const LoaderLogContext& Other) = delete;
    LoaderLogContext& operator=(LoaderLogContext&& Other) noexcept = delete;

    template <typename S, typename... Args, typename Char = fmt::char_t<S>>
    LoaderLogContext& log(LoaderLogLevel level, const S& format_str, Args&&... args)
    {
        fmt::basic_memory_buffer<char, 512 * 20 + 2048 + 1024> buffer;

        if (level >= LoaderLogLevel::Warning)
        {
            const auto& vargs = fmt::make_args_checked<Args...>(format_str, args...);
            fmt::vformat_to(buffer, format_str, vargs);
            fmt::format_to(buffer, "\n");
        }

        if (level >= LoaderLogLevel::Error)
        {
            write_stacktrace(buffer);
        }

        if (buffer.size())
        {
            DbgOutput(buffer.data());
        }

        return *this;
    }

    LoaderLogContext& log_stacktrace()
    {
        fmt::basic_memory_buffer<char, 512 * 20 + 2048 + 1024> buffer;

        write_stacktrace(buffer);

        if (buffer.size())
        {
            DbgOutput(buffer.data());
        }

        return *this;
    }
};

class LoaderStackEntry final
{
    ClrDebugState* m_pClrDebugState;
    ClrDebugState m_IncomingClrDebugState;
    ContractStackRecord m_contractStackRecord;
public:
    LoaderStackEntry(std::string_view szFunction, std::string_view szFile, int lineNum)
    {
        m_pClrDebugState = GetClrDebugState();

        // Save the incoming contents for restoration in the destructor
        m_IncomingClrDebugState = *m_pClrDebugState;

        // Setup the new stack record.
        m_contractStackRecord.m_szFunction = szFunction;
        m_contractStackRecord.m_szFile = szFile;
        m_contractStackRecord.m_lineNum = lineNum;
        m_contractStackRecord.m_construct = "LoaderStackEntry";

        // Link the new ContractStackRecord into the chain for this thread.
        m_pClrDebugState->LinkContractStackTrace(&m_contractStackRecord);
    }

    ~LoaderStackEntry()
    {
        if (m_pClrDebugState)
        {
            // Backout all changes to debug state.
            *m_pClrDebugState = m_IncomingClrDebugState;
        }
    }

    LoaderStackEntry(const LoaderStackEntry& Other) = delete;
    LoaderStackEntry(LoaderStackEntry&& Other) noexcept = delete;
    LoaderStackEntry& operator=(const LoaderStackEntry& Other) = delete;
    LoaderStackEntry& operator=(LoaderStackEntry&& Other) noexcept = delete;
};

template <typename... Args>
class LoaderFunction final
{
    std::string_view name;
    std::tuple<Args...> arguments;
    LoaderLogContext log_context;
    std::vector<ILoaderFunctionLocal*> local_watches;
    LoaderStackEntry* stack_entry;

public:
    explicit LoaderFunction(std::string_view name, Args&&... args) : name{name},
                                                                     arguments{std::forward<Args&&>(args)...},
                                                                     stack_entry{new LoaderStackEntry{name, __FILE__, __LINE__}}
    {
        log(LoaderLogLevel::Trace, "{}: ENTER", name);
    }

    ~LoaderFunction()
    {
        log(LoaderLogLevel::Trace, "{}: EXIT", name);

        step();
        std::apply([&](auto& ...x)
        {
            (..., x.free());
        }, arguments);
        for (auto*& LocalWatch : local_watches)
        {
            LocalWatch->free();
            delete LocalWatch;
            LocalWatch = nullptr;
        }
    }

    LoaderFunction(const LoaderFunction& Other) = delete;
    LoaderFunction(LoaderFunction&& Other) noexcept = delete;
    LoaderFunction& operator=(const LoaderFunction& Other) = delete;
    LoaderFunction& operator=(LoaderFunction&& Other) noexcept = delete;

    void step()
    {
        std::apply([&](auto& ...x)
        {
            (..., x.update());
        }, arguments);
        for (auto* LocalWatch : local_watches)
        {
            LocalWatch->update();
        }
    }

    template <typename S, typename... Args, typename Char = fmt::char_t<S>>
    LoaderFunction& log(LoaderLogLevel level, const S& format_str, Args&&... args)
    {
        log_context.log(level, format_str, std::forward<Args>(args)...);
        step();
        return *this;
    }

    template <typename T>
    void watch_local(std::string_view name, std::string_view type_name, T* value, bool do_not_step = false)
    {
        if (!do_not_step)
            step();
        local_watches.push_back(new LoaderFunctionLocal<T>(name, type_name, value));
    }

    template <typename TBuffer>
    void log_stacktrace(TBuffer& buffer)
    {
        log_context.log_stacktrace();
    }

    [[nodiscard]] const LoaderLogContext& LogContext() const
    {
        return log_context;
    }

    [[nodiscard]] LoaderLogContext& LogContext()
    {
        return log_context;
    }
};

// todo: use decltype(std::function{foo})::result_type as return type
// todo: merge with contracts
// todo: checks, asserts
// todo: ring buffer logging
// todo: hard errors


#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define FOR_EACH_C_1(what, x) what(x)
#define FOR_EACH_C_2(what, x, ...) what(x), FOR_EACH_C_1(what,  __VA_ARGS__)
#define FOR_EACH_C_3(what, x, ...) what(x), FOR_EACH_C_2(what, __VA_ARGS__)
#define FOR_EACH_C_4(what, x, ...) what(x), FOR_EACH_C_3(what,  __VA_ARGS__)
#define FOR_EACH_C_5(what, x, ...) what(x), FOR_EACH_C_4(what,  __VA_ARGS__)
#define FOR_EACH_C_6(what, x, ...) what(x), FOR_EACH_C_5(what,  __VA_ARGS__)
#define FOR_EACH_C_7(what, x, ...) what(x), FOR_EACH_C_6(what,  __VA_ARGS__)
#define FOR_EACH_C_8(what, x, ...) what(x), FOR_EACH_C_7(what,  __VA_ARGS__)
#define FOR_EACH_S_1(what, x) what(x)
#define FOR_EACH_S_2(what, x, ...) what(x); FOR_EACH_S_1(what,  __VA_ARGS__);
#define FOR_EACH_S_3(what, x, ...) what(x); FOR_EACH_S_2(what, __VA_ARGS__);
#define FOR_EACH_S_4(what, x, ...) what(x); FOR_EACH_S_3(what,  __VA_ARGS__);
#define FOR_EACH_S_5(what, x, ...) what(x); FOR_EACH_S_4(what,  __VA_ARGS__);
#define FOR_EACH_S_6(what, x, ...) what(x); FOR_EACH_S_5(what,  __VA_ARGS__);
#define FOR_EACH_S_7(what, x, ...) what(x); FOR_EACH_S_6(what,  __VA_ARGS__);
#define FOR_EACH_S_8(what, x, ...) what(x); FOR_EACH_S_7(what,  __VA_ARGS__);

#define FOR_EACH_NARG(...) FOR_EACH_NARG_(__VA_ARGS__, FOR_EACH_RSEQ_N())
#define FOR_EACH_NARG_(...) FOR_EACH_ARG_N(__VA_ARGS__) 
#define FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N 
#define FOR_EACH_RSEQ_N() 8, 7, 6, 5, 4, 3, 2, 1, 0

#define FOR_EACH_C_(N, what, ...) CONCATENATE(FOR_EACH_C_, N)(what, __VA_ARGS__)
#define FOR_EACH_S_(N, what, ...) CONCATENATE(FOR_EACH_S_, N)(what, __VA_ARGS__)

#define LDR_FUNC_IMPL(...) LoaderFunction ldr_function{__FUNCTION__ __VA_OPT__(,) __VA_ARGS__}; LoaderLogContext& ldr_log = ldr_function.LogContext(); (void)ldr_log;
#define LDR_FUNC_VOID_MANUAL(...) __try { ([&]() -> void { LDR_FUNC_IMPL(__VA_ARGS__)
#define LDR_FUNC_MANUAL(returntype, ...) __try { return LdrpWrapResultReturn([&]() -> returntype { LDR_FUNC_IMPL(__VA_ARGS__)
#define LDR_FUNC_VOID(...) LDR_FUNC_VOID_MANUAL(FOR_EACH_C_(FOR_EACH_NARG(__VA_ARGS__), LDR_ARG, __VA_ARGS__))
#define LDR_FUNC(returntype, ...) LDR_FUNC_MANUAL(returntype, FOR_EACH_C_(FOR_EACH_NARG(__VA_ARGS__), LDR_ARG, __VA_ARGS__))
#define LDR_LOG_IMPL(context, level, fmtstr, ...) context.log(LoaderLogLevel::level, LDR_LOCATION_FMT_PREFIX fmtstr, __RELFILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define LDR_TRACE(fmtstr, ...) LDR_LOG_IMPL(ldr_function, Trace, fmtstr, __VA_ARGS__)
#define LDR_DEBUG(fmtstr, ...) LDR_LOG_IMPL(ldr_function, Debug, fmtstr, __VA_ARGS__)
#define LDR_INFO(fmtstr, ...) LDR_LOG_IMPL(ldr_function, Information, fmtstr, __VA_ARGS__)
#define LDR_WARNING(fmtstr, ...) LDR_LOG_IMPL(ldr_function, Warning, fmtstr, __VA_ARGS__)
#define LDR_ERROR(fmtstr, ...) LDR_LOG_IMPL(ldr_function, Error, fmtstr, __VA_ARGS__)
#define LDR_FATAL(fmtstr, ...) LDR_LOG_IMPL(ldr_function, Fatal, fmtstr, __VA_ARGS__)

#define LDR_STEP() ldr_function.step()
#define LDR_LOG_STACKTRACE() ldr_function.log_stacktrace()
#define LDR_WATCH_LOCAL(x) ldr_function.watch_local<decltype(x)>(LDR_LOCAL(x))
#define LDR_WATCH_LOCALS_IMPL(x) ldr_function.watch_local<decltype(x)>(LDR_LOCAL(x), true)
#define LDR_WATCH_LOCALS(...) FOR_EACH_S_(FOR_EACH_NARG(__VA_ARGS__), LDR_WATCH_LOCALS_IMPL, __VA_ARGS__) LDR_STEP()

#define LDR_FUNC_END_IMPL_EXCEPT(statement) __except(LdrpDebugExceptionFilter(GetExceptionCode(), GetExceptionInformation())) { LoaderLogContext ldr_log; LDR_LOG_IMPL(ldr_log, Error, "Exception " LDR_HEX32_FMT, GetExceptionCode()); statement; }
#define LDR_FUNC_END_IMPL(statement) }()); } LDR_FUNC_END_IMPL_EXCEPT(statement)
#define LDR_FUNC_END_VOID }(), LdrpWrapNoResultReturn()); } LDR_FUNC_END_IMPL_EXCEPT()
#define LDR_FUNC_END_RETURN LDR_FUNC_END_IMPL(return GetExceptionCode())
#define LDR_FUNC_END_RETURN_NULL LDR_FUNC_END_IMPL(return nullptr)

/* The ASSERTs must be cast to void to avoid warnings about unused results. */
#define CHECK_ZERO assert_value
#define CHECK_NONZERO !assert_value
#define CHECK_SUCCESS !NT_SUCCESS(assert_value)
#define CHECK_EQ(expected) ((assert_value) != (expected))
#define ASSERTEX(exp, checker, handle, message, ...) if (auto&& assert_value = exp; checker) { LdrpAssert(#exp, __FUNCTION__, __RELFILE__, __LINE__, message __VA_OPT__(,) __VA_ARGS__); handle }
#define ASSERT_NONZERO(exp, handle, message, ...) ASSERTEX(exp, CHECK_NONZERO, handle, message, __VA_ARGS__)
#define ASSERTMSG(exp, message, ...) ASSERTEX(exp, CHECK_NONZERO, , message, __VA_ARGS__)
//#define ASSERT(exp) ASSERTMSG(exp, {})

#define ASSERT_SUCCESS(exp, handle) ASSERTEX(exp, CHECK_SUCCESS, handle;, "NT:" LDR_HEX32_FMT, assert_value)
#define ASSERT_SUCCESS_OR_IGNORE(exp) ASSERT_SUCCESS(exp,)
#define ASSERT_SUCCESS_OR_RETURN(exp) ASSERT_SUCCESS(exp, return assert_value)
#define ASSERT_EQ(exp, expected) ASSERTEX(exp, CHECK_EQ(expected), , "{} != {}", assert_value, expected)
#define ASSERT_TRUE(exp) ASSERTEX(exp, CHECK_NONZERO, , "!{}", assert_value)
#define ASSERT_FALSE(exp) ASSERTEX(exp, CHECK_ZERO, , "{}", assert_value)

template <typename T>
LoaderDataWatch<std::remove_cvref_t<T>>* LdrpGetOrCreateDataWatch(T* pointer)
{
    using V = std::remove_cvref_t<T>;
    auto* instance = LdrpGetDataWatch(reinterpret_cast<ULONG_PTR>(static_cast<const void*>(pointer)));
    LoaderDataWatch<V>* implInstance;

    if (instance)
    {
        ASSERT_TRUE(instance->equal_type(typeid(V)));
        implInstance = dynamic_cast<LoaderDataWatch<V>*>(instance);
        return implInstance;
    }

    implInstance = new LoaderDataWatch<V>{const_cast<V*>(pointer)};
    return implInstance;
}

#ifndef NDEBUG
#define LDRP_ASSERT_CSLIST_ENTRY(Entry) ASSERTMSG((Entry) && (Entry)->Debug == LDRP_CSLIST_ENTRY_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)Entry, (Entry) ? (Entry)->Debug : 0)
#define LDRP_ASSERT_CSLIST_ENTRY_OPTIONAL(Entry) ASSERTMSG(!(Entry) || (Entry)->Debug == LDRP_CSLIST_ENTRY_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)Entry, (Entry) ? (Entry)->Debug : 0)
#define LDRP_MARK_CSLIST_ENTRY(Entry) do { ASSERTMSG(Entry, LDR_HEXPTR_FMT, (ULONG_PTR)Entry); (Entry)->Debug = LDRP_CSLIST_ENTRY_MARKER; } while (FALSE)
#define LDRP_ASSERT_DDAG_NODE(Node) ASSERTMSG((Node) && (Node)->Debug == LDRP_DDAG_NODE_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)Node, (Node) ? (Node)->Debug : 0)
#define LDRP_ASSERT_DDAG_NODE_OPTIONAL(Node) ASSERTMSG(!(Node) || (Node)->Debug == LDRP_DDAG_NODE_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)Node, (Node) ? (Node)->Debug : 0)
#define LDRP_MARK_DDAG_NODE(Node) do { ASSERTMSG(Node, LDR_HEXPTR_FMT, (ULONG_PTR)Node); (Node)->Debug = LDRP_DDAG_NODE_MARKER; } while (FALSE)
#define LDRP_ASSERT_MODULE_ENTRY(Entry) ASSERTMSG((Entry) && (Entry)->Debug == LDRP_MODULE_ENTRY_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)Entry, (Entry) ? (Entry)->Debug : 0)
#define LDRP_ASSERT_MODULE_ENTRY_OPTIONAL(Entry) ASSERTMSG(!(Entry) || (Entry)->Debug == LDRP_MODULE_ENTRY_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)Entry, (Entry) ? (Entry)->Debug : 0)
#define LDRP_MARK_MODULE_ENTRY(Entry) do { ASSERTMSG(Entry, LDR_HEXPTR_FMT, (ULONG_PTR)Entry); (Entry)->Debug = LDRP_MODULE_ENTRY_MARKER; } while (FALSE)
#define LDRP_ASSERT_LOAD_CONTEXT(LoadContext) ASSERTMSG((LoadContext) && (LoadContext)->Debug == LDRP_LOAD_CONTEXT_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", LoadContext, (LoadContext) ? (LoadContext)->Debug : 0)
#define LDRP_ASSERT_LOAD_CONTEXT_OPTIONAL(LoadContext) ASSERTMSG(!(LoadContext) || (LoadContext)->Debug == LDRP_LOAD_CONTEXT_MARKER, LDR_HEXPTR_FMT "[" LDR_HEX32_FMT "]", (ULONG_PTR)LoadContext, (LoadContext) ? (LoadContext)->Debug : 0)
#define LDRP_MARK_LOAD_CONTEXT(LoadContext) do { ASSERTMSG(LoadContext, LDR_HEXPTR_FMT, (ULONG_PTR)LoadContext); (LoadContext)->Debug = LDRP_LOAD_CONTEXT_MARKER; } while (FALSE)
#define LDRP_ASSERT_NODE_ENTRY(Node, Entry) ASSERTMSG((Node) && (Entry) && ((Entry)->DdagNode == (Node)), LDR_HEXPTR_FMT "," LDR_HEXPTR_FMT "," LDR_HEXPTR_FMT, (ULONG_PTR)Node, (ULONG_PTR)Entry, (ULONG_PTR)((Entry)->DdagNode))
#else
#define LDRP_ASSERT_CSLIST_ENTRY(Entry) ASSERTMSG(Entry, LDR_HEXPTR_FMT, (ULONG_PTR)Entry)
#define LDRP_ASSERT_CSLIST_ENTRY_OPTIONAL(Entry)
#define LDRP_MARK_CSLIST_ENTRY(Entry) ASSERTMSG(Entry, LDR_HEXPTR_FMT, (ULONG_PTR)Entry)
#define LDRP_ASSERT_DDAG_NODE(Node) ASSERTMSG(Node, LDR_HEXPTR_FMT, (ULONG_PTR)Node)
#define LDRP_ASSERT_DDAG_NODE_OPTIONAL(Node)
#define LDRP_MARK_DDAG_NODE(Node) ASSERTMSG(Node, LDR_HEXPTR_FMT, (ULONG_PTR)Node)
#define LDRP_ASSERT_MODULE_ENTRY(Entry) ASSERTMSG(Entry, LDR_HEXPTR_FMT, (ULONG_PTR)Entry)
#define LDRP_ASSERT_MODULE_ENTRY_OPTIONAL(Entry)
#define LDRP_MARK_MODULE_ENTRY(Entry) ASSERTMSG(Entry, LDR_HEXPTR_FMT, (ULONG_PTR)Entry)
#define LDRP_ASSERT_LOAD_CONTEXT(LoadContext) ASSERTMSG(LoadContext, LDR_HEXPTR_FMT, (ULONG_PTR)LoadContext)
#define LDRP_ASSERT_LOAD_CONTEXT_OPTIONAL(LoadContext)
#define LDRP_MARK_LOAD_CONTEXT(LoadContext) ASSERTMSG(LoadContext, LDR_HEXPTR_FMT, (ULONG_PTR)LoadContext)
#define LDRP_ASSERT_NODE_ENTRY(Node, Entry) ASSERTMSG((Node) && (Entry), LDR_HEXPTR_FMT "," LDR_HEXPTR_FMT, (ULONG_PTR)Node, (ULONG_PTR)Entry)
#endif
