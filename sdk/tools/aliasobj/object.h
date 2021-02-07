#ifndef INCLUDE_OBJECT_H
#define INCLUDE_OBJECT_H

#include <sal.h>

#ifdef __GNUC__
#define __int8 char
#define __int16 short
#define __int32 int
#define __int64 long long

# ifdef __cplusplus
#  define __forceinline inline __attribute__((__always_inline__))
# else

#  if defined (__GNUC__) && defined (__GNUC_MINOR__)
#   define __MINGW_GNUC_PREREQ(major, minor) \
     (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#  else
#   define __MINGW_GNUC_PREREQ(major, minor) 0
#  endif

#  if ( __MINGW_GNUC_PREREQ(4, 3)  &&  __STDC_VERSION__ >= 199901L)
#   define __forceinline extern inline __attribute__((__always_inline__,__gnu_inline__))
#  else
#   define __forceinline extern __inline__ __attribute__((__always_inline__))
#  endif

# endif
#endif

typedef unsigned char BYTE, BOOLEAN;
typedef unsigned short WORD;
typedef unsigned long DWORD;

#if defined(_M_AMD64)
#include <intrin.h>
typedef unsigned __int64 DWORD64;
#endif

typedef _Null_terminated_ char* PSTR;
typedef _Null_terminated_ const char* PCSTR;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FORCEINLINE
#if defined(__GNUC__) || (defined(_MSC_VER) && (_MSC_VER >= 1200))
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#endif

#ifdef __cplusplus
#ifndef _STATIC_ASSERT
#define _STATIC_ASSERT(expr) static_assert((expr), #expr)
#endif
#else
#ifndef _STATIC_ASSERT
#ifdef __clang__
#define _STATIC_ASSERT(expr) _Static_assert((expr), #expr)
#else
#define _STATIC_ASSERT(expr) typedef char __static_assert_t[(expr) != 0]
#endif
#endif
#endif

#define IMAGE_FILE_MACHINE_UNKNOWN      0
#define IMAGE_SCN_CNT_CODE              0x00000020  // Section contains code.
#define IMAGE_SCN_MEM_EXECUTE           0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ              0x40000000  // Section is readable.
#define IMAGE_SYM_UNDEFINED             (short)0    // Symbol is undefined or is common.
#define IMAGE_SYM_TYPE_NULL             0x0000      // no type.
#define IMAGE_SYM_CLASS_EXTERNAL        0x0002
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL   0x0069
#define IMAGE_WEAK_EXTERN_SEARCH_ALIAS  3
#define IMAGE_SIZEOF_FILE_HEADER        20
#define IMAGE_SIZEOF_SHORT_NAME         8
#define IMAGE_SIZEOF_SECTION_HEADER     40
#define IMAGE_SIZEOF_SYMBOL             18

#include <pshpack4.h>

typedef struct
{
    WORD Machine;
    WORD NumberOfSections;
    DWORD TimeDateStamp;
    DWORD PointerToSymbolTable;
    DWORD NumberOfSymbols;
    WORD SizeOfOptionalHeader;
    WORD Characteristics;
} IMAGE_FILE_HEADER;

typedef struct
{
    BYTE Name[IMAGE_SIZEOF_SHORT_NAME];

    union
    {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;

    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD NumberOfRelocations;
    WORD NumberOfLinenumbers;
    DWORD Characteristics;
} IMAGE_SECTION_HEADER;

#include <poppack.h>

#include <pshpack2.h>

typedef struct
{
    union
    {
        BYTE ShortName[IMAGE_SIZEOF_SHORT_NAME];

        struct
        {
            DWORD Short; // if 0, use LongName
            DWORD Long; // offset into string table
        } Name;

        DWORD LongName[2]; // PBYTE [2]
    } N;

    DWORD Value;
    short SectionNumber;
    WORD Type;
    BYTE StorageClass;
    BYTE NumberOfAuxSymbols;
} IMAGE_SYMBOL;

typedef struct
{
    DWORD TagIndex; // struct, union, or enum tag index

    union
    {
        struct
        {
            WORD Linenumber; // declaration line number
            WORD Size; // size of struct, union, or enum
        } LnSz;

        DWORD TotalSize;
    } Misc;

    union
    {
        struct
        {
            // if ISFCN, tag, or .bb
            DWORD PointerToLinenumber;
            DWORD PointerToNextFunction;
        } Function;

        struct
        {
            // if ISARY, up to 4 dimen.
            WORD Dimension[4];
        } Array;
    } FcnAry;

    WORD TvIndex; // tv index
} IMAGE_AUX_SYMBOL;

#include <poppack.h>

_STATIC_ASSERT(sizeof(IMAGE_FILE_HEADER) == IMAGE_SIZEOF_FILE_HEADER);
_STATIC_ASSERT(sizeof(IMAGE_SECTION_HEADER) == IMAGE_SIZEOF_SECTION_HEADER);
_STATIC_ASSERT(sizeof(IMAGE_SYMBOL) == IMAGE_SIZEOF_SYMBOL);
_STATIC_ASSERT(sizeof(IMAGE_AUX_SYMBOL) == IMAGE_SIZEOF_SYMBOL);

FORCEINLINE
void
RtlSecureZeroMemory(_Out_writes_bytes_all_(cnt) void* ptr, _In_ size_t cnt)
{
    volatile char* vptr = (volatile char*)ptr;

#if defined(_M_AMD64)

    __stosb((BYTE*)((DWORD64)vptr), 0, cnt);

#else

    while (cnt)
    {
#if !defined(_M_CEE) && (defined(_M_ARM) || defined(_M_ARM64))

        __iso_volatile_store8(vptr, 0);

#else

        *vptr = 0;

#endif

        vptr++;
        cnt--;
    }

#endif // _M_AMD64
}

#endif
