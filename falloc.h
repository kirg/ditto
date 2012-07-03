#ifndef _FALLOC_H_
#define _FALLOC_H_

#include <string.h>
#include <windows.h>


#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif


typedef void *  FastAlloc;

void
    falloc_init (
        void
);

void
    falloc_cleanup (
        void
);

FastAlloc
    new_falloc (
        wchar_t *   type,
        int         size     // 0 -> variable length, else fixed length
);

void
    delete_falloc (
        FastAlloc   fa
);

void *
    falloc (
        FastAlloc   fa,
        int         size
);

void
    ffree (
        FastAlloc   fa,
        void *      buf
);

void
    falloc_stat (
        FastAlloc   fa
);

static inline
void *
    valloc (
        int size
)
{
    return VirtualAlloc( NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
}

static inline
void
    vfree (
        void *  buf
)
{
    VirtualFree( buf, 0, MEM_RELEASE );
}


#endif
