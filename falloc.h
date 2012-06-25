#ifndef _FALLOC_H_
#define _FALLOC_H_

#include <windows.h>

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
    new_FastAllocator (
        wchar_t *   type
);

void
    free_FastAllocator (
        FastAlloc   fa
);

void *
    falloc (
        FastAlloc   fa,
        int         size
);

#endif
