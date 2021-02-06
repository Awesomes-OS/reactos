/*
 * Wine internal Unicode definitions
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_UNICODE_H
#define __WINE_WINE_UNICODE_H

#include <stdarg.h>
#include <stdio.h>
#include <ndk/rtlfuncs.h>

#ifdef __WINE_USE_MSVCRT
#error This file should not be used with msvcrt headers
#endif

#ifndef __ms_va_list
#  define __ms_va_list va_list
#  define __ms_va_start(list,arg) va_start(list,arg)
#  define __ms_va_end(list) va_end(list)
#  ifdef va_copy
#   define __ms_va_copy(dest,src) va_copy(dest,src)
#  else
#   define __ms_va_copy(dest,src) ((dest) = (src))
#  endif
#endif

#ifndef WINAPIV
#  define WINAPIV __cdecl
#endif

#ifndef WINE_UNICODE_INLINE
#define WINE_UNICODE_INLINE static FORCEINLINE
#endif

WINE_UNICODE_INLINE WCHAR tolowerW( WCHAR ch )
{
    return RtlDowncaseUnicodeChar( ch );
}

WINE_UNICODE_INLINE WCHAR toupperW( WCHAR ch )
{
    return RtlUpcaseUnicodeChar( ch );
}

#define memicmpW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strlenW(s) wcslen((s))
#define strcpyW(d,s) wcscpy((d),(s))
#define strcatW(d,s) wcscat((d),(s))
#define strcspnW(d,s) wcscspn((d),(s))
#define strstrW(d,s) wcsstr((d),(s))
#define strtolW(s,e,b) wcstol((s),(e),(b))
#define strchrW(s,c) wcschr((s),(c))
#define strrchrW(s,c) wcsrchr((s),(c))
#define strncmpW(s1,s2,n) wcsncmp((s1),(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((s1),(s2),(n))
#define strcmpW(s1,s2) wcscmp((s1),(s2))
#define strcmpiW(s1,s2) _wcsicmp((s1),(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strtoulW(s1,s2,b) wcstoul((s1),(s2),(b))
#define strspnW(str, accept) wcsspn((str),(accept))
#define strpbrkW(str, accept) wcspbrk((str),(accept))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define iscntrlW(n) iswcntrl((n))
#define atoiW(s) _wtoi((s))
#define atolW(s) _wtol((s))
#define strlwrW(s) _wcslwr((s))
#define struprW(s) _wcsupr((s))
#if 0
#define vsprintfW vswprintf
#define vsnprintfW _vsnwprintf
#define isprintW iswprint
#endif

static __inline unsigned short get_char_typeW( WCHAR ch )
{
    extern const unsigned short wine_wctype_table[];
    return wine_wctype_table[wine_wctype_table[ch >> 8] + (ch & 0xff)];
}

static __inline WCHAR *memchrW( const WCHAR *ptr, WCHAR ch, size_t n )
{
    const WCHAR *end;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) return (WCHAR *)ptr;
    return NULL;
}

static __inline WCHAR *memrchrW( const WCHAR *ptr, WCHAR ch, size_t n )
{
    const WCHAR *end, *ret = NULL;
    for (end = ptr + n; ptr < end; ptr++) if (*ptr == ch) ret = ptr;
    return (WCHAR *)ret;
}

NTSYSAPI int __cdecl _vsnwprintf(WCHAR*,size_t,const WCHAR*,__ms_va_list);

static inline int WINAPIV snprintfW( WCHAR *str, size_t len, const WCHAR *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = _vsnwprintf(str, len, format, valist);
    __ms_va_end(valist);
    return retval;
}

static inline int WINAPIV sprintfW( WCHAR *str, const WCHAR *format, ...)
{
    int retval;
    __ms_va_list valist;
    __ms_va_start(valist, format);
    retval = _vsnwprintf(str, MAXLONG, format, valist);
    __ms_va_end(valist);
    return retval;
}

#undef WINE_UNICODE_INLINE


#endif
