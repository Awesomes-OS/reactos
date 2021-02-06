#include <precomp.h>
#include <mbstring.h>

/*
 * @implemented
 */
extern __inline int (__cdecl isleadbyte)(int const c)
{
    return _isctype( c, _LEADBYTE );
}
