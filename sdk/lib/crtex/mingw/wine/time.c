/*
 * msvcrt.dll date/time functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2004 Hans Leidekker
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include "msvcrt.h"
#include "mtdll.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/debug.h"


void _invalid_parameter(
   const wchar_t * expression,
   const wchar_t * function, 
   const wchar_t * file, 
   unsigned int line,
   uintptr_t pReserved);

#define MSVCRT_INVALID_PMT(x,err)   (*_errno() = (err), _invalid_parameter(NULL, NULL, NULL, 0, 0))
extern MSVCRT_threadlocinfo _LIBCNT_locinfo;
#define get_locinfo() (&_LIBCNT_locinfo)

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

#if _MSVCR_VER>=140
static const int MAX_SECONDS = 60;
#else
static const int MAX_SECONDS = 59;
#endif

static inline BOOL IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0);
}

/*********************************************************************
 *		_daylight (MSVCRT.@)
 */
int MSVCRT___daylight = 1;

/*********************************************************************
 *		_timezone (MSVCRT.@)
 */
MSVCRT_long MSVCRT___timezone = 28800;

/*********************************************************************
 *		_dstbias (MSVCRT.@)
 */
int MSVCRT__dstbias = -3600;

#if _MSVCR_VER <= 90
#define STRFTIME_CHAR char
#define STRFTIME_TD(td, name) td->str.names.name
#else
#define STRFTIME_CHAR MSVCRT_wchar_t
#define STRFTIME_TD(td, name) td->wstr.names.name
#endif

#define strftime_str(a,b,c,d) strftime_nstr(a,b,c,d,MSVCRT_SIZE_MAX)
static inline BOOL strftime_nstr(STRFTIME_CHAR *str, MSVCRT_size_t *pos,
        MSVCRT_size_t max, const STRFTIME_CHAR *src, MSVCRT_size_t len)
{
    while(*src && len)
    {
        if(*pos >= max) {
            *str = 0;
            *_errno() = MSVCRT_ERANGE;
            return FALSE;
        }

        str[*pos] = *src;
        src++;
        *pos += 1;
        len--;
    }
    return TRUE;
}

static inline BOOL strftime_int(STRFTIME_CHAR *str, MSVCRT_size_t *pos, MSVCRT_size_t max,
        int src, int prec, int l, int h)
{
#if _MSVCR_VER > 90
    static const WCHAR fmt[] = {'%','0','*','d',0};
#endif
    MSVCRT_size_t len;

    if(!MSVCRT_CHECK_PMT(src>=l && src<=h)) {
        *str = 0;
        return FALSE;
    }

#if _MSVCR_VER <= 90
    len = _snprintf(str+*pos, max-*pos, "%0*d", prec, src);
#else
    len = _snwprintf(str+*pos, max-*pos, fmt, prec, src);
#endif
    if(len == -1) {
        *str = 0;
        *_errno() = MSVCRT_ERANGE;
        return FALSE;
    }

    *pos += len;
    return TRUE;
}

static inline BOOL strftime_format(STRFTIME_CHAR *str, MSVCRT_size_t *pos, MSVCRT_size_t max,
        const struct MSVCRT_tm *mstm, MSVCRT___lc_time_data *time_data, const STRFTIME_CHAR *format)
{
    MSVCRT_size_t count;
    BOOL ret = TRUE;

    while(*format && ret)
    {
        count = 1;
        while(format[0] == format[count]) count++;

        switch(*format) {
        case '\'':
            if(count % 2 == 0) break;

            format += count;
            count = 0;
            while(format[count] && format[count] != '\'') count++;

            ret = strftime_nstr(str, pos, max, format, count);
            if(!ret) return FALSE;
            if(format[count] == '\'') count++;
            break;
        case 'd':
            if(count > 2)
            {
                if(!MSVCRT_CHECK_PMT(mstm->tm_wday>=0 && mstm->tm_wday<=6))
                {
                    *str = 0;
                    return FALSE;
                }
            }
            switch(count) {
            case 1:
            case 2:
                ret = strftime_int(str, pos, max, mstm->tm_mday, count==1 ? 0 : 2, 1, 31);
                break;
            case 3:
                ret = strftime_str(str, pos, max, STRFTIME_TD(time_data, short_wday)[mstm->tm_wday]);
                break;
            default:
                ret = strftime_nstr(str, pos, max, format, count-4);
                if(ret)
                    ret = strftime_str(str, pos, max, STRFTIME_TD(time_data, wday)[mstm->tm_wday]);
                break;
            }
            break;
        case 'M':
            if(count > 2)
            {
                if(!MSVCRT_CHECK_PMT(mstm->tm_mon>=0 && mstm->tm_mon<=11))
                {
                    *str = 0;
                    return FALSE;
                }
            }
            switch(count) {
            case 1:
            case 2:
                ret = strftime_int(str, pos, max, mstm->tm_mon+1, count==1 ? 0 : 2, 1, 12);
                break;
            case 3:
                ret = strftime_str(str, pos, max, STRFTIME_TD(time_data, short_mon)[mstm->tm_mon]);
                break;
            default:
                ret = strftime_nstr(str, pos, max, format, count-4);
                if(ret)
                    ret = strftime_str(str, pos, max, STRFTIME_TD(time_data, mon)[mstm->tm_mon]);
                break;
            }
            break;
        case 'y':
            if(count > 1)
            {
#if _MSVCR_VER>=140
                if(!MSVCRT_CHECK_PMT(mstm->tm_year >= -1900 && mstm->tm_year <= 8099))
#else
                if(!MSVCRT_CHECK_PMT(mstm->tm_year >= 0))
#endif
                {
                    *str = 0;
                    return FALSE;
                }
            }

            switch(count) {
            case 1:
                ret = strftime_nstr(str, pos, max, format, 1);
                break;
            case 2:
            case 3:
                ret = strftime_nstr(str, pos, max, format, count-2);
                if(ret)
                    ret = strftime_int(str, pos, max, (mstm->tm_year+1900)%100, 2, 0, 99);
                break;
            default:
                ret = strftime_nstr(str, pos, max, format, count-4);
                if(ret)
                    ret = strftime_int(str, pos, max, mstm->tm_year+1900, 4, 0, 9999);
                break;
            }
            break;
        case 'h':
            if(!MSVCRT_CHECK_PMT(mstm->tm_hour>=0 && mstm->tm_hour<=23))
            {
                *str = 0;
                return FALSE;
            }
            if(count > 2)
                ret = strftime_nstr(str, pos, max, format, count-2);
            if(ret)
                ret = strftime_int(str, pos, max, (mstm->tm_hour + 11) % 12 + 1,
                        count == 1 ? 0 : 2, 1, 12);
            break;
        case 'H':
            if(count > 2)
                ret = strftime_nstr(str, pos, max, format, count-2);
            if(ret)
                ret = strftime_int(str, pos, max, mstm->tm_hour, count == 1 ? 0 : 2, 0, 23);
            break;
        case 'm':
            if(count > 2)
                ret = strftime_nstr(str, pos, max, format, count-2);
            if(ret)
                ret = strftime_int(str, pos, max, mstm->tm_min, count == 1 ? 0 : 2, 0, 59);
            break;
        case 's':
            if(count > 2)
                ret = strftime_nstr(str, pos, max, format, count-2);
            if(ret)
                ret = strftime_int(str, pos, max, mstm->tm_sec, count == 1 ? 0 : 2, 0, MAX_SECONDS);
            break;
        case 'a':
        case 'A':
        case 't':
            if(!MSVCRT_CHECK_PMT(mstm->tm_hour>=0 && mstm->tm_hour<=23))
            {
                *str = 0;
                return FALSE;
            }
            ret = strftime_nstr(str, pos, max,
                    mstm->tm_hour < 12 ? STRFTIME_TD(time_data, am) : STRFTIME_TD(time_data, pm),
                    (*format == 't' && count == 1) ? 1 : MSVCRT_SIZE_MAX);
            break;
        default:
            ret = strftime_nstr(str, pos, max, format, count);
            break;
        }
        format += count;
    }

    return ret;
}

#if _MSVCR_VER>=140
static inline BOOL strftime_tzdiff(STRFTIME_CHAR *str, MSVCRT_size_t *pos, MSVCRT_size_t max, BOOL is_dst)
{
    MSVCRT_long tz = MSVCRT___timezone + (is_dst ? MSVCRT__dstbias : 0);
    char sign;

    if(tz < 0) {
        sign = '+';
        tz = -tz;
    }else {
        sign = '-';
    }

    if(*pos < max)
        str[(*pos)++] = sign;
    if(!strftime_int(str, pos, max, tz/60/60, 2, 0, 99))
        return FALSE;
    return strftime_int(str, pos, max, tz/60%60, 2, 0, 59);
}
#endif

static MSVCRT_size_t strftime_impl(STRFTIME_CHAR *str, MSVCRT_size_t max,
        const STRFTIME_CHAR *format, const struct MSVCRT_tm *mstm,
        MSVCRT___lc_time_data *time_data, MSVCRT__locale_t loc)
{
    MSVCRT_size_t ret, tmp;
    BOOL alternate;
    int year = mstm ? mstm->tm_year + 1900 : -1;

    if(!str || !format) {
        if(str && max)
            *str = 0;
        *_errno() = MSVCRT_EINVAL;
        return 0;
    }

    if(!time_data)
        time_data = loc ? loc->locinfo->lc_time_curr : get_locinfo()->lc_time_curr;

    for(ret=0; *format && ret<max; format++) {
        if(*format != '%') {
#if 0 // CRTEX
            if(_isleadbyte_l((unsigned char)*format, loc)) {
                str[ret++] = *(format++);
                if(ret == max) continue;
                if(!MSVCRT_CHECK_PMT(str[ret]))
                    goto einval_error;
            }
#endif
            str[ret++] = *format;
            continue;
        }

        format++;
        if(*format == '#') {
            alternate = TRUE;
            format++;
        }else {
            alternate = FALSE;
        }

        if(!MSVCRT_CHECK_PMT(mstm))
            goto einval_error;

        switch(*format) {
        case 'c':
#if _MSVCR_VER>=140
            if(time_data == &cloc_time_data && !alternate)
            {
                static const WCHAR datetime_format[] =
                        { '%','a',' ','%','b',' ','%','e',' ','%','T',' ','%','Y',0 };
                tmp = strftime_impl(str+ret, max-ret, datetime_format, mstm, time_data, loc);
                if(!tmp)
                    return 0;
                ret += tmp;
                break;
            }
#endif
            if(!strftime_format(str, &ret, max, mstm, time_data,
                    alternate ? STRFTIME_TD(time_data, date) : STRFTIME_TD(time_data, short_date)))
                return 0;
            if(ret < max)
                str[ret++] = ' ';
            if(!strftime_format(str, &ret, max, mstm, time_data, STRFTIME_TD(time_data, time)))
                return 0;
            break;
        case 'x':
            if(!strftime_format(str, &ret, max, mstm, time_data,
                    alternate ? STRFTIME_TD(time_data, date) : STRFTIME_TD(time_data, short_date)))
                return 0;
            break;
        case 'X':
            if(!strftime_format(str, &ret, max, mstm, time_data, STRFTIME_TD(time_data, time)))
                return 0;
            break;
        case 'a':
            if(!MSVCRT_CHECK_PMT(mstm->tm_wday>=0 && mstm->tm_wday<=6))
                goto einval_error;
            if(!strftime_str(str, &ret, max, STRFTIME_TD(time_data, short_wday)[mstm->tm_wday]))
                return 0;
            break;
        case 'A':
            if(!MSVCRT_CHECK_PMT(mstm->tm_wday>=0 && mstm->tm_wday<=6))
                goto einval_error;
            if(!strftime_str(str, &ret, max, STRFTIME_TD(time_data, wday)[mstm->tm_wday]))
                return 0;
            break;
        case 'b':
#if _MSVCR_VER>=140
        case 'h':
#endif
            if(!MSVCRT_CHECK_PMT(mstm->tm_mon>=0 && mstm->tm_mon<=11))
                goto einval_error;
            if(!strftime_str(str, &ret, max, STRFTIME_TD(time_data, short_mon)[mstm->tm_mon]))
                return 0;
            break;
        case 'B':
            if(!MSVCRT_CHECK_PMT(mstm->tm_mon>=0 && mstm->tm_mon<=11))
                goto einval_error;
            if(!strftime_str(str, &ret, max, STRFTIME_TD(time_data, mon)[mstm->tm_mon]))
                return 0;
            break;
#if _MSVCR_VER>=140
        case 'C':
            if(!MSVCRT_CHECK_PMT(year>=0 && year<=9999))
                goto einval_error;
            if(!strftime_int(str, &ret, max, year/100, alternate ? 0 : 2, 0, 99))
                return 0;
            break;
#endif
        case 'd':
            if(!strftime_int(str, &ret, max, mstm->tm_mday, alternate ? 0 : 2, 1, 31))
                return 0;
            break;
#if _MSVCR_VER>=140
        case 'D':
            if(!MSVCRT_CHECK_PMT(year>=0 && year<=9999))
                goto einval_error;
            if(!strftime_int(str, &ret, max, mstm->tm_mon+1, alternate ? 0 : 2, 1, 12))
                return 0;
            if(ret < max)
                str[ret++] = '/';
            if(!strftime_int(str, &ret, max, mstm->tm_mday, alternate ? 0 : 2, 1, 31))
                return 0;
            if(ret < max)
                str[ret++] = '/';
            if(!strftime_int(str, &ret, max, year%100, alternate ? 0 : 2, 0, 99))
                return 0;
            break;
        case 'e':
            if(!strftime_int(str, &ret, max, mstm->tm_mday, alternate ? 0 : 2, 1, 31))
                return 0;
            if(!alternate && str[ret-2] == '0')
                str[ret-2] = ' ';
            break;
        case 'F':
            if(!strftime_int(str, &ret, max, year, alternate ? 0 : 4, 0, 9999))
                return 0;
            if(ret < max)
                str[ret++] = '-';
            if(!strftime_int(str, &ret, max, mstm->tm_mon+1, alternate ? 0 : 2, 1, 12))
                return 0;
            if(ret < max)
                str[ret++] = '-';
            if(!strftime_int(str, &ret, max, mstm->tm_mday, alternate ? 0 : 2, 1, 31))
                return 0;
            break;
        case 'g':
        case 'G':
            if(!MSVCRT_CHECK_PMT(year>=0 && year<=9999))
                goto einval_error;
            /* fall through */
        case 'V':
        {
            int iso_year = year;
            int iso_days = mstm->tm_yday - (mstm->tm_wday ? mstm->tm_wday : 7) + 4;
            if (iso_days < 0)
                iso_days += 365 + IsLeapYear(--iso_year);
            else if(iso_days >= 365 + IsLeapYear(iso_year))
                iso_days -= 365 + IsLeapYear(iso_year++);

            if(*format == 'G') {
                if(!strftime_int(str, &ret, max, iso_year, 4, 0, 9999))
                    return 0;
            } else if(*format == 'g') {
                if(!strftime_int(str, &ret, max, iso_year%100, 2, 0, 99))
                    return 0;
            } else {
                if(!strftime_int(str, &ret, max, iso_days/7 + 1, alternate ? 0 : 2, 0, 53))
                    return 0;
            }
            break;
        }
#endif
        case 'H':
            if(!strftime_int(str, &ret, max, mstm->tm_hour, alternate ? 0 : 2, 0, 23))
                return 0;
            break;
        case 'I':
            if(!MSVCRT_CHECK_PMT(mstm->tm_hour>=0 && mstm->tm_hour<=23))
                goto einval_error;
            if(!strftime_int(str, &ret, max, (mstm->tm_hour + 11) % 12 + 1,
                        alternate ? 0 : 2, 1, 12))
                return 0;
            break;
        case 'j':
            if(!strftime_int(str, &ret, max, mstm->tm_yday+1, alternate ? 0 : 3, 1, 366))
                return 0;
            break;
        case 'm':
            if(!strftime_int(str, &ret, max, mstm->tm_mon+1, alternate ? 0 : 2, 1, 12))
                return 0;
            break;
        case 'M':
            if(!strftime_int(str, &ret, max, mstm->tm_min, alternate ? 0 : 2, 0, 59))
                return 0;
            break;
#if _MSVCR_VER>=140
        case 'n':
            str[ret++] = '\n';
            break;
#endif
        case 'p':
            if(!MSVCRT_CHECK_PMT(mstm->tm_hour>=0 && mstm->tm_hour<=23))
                goto einval_error;
            if(!strftime_str(str, &ret, max, mstm->tm_hour<12 ?
                        STRFTIME_TD(time_data, am) : STRFTIME_TD(time_data, pm)))
                return 0;
            break;
#if _MSVCR_VER>=140
        case 'r':
            if(time_data == &cloc_time_data)
            {
                if(!MSVCRT_CHECK_PMT(mstm->tm_hour>=0 && mstm->tm_hour<=23))
                    goto einval_error;
                if(!strftime_int(str, &ret, max, (mstm->tm_hour + 11) % 12 + 1,
                            alternate ? 0 : 2, 1, 12))
                    return 0;
                if(ret < max)
                    str[ret++] = ':';
                if(!strftime_int(str, &ret, max, mstm->tm_min, alternate ? 0 : 2, 0, 59))
                    return 0;
                if(ret < max)
                    str[ret++] = ':';
                if(!strftime_int(str, &ret, max, mstm->tm_sec, alternate ? 0 : 2, 0, MAX_SECONDS))
                    return 0;
                if(ret < max)
                    str[ret++] = ' ';
                if(!strftime_str(str, &ret, max, mstm->tm_hour<12 ?
                            STRFTIME_TD(time_data, am) : STRFTIME_TD(time_data, pm)))
                    return 0;
            }
            else
            {
                if(!strftime_format(str, &ret, max, mstm, time_data, STRFTIME_TD(time_data, time)))
                    return 0;
            }
            break;
        case 'R':
            if(!strftime_int(str, &ret, max, mstm->tm_hour, alternate ? 0 : 2, 0, 23))
                return 0;
            if(ret < max)
                str[ret++] = ':';
            if(!strftime_int(str, &ret, max, mstm->tm_min, alternate ? 0 : 2, 0, 59))
                return 0;
            break;
#endif
        case 'S':
            if(!strftime_int(str, &ret, max, mstm->tm_sec, alternate ? 0 : 2, 0, MAX_SECONDS))
                return 0;
            break;
#if _MSVCR_VER>=140
        case 't':
            str[ret++] = '\t';
            break;
        case 'T':
            if(!strftime_int(str, &ret, max, mstm->tm_hour, alternate ? 0 : 2, 0, 23))
                return 0;
            if(ret < max)
                str[ret++] = ':';
            if(!strftime_int(str, &ret, max, mstm->tm_min, alternate ? 0 : 2, 0, 59))
                return 0;
            if(ret < max)
                str[ret++] = ':';
            if(!strftime_int(str, &ret, max, mstm->tm_sec, alternate ? 0 : 2, 0, MAX_SECONDS))
                return 0;
            break;
        case 'u':
            if(!MSVCRT_CHECK_PMT(mstm->tm_wday>=0 && mstm->tm_wday<=6))
                goto einval_error;
            tmp = mstm->tm_wday ? mstm->tm_wday : 7;
            if(!strftime_int(str, &ret, max, tmp, 0, 1, 7))
                return 0;
            break;
#endif
        case 'w':
            if(!strftime_int(str, &ret, max, mstm->tm_wday, 0, 0, 6))
                return 0;
            break;
        case 'y':
#if _MSVCR_VER>=140
            if(!MSVCRT_CHECK_PMT(year>=0 && year<=9999))
#else
            if(!MSVCRT_CHECK_PMT(year>=1900))
#endif
                goto einval_error;
            if(!strftime_int(str, &ret, max, year%100, alternate ? 0 : 2, 0, 99))
                return 0;
            break;
        case 'Y':
            if(!strftime_int(str, &ret, max, year, alternate ? 0 : 4, 0, 9999))
                return 0;
            break;
#if 0 // CRTEX
        case 'z':
#if _MSVCR_VER>=140
            MSVCRT__tzset();
            if(!strftime_tzdiff(str, &ret, max, mstm->tm_isdst))
                return 0;
            break;
#endif
        case 'Z':
            MSVCRT__tzset();
#if _MSVCR_VER <= 90
            if(MSVCRT__get_tzname(&tmp, str+ret, max-ret, mstm->tm_isdst ? 1 : 0))
                return 0;
#else
                if(MSVCRT__mbstowcs_s_l(&tmp, str+ret, max-ret,
                            mstm->tm_isdst ? tzname_dst : tzname_std,
                            MSVCRT__TRUNCATE, loc) == MSVCRT_STRUNCATE)
                    ret = max;
#endif
            ret += tmp-1;
            break;
#endif
        case 'U':
        case 'W':
            if(!MSVCRT_CHECK_PMT(mstm->tm_wday>=0 && mstm->tm_wday<=6))
                goto einval_error;
            if(!MSVCRT_CHECK_PMT(mstm->tm_yday>=0 && mstm->tm_yday<=365))
                goto einval_error;
            if(*format == 'U')
                tmp = mstm->tm_wday;
            else if(!mstm->tm_wday)
                tmp = 6;
            else
                tmp = mstm->tm_wday-1;

            tmp = mstm->tm_yday/7 + (tmp<=mstm->tm_yday%7);
            if(!strftime_int(str, &ret, max, tmp, alternate ? 0 : 2, 0, 53))
                return 0;
            break;
        case '%':
            str[ret++] = '%';
            break;
        default:
            WARN("unknown format %c\n", *format);
            MSVCRT_INVALID_PMT("unknown format", MSVCRT_EINVAL);
            goto einval_error;
        }
    }

    if(ret == max) {
        if(max)
            *str = 0;
        *_errno() = MSVCRT_ERANGE;
        return 0;
    }

    str[ret] = 0;
    return ret;

einval_error:
    *str = 0;
    return 0;
}


static MSVCRT_size_t strftime_helper(char *str, MSVCRT_size_t max, const char *format,
        const struct MSVCRT_tm *mstm, MSVCRT___lc_time_data *time_data, MSVCRT__locale_t loc)
{
#if _MSVCR_VER <= 90
    TRACE("(%p %ld %s %p %p %p)\n", str, max, format, mstm, time_data, loc);
    return strftime_impl(str, max, format, mstm, time_data, loc);
#else
    MSVCRT_wchar_t *s, *fmt;
    MSVCRT_size_t len;

    TRACE("(%p %ld %s %p %p %p)\n", str, max, format, mstm, time_data, loc);

    if (!MSVCRT_CHECK_PMT(str != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(max != 0)) return 0;
    *str = 0;
    if (!MSVCRT_CHECK_PMT(format != NULL)) return 0;

    len = _mbstowcs_l( NULL, format, 0, (_locale_t)loc ) + 1;
    if (!len || !(fmt = MSVCRT_malloc( len*sizeof(MSVCRT_wchar_t) ))) return 0;
    _mbstowcs_l(fmt, format, len, (_locale_t)loc);

    if ((s = MSVCRT_malloc( max*sizeof(MSVCRT_wchar_t) )))
    {
        len = strftime_impl( s, max, fmt, mstm, time_data, loc );
        if (len)
            len = _wcstombs_l( str, s, max, (_locale_t)loc );
        MSVCRT_free( s );
    }
    else len = 0;

    MSVCRT_free( fmt );
    return len;
#endif
}

/*********************************************************************
 *		_Strftime (MSVCRT.@)
 */
MSVCRT_size_t CDECL _Strftime(char *str, MSVCRT_size_t max, const char *format,
        const struct MSVCRT_tm *mstm, MSVCRT___lc_time_data *time_data)
{
    return strftime_helper(str, max, format, mstm, time_data, NULL);
}


static MSVCRT_size_t wcsftime_helper( MSVCRT_wchar_t *str, MSVCRT_size_t max,
        const MSVCRT_wchar_t *format, const struct MSVCRT_tm *mstm,
        MSVCRT___lc_time_data *time_data, MSVCRT__locale_t loc )
{
#if _MSVCR_VER <= 90
    char *s, *fmt;
    MSVCRT_size_t len;

    TRACE("%p %ld %s %p %p %p\n", str, max, debugstr_w(format), mstm, time_data, loc);

    len = _wcstombs_l( NULL, format, 0, loc ) + 1;
    if (!(fmt = MSVCRT_malloc( len ))) return 0;
    _wcstombs_l(fmt, format, len, loc);

    if ((s = MSVCRT_malloc( max*4 )))
    {
        if (!strftime_impl( s, max*4, fmt, mstm, time_data, loc )) s[0] = 0;
        len = _mbstowcs_l( str, s, max, loc );
        MSVCRT_free( s );
    }
    else len = 0;

    MSVCRT_free( fmt );
    return len;
#else
    TRACE("%p %ld %s %p %p %p\n", str, max, debugstr_w(format), mstm, time_data, loc);
    return strftime_impl(str, max, format, mstm, time_data, loc);
#endif
}


/*********************************************************************
 *		_Wcsftime (MSVCR110.@)
 */
MSVCRT_size_t CDECL _Wcsftime(MSVCRT_wchar_t *str, MSVCRT_size_t max,
        const MSVCRT_wchar_t *format, const struct MSVCRT_tm *mstm,
        MSVCRT___lc_time_data *time_data)
{
    return wcsftime_helper(str, max, format, mstm, time_data, NULL);
}