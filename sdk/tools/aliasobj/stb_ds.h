/* stb_ds.h - v0.65 - public domain data structures - Sean Barrett 2019

   This is a single-header-file library that provides easy-to-use
   dynamic arrays and hash tables for C (also works in C++).

   For a gentle introduction:
      http://nothings.org/stb_ds

   To use this library, do this in *one* C or C++ file:
      #define STB_DS_IMPLEMENTATION
      #include "stb_ds.h"

TABLE OF CONTENTS

  Table of Contents
  Compile-time options
  License
  Documentation
  Notes
  Notes - Dynamic arrays
  Notes - Hash maps
  Credits

COMPILE-TIME OPTIONS

  #define STBDS_NO_SHORT_NAMES

     This flag needs to be set globally.

     By default stb_ds exposes shorter function names that are not qualified
     with the "stbds_" prefix. If these names conflict with the names in your
     code, define this flag.

  #define STBDS_SIPHASH_2_4

     This flag only needs to be set in the file containing #define STB_DS_IMPLEMENTATION.

     By default stb_ds.h hashes using a weaker variant of SipHash and a custom hash for
     4- and 8-byte keys. On 64-bit platforms, you can define the above flag to force
     stb_ds.h to use specification-compliant SipHash-2-4 for all keys. Doing so makes
     hash table insertion about 20% slower on 4- and 8-byte keys, 5% slower on
     64-byte keys, and 10% slower on 256-byte keys on my test computer.

  #define STBDS_REALLOC(context,ptr,size) better_realloc
  #define STBDS_FREE(context,ptr)         better_free

     These defines only need to be set in the file containing #define STB_DS_IMPLEMENTATION.

     By default stb_ds uses stdlib realloc() and free() for memory management. You can
     substitute your own functions instead by defining these symbols. You must either
     define both, or neither. Note that at the moment, 'context' will always be NULL.
     @TODO add an array/hash initialization function that takes a memory context pointer.

  #define STBDS_UNIT_TESTS

     Defines a function stbds_unit_tests() that checks the functioning of the data structures.

  Note that on older versions of gcc (e.g. 5.x.x) you may need to build with '-std=c++0x'
     (or equivalentally '-std=c++11') when using anonymous structures as seen on the web
     page or in STBDS_UNIT_TESTS.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

DOCUMENTATION

  Dynamic Arrays

    Non-function interface:

      Declare an empty dynamic array of type T
        T* foo = NULL;

      Access the i'th item of a dynamic array 'foo' of type T, T* foo:
        foo[i]

    Functions (actually macros)

      arrfree:
        void arrfree(T*);
          Frees the array.

      arrlen:
        ptrdiff_t arrlen(T*);
          Returns the number of elements in the array.

      arrlenu:
        size_t arrlenu(T*);
          Returns the number of elements in the array as an unsigned type.

      arrpop:
        T arrpop(T* a)
          Removes the final element of the array and returns it.

      arrput:
        T arrput(T* a, T b);
          Appends the item b to the end of array a. Returns b.

      arrins:
        T arrins(T* a, int p, T b);
          Inserts the item b into the middle of array a, into a[p],
          moving the rest of the array over. Returns b.

      arrinsn:
        void arrins(T* a, int p, int n);
          Inserts n uninitialized items into array a starting at a[p],
          moving the rest of the array over.

      arraddnptr:
        T* arraddnptr(T* a, int n)
          Appends n uninitialized items onto array at the end.
          Returns a pointer to the first uninitialized item added.

      arraddnindex:
        size_t arraddnindex(T* a, int n)
          Appends n uninitialized items onto array at the end.
          Returns the index of the first uninitialized item added.

      arrdel:
        void arrdel(T* a, int p);
          Deletes the element at a[p], moving the rest of the array over.

      arrdeln:
        void arrdel(T* a, int p, int n);
          Deletes n elements starting at a[p], moving the rest of the array over.

      arrdelswap:
        void arrdelswap(T* a, int p);
          Deletes the element at a[p], replacing it with the element from
          the end of the array. O(1) performance.

      arrsetlen:
        void arrsetlen(T* a, int n);
          Changes the length of the array to n. Allocates uninitialized
          slots at the end if necessary.

      arrsetcap:
        size_t arrsetcap(T* a, int n);
          Sets the length of allocated storage to at least n. It will not
          change the length of the array.

      arrcap:
        size_t arrcap(T* a);
          Returns the number of total elements the array can contain without
          needing to be reallocated.

  Hash maps & String hash maps

    Given T is a structure type: struct { TK key; TV value; }. Note that some
    functions do not require TV value and can have other fields. For string
    hash maps, TK must be 'char *'.

    Special interface:

      stbds_rand_seed:
        void stbds_rand_seed(size_t seed);
          For security against adversarially chosen data, you should seed the
          library with a strong random number. Or at least seed it with time().

      stbds_hash_string:
        size_t stbds_hash_string(char *str, size_t seed);
          Returns a hash value for a string.

      stbds_hash_bytes:
        size_t stbds_hash_bytes(void *p, size_t len, size_t seed);
          These functions hash an arbitrary number of bytes. The function
          uses a custom hash for 4- and 8-byte data, and a weakened version
          of SipHash for everything else. On 64-bit platforms you can get
          specification-compliant SipHash-2-4 on all data by defining
          STBDS_SIPHASH_2_4, at a significant cost in speed.

    Non-function interface:

      Declare an empty hash map of type T
        T* foo = NULL;

      Access the i'th entry in a hash table T* foo:
        foo[i]

    Function interface (actually macros):

      hmfree
      shfree
        void hmfree(T*);
        void shfree(T*);
          Frees the hashmap and sets the pointer to NULL.

      hmlen
      shlen
        ptrdiff_t hmlen(T*)
        ptrdiff_t shlen(T*)
          Returns the number of elements in the hashmap.

      hmlenu
      shlenu
        size_t hmlenu(T*)
        size_t shlenu(T*)
          Returns the number of elements in the hashmap.

      hmgeti
      shgeti
      hmgeti_ts
        ptrdiff_t hmgeti(T*, TK key)
        ptrdiff_t shgeti(T*, char* key)
        ptrdiff_t hmgeti_ts(T*, TK key, ptrdiff_t tempvar)
          Returns the index in the hashmap which has the key 'key', or -1
          if the key is not present.

      hmget
      hmget_ts
      shget
        TV hmget(T*, TK key)
        TV shget(T*, char* key)
        TV hmget_ts(T*, TK key, ptrdiff_t tempvar)
          Returns the value corresponding to 'key' in the hashmap.
          The structure must have a 'value' field

      hmgets
      shgets
        T hmgets(T*, TK key)
        T shgets(T*, char* key)
          Returns the structure corresponding to 'key' in the hashmap.

      hmgetp
      shgetp
      hmgetp_ts
      hmgetp_null
      shgetp_null
        T* hmgetp(T*, TK key)
        T* shgetp(T*, char* key)
        T* hmgetp_ts(T*, TK key, ptrdiff_t tempvar)
        T* hmgetp_null(T*, TK key)
        T* shgetp_null(T*, char *key)
          Returns a pointer to the structure corresponding to 'key' in
          the hashmap. Functions ending in "_null" return NULL if the key
          is not present in the hashmap; the others return a pointer to a
          structure holding the default value (but not the searched-for key).

      hmdefault
      shdefault
        TV hmdefault(T*, TV value)
        TV shdefault(T*, TV value)
          Sets the default value for the hashmap, the value which will be
          returned by hmget/shget if the key is not present.

      hmdefaults
      shdefaults
        TV hmdefaults(T*, T item)
        TV shdefaults(T*, T item)
          Sets the default struct for the hashmap, the contents which will be
          returned by hmgets/shgets if the key is not present.

      hmput
      shput
        TV hmput(T*, TK key, TV value)
        TV shput(T*, char* key, TV value)
          Inserts a <key,value> pair into the hashmap. If the key is already
          present in the hashmap, updates its value.

      hmputs
      shputs
        T hmputs(T*, T item)
        T shputs(T*, T item)
          Inserts a struct with T.key into the hashmap. If the struct is already
          present in the hashmap, updates it.

      hmdel
      shdel
        int hmdel(T*, TK key)
        int shdel(T*, char* key)
          If 'key' is in the hashmap, deletes its entry and returns 1.
          Otherwise returns 0.

    Function interface (actually macros) for strings only:

      sh_new_strdup
        void sh_new_strdup(T*);
          Overwrites the existing pointer with a newly allocated
          string hashmap which will automatically allocate and free
          each string key using realloc/free

      sh_new_arena
        void sh_new_arena(T*);
          Overwrites the existing pointer with a newly allocated
          string hashmap which will automatically allocate each string
          key to a string arena. Every string key ever used by this
          hash table remains in the arena until the arena is freed.
          Additionally, any key which is deleted and reinserted will
          be allocated multiple times in the string arena.

NOTES

  * These data structures are realloc'd when they grow, and the macro
    "functions" write to the provided pointer. This means: (a) the pointer
    must be an lvalue, and (b) the pointer to the data structure is not
    stable, and you must maintain it the same as you would a realloc'd
    pointer. For example, if you pass a pointer to a dynamic array to a
    function which updates it, the function must return back the new
    pointer to the caller. This is the price of trying to do this in C.

  * The following are the only functions that are thread-safe on a single data
    structure, i.e. can be run in multiple threads simultaneously on the same
    data structure
        hmlen        shlen
        hmlenu       shlenu
        hmget_ts     shget_ts
        hmgeti_ts    shgeti_ts
        hmgets_ts    shgets_ts

  * You iterate over the contents of a dynamic array and a hashmap in exactly
    the same way, using arrlen/hmlen/shlen:

      for (i=0; i < arrlen(foo); ++i)
         ... foo[i] ...

  * All operations except arrins/arrdel are O(1) amortized, but individual
    operations can be slow, so these data structures may not be suitable
    for real time use. Dynamic arrays double in capacity as needed, so
    elements are copied an average of once. Hash tables double/halve
    their size as needed, with appropriate hysteresis to maintain O(1)
    performance.

NOTES - DYNAMIC ARRAY

  * If you know how long a dynamic array is going to be in advance, you can avoid
    extra memory allocations by using arrsetlen to allocate it to that length in
    advance and use foo[n] while filling it out, or arrsetcap to allocate the memory
    for that length and use arrput/arrpush as normal.

  * Unlike some other versions of the dynamic array, this version should
    be safe to use with strict-aliasing optimizations.

NOTES - HASH MAP

  * For compilers other than GCC and clang (e.g. Visual Studio), for hmput/hmget/hmdel
    and variants, the key must be an lvalue (so the macro can take the address of it).
    Extensions are used that eliminate this requirement if you're using C99 and later
    in GCC or clang, or if you're using C++ in GCC. But note that this can make your
    code less portable.

  * To test for presence of a key in a hashmap, just do 'hmgeti(foo,key) >= 0'.

  * The iteration order of your data in the hashmap is determined solely by the
    order of insertions and deletions. In particular, if you never delete, new
    keys are always added at the end of the array. This will be consistent
    across all platforms and versions of the library. However, you should not
    attempt to serialize the internal hash table, as the hash is not consistent
    between different platforms, and may change with future versions of the library.

  * Use sh_new_arena() for string hashmaps that you never delete from. Initialize
    with NULL if you're managing the memory for your strings, or your strings are
    never freed (at least until the hashmap is freed). Otherwise, use sh_new_strdup().
    @TODO: make an arena variant that garbage collects the strings with a trivial
    copy collector into a new arena whenever the table shrinks / rebuilds. Since
    current arena recommendation is to only use arena if it never deletes, then
    this can just replace current arena implementation.

  * If adversarial input is a serious concern and you're on a 64-bit platform,
    enable STBDS_SIPHASH_2_4 (see the 'Compile-time options' section), and pass
    a strong random number to stbds_rand_seed.

  * The default value for the hash table is stored in foo[-1], so if you
    use code like 'hmget(T,k)->value = 5' you can accidentally overwrite
    the value stored by hmdefault if 'k' is not present.

CREDITS

  Sean Barrett -- library, idea for dynamic array API/implementation
  Per Vognsen  -- idea for hash table API/implementation
  Rafael Sachetto -- arrpop()
  github:HeroicKatora -- arraddn() reworking

  Bugfixes:
    Andy Durdin
    Shane Liesegang
    Vinh Truong
    Andreas Molzer
    github:hashitaku
    github:srdjanstipic
*/

#ifndef INCLUDE_STB_DS_H
#define INCLUDE_STB_DS_H

#include <stddef.h>
#include <string.h>

#ifndef STBDS_NO_SHORT_NAMES
#define arrlen      stbds_arrlen
#define arrlenu     stbds_arrlenu
#define arrput      stbds_arrput
#define arrpush     stbds_arrput
#define arrpop      stbds_arrpop
#define arrfree     stbds_arrfree
#define arraddn     stbds_arraddn // deprecated, use one of the following instead:
#define arraddnptr  stbds_arraddnptr
#define arraddnindex stbds_arraddnindex
#define arrsetlen   stbds_arrsetlen
#define arrlast     stbds_arrlast
#define arrins      stbds_arrins
#define arrinsn     stbds_arrinsn
#define arrdel      stbds_arrdel
#define arrdeln     stbds_arrdeln
#define arrdelswap  stbds_arrdelswap
#define arrcap      stbds_arrcap
#define arrsetcap   stbds_arrsetcap
#endif

#if defined(STBDS_REALLOC) && !defined(STBDS_FREE) || !defined(STBDS_REALLOC) && defined(STBDS_FREE)
#error "You must define both STBDS_REALLOC and STBDS_FREE, or neither."
#endif
#if !defined(STBDS_REALLOC) && !defined(STBDS_FREE)
#include <stdlib.h>
#define STBDS_REALLOC(c,p,s) realloc(p,s)
#define STBDS_FREE(c,p)      free(p)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    ///////////////
    //
    // Everything below here is implementation details
    //

    extern void* stbds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap);

#ifdef __cplusplus
}
#endif

#define STBDS_OFFSETOF(var,field)           ((char *) &(var)->field - (char *) (var))

#define stbds_header(t)  ((stbds_array_header *) (t) - 1)
#define stbds_temp(t)    stbds_header(t)->temp
#define stbds_temp_key(t) (*(char **) stbds_header(t)->hash_table)

#define stbds_arrsetcap(a,n)  (stbds_arrgrow(a,0,n))
#define stbds_arrsetlen(a,n)  ((stbds_arrcap(a) < (size_t) (n) ? stbds_arrsetcap((a),(size_t)(n)),0 : 0), (a) ? stbds_header(a)->length = (size_t) (n) : 0)
#define stbds_arrcap(a)       ((a) ? stbds_header(a)->capacity : 0)
#define stbds_arrlen(a)       ((a) ? (ptrdiff_t) stbds_header(a)->length : 0)
#define stbds_arrlenu(a)      ((a) ?             stbds_header(a)->length : 0)
#define stbds_arrput(a,v)     (stbds_arrmaybegrow(a,1), (a)[stbds_header(a)->length++] = (v))
#define stbds_arrpush         stbds_arrput  // synonym
#define stbds_arrpop(a)       (stbds_header(a)->length--, (a)[stbds_header(a)->length])
#define stbds_arraddn(a,n)    ((void)(stbds_arraddnoff(a, n)))    // deprecated, use one of the following instead:
#define stbds_arraddnptr(a,n) (stbds_arrmaybegrow(a,n), stbds_header(a)->length += (n), &(a)[stbds_header(a)->length-(n)])
#define stbds_arraddnoff(a,n) (stbds_arrmaybegrow(a,n), stbds_header(a)->length += (n), stbds_header(a)->length-(n))
#define stbds_arrlast(a)      ((a)[stbds_header(a)->length-1])
#define stbds_arrfree(a)      ((void) ((a) ? STBDS_FREE(NULL,stbds_header(a)) : (void)0), (a)=NULL)
#define stbds_arrdel(a,i)     stbds_arrdeln(a,i,1)
#define stbds_arrdeln(a,i,n)  (memmove(&(a)[i], &(a)[(i)+(n)], sizeof *(a) * (stbds_header(a)->length-(n)-(i))), stbds_header(a)->length -= (n))
#define stbds_arrdelswap(a,i) ((a)[i] = stbds_arrlast(a), stbds_header(a)->length -= 1)
#define stbds_arrinsn(a,i,n)  (stbds_arraddn((a),(n)), memmove(&(a)[(i)+(n)], &(a)[i], sizeof *(a) * (stbds_header(a)->length-(n)-(i))))
#define stbds_arrins(a,i,v)   (stbds_arrinsn((a),(i),1), (a)[i]=(v))

#define stbds_arrmaybegrow(a,n)  ((!(a) || stbds_header(a)->length + (n) > stbds_header(a)->capacity) \
                                  ? (stbds_arrgrow(a,n,0),0) : 0)

#define stbds_arrgrow(a,b,c)   ((a) = stbds_arrgrowf_wrapper((a), sizeof *(a), (b), (c)))

typedef struct
{
    size_t      length;
    size_t      capacity;
    void* hash_table;
    ptrdiff_t   temp;
} stbds_array_header;

#ifdef __cplusplus
// in C we use implicit assignment from these void*-returning functions to T*.
// in C++ these templates make the same code work
template<class T> static T* stbds_arrgrowf_wrapper(T* a, size_t elemsize, size_t addlen, size_t min_cap) {
    return (T*)stbds_arrgrowf((void*)a, elemsize, addlen, min_cap);
}
#else
#define stbds_arrgrowf_wrapper            stbds_arrgrowf
#endif

#endif // INCLUDE_STB_DS_H


//////////////////////////////////////////////////////////////////////////////
//
//   IMPLEMENTATION
//

#ifdef STB_DS_IMPLEMENTATION
#include <assert.h>
#include <string.h>

#ifndef STBDS_ASSERT
#define STBDS_ASSERT_WAS_UNDEFINED
#define STBDS_ASSERT(x)   ((void) 0)
#endif

//
// stbds_arr implementation
//

void* stbds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap)
{
    void* b;
    size_t min_len = stbds_arrlen(a) + addlen;

    // compute the minimum capacity needed
    if (min_len > min_cap)
        min_cap = min_len;

    if (min_cap <= stbds_arrcap(a))
        return a;

    // increase needed capacity to guarantee O(1) amortized
    if (min_cap < 2 * stbds_arrcap(a))
        min_cap = 2 * stbds_arrcap(a);
    else if (min_cap < 4)
        min_cap = 4;

    b = STBDS_REALLOC(NULL, (a) ? stbds_header(a) : 0, elemsize * min_cap + sizeof(stbds_array_header));
    b = (char*)b + sizeof(stbds_array_header);
    if (a == NULL) {
        stbds_header(b)->length = 0;
        stbds_header(b)->hash_table = 0;
    }
    stbds_header(b)->capacity = min_cap;

    return b;
}

#endif

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2019 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
