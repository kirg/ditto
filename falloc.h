#ifndef _FALLOC_H_
#define _FALLOC_H_

#include <string.h>

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



#endif
