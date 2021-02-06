#pragma once

/* Helper macro to enable gcc's extension.  */
#ifndef __GNU_EXTENSION
 #ifdef __GNUC__
  #define __GNU_EXTENSION __extension__
 #else
  #define __GNU_EXTENSION
 #endif
#endif /* __GNU_EXTENSION */

#ifndef _ANONYMOUS_UNION
 #if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
  #define _ANONYMOUS_UNION
  #define _UNION_NAME(x) x
 #else
  #define _ANONYMOUS_UNION __GNU_EXTENSION
  #define _UNION_NAME(x)
 #endif /* NONAMELESSUNION */
#endif /* _ANONYMOUS_UNION */

#ifndef DUMMYUNIONNAME
 #if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
  #define DUMMYUNIONNAME  u
  #define DUMMYUNIONNAME1 u1
  #define DUMMYUNIONNAME2 u2
  #define DUMMYUNIONNAME3 u3
  #define DUMMYUNIONNAME4 u4
  #define DUMMYUNIONNAME5 u5
  #define DUMMYUNIONNAME6 u6
  #define DUMMYUNIONNAME7 u7
  #define DUMMYUNIONNAME8 u8
  #define DUMMYUNIONNAME9  u9
 #else
  #define DUMMYUNIONNAME
  #define DUMMYUNIONNAME1
  #define DUMMYUNIONNAME2
  #define DUMMYUNIONNAME3
  #define DUMMYUNIONNAME4
  #define DUMMYUNIONNAME5
  #define DUMMYUNIONNAME6
  #define DUMMYUNIONNAME7
  #define DUMMYUNIONNAME8
  #define DUMMYUNIONNAME9
 #endif /* NONAMELESSUNION */
#endif /* DUMMYUNIONNAME */

#ifndef _ANONYMOUS_STRUCT
 #if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
  #define _ANONYMOUS_STRUCT
  #define _STRUCT_NAME(x) x
 #else
  #define _ANONYMOUS_STRUCT __GNU_EXTENSION
  #define _STRUCT_NAME(x)
 #endif /* NONAMELESSUNION */
#endif /* _ANONYMOUS_STRUCT */

#ifndef DUMMYSTRUCTNAME
 #if defined(NONAMELESSUNION)// || !defined(_MSC_EXTENSIONS)
  #define DUMMYSTRUCTNAME s
  #define DUMMYSTRUCTNAME1 s1
  #define DUMMYSTRUCTNAME2 s2
  #define DUMMYSTRUCTNAME3 s3
  #define DUMMYSTRUCTNAME4 s4
  #define DUMMYSTRUCTNAME5 s5
 #else
  #define DUMMYSTRUCTNAME
  #define DUMMYSTRUCTNAME1
  #define DUMMYSTRUCTNAME2
  #define DUMMYSTRUCTNAME3
  #define DUMMYSTRUCTNAME4
  #define DUMMYSTRUCTNAME5
 #endif /* NONAMELESSUNION */
#endif /* DUMMYSTRUCTNAME */