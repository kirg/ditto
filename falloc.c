
#include <windows.h>
#include <stdio.h>

#include "falloc.h"



#define VIRTALLOC_MINSIZE   (2048*1024) /* 2 MiB */
#define VIRTALLOC_MAXSIZE   (65535*1024) /* 64 MiB */

#define VIRTALLOC_FACTOR    (2)

struct VirtAlloc {
    struct VirtAlloc * next;

    LPVOID  address;
    DWORD   size;
};


struct FastAllocatorContext {
    void *      free;   /* stack of free bufs */
    int         size;   /* == 0 -> variable length */

    long long   total;  /* total allocated */

    char *      next;   /* next available slot in valloc */
    char *      end;    /* last valid offset in valloc */

    wchar_t *   type;   /* user-defined identifier for falloc */

    struct VirtAlloc * vallocs;
};


void
    falloc_init (
        void
)
{
}

void
    falloc_cleanup (
        void
)
{
}

void *
    falloc (
        FastAlloc   fa,
        int         size
)
{
    struct FastAllocatorContext *  fac;
    void * buf;

    fac = (struct FastAllocatorContext *)fa;

//wprintf(L"\nfalloc(%d) for %s/%d\n", size, fac->type, fac->size);

    if (fac->size != 0)  {

        size = ((size + fac->size - 1) / fac->size) * fac->size;

        if (fac->free != NULL) {

            buf = fac->free;
            fac->free = *(void **)fac->free;

//wprintf(L"FALLOC: pop from free list: %p\n", buf);
            return buf;
        }
    }

    do {

        if ((fac->next + size) < fac->end) {

            buf = fac->next;

            fac->next += size;
            fac->total += size;

            break;

        } else {

            struct VirtAlloc *  v;
            int                 valloc_size;

            valloc_size = (fac->vallocs == NULL) ?
                VIRTALLOC_MINSIZE :
                fac->vallocs->size * VIRTALLOC_FACTOR;

            if (valloc_size > VIRTALLOC_MAXSIZE) {
                valloc_size = VIRTALLOC_MAXSIZE;
            }

            v = malloc( sizeof(struct VirtAlloc) );

            if (v) {

                v->address  = VirtualAlloc( NULL, valloc_size,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

                if (v->address) {

                    v->next         = fac->vallocs;
                    fac->vallocs    = v;

                    v->size         = valloc_size;

//wprintf(L"VirtualAlloc(%d bytes) for %s: %p\n", valloc_size, fac->type, v->address);

                    fac->next   = v->address;
                    fac->end    = (char *)v->address + v->size;

                } else {

wprintf(L"VirtualAlloc(%d bytes) for %s: failed\n", valloc_size, fac->type, v->address);
                    free(v);
                    buf = NULL;
                    break;
                }

            }

        }

    } while (1);

    if (buf == NULL) {
        wprintf(L"falloc returned NULL (type=%s)\n", fac->type);
    }

    return buf;
}

void
    ffree (
        FastAlloc   fa,
        void *      buf
)
{
    struct FastAllocatorContext * fac = (struct FastAllocatorContext *)fa;
    
    if (fac->size != 0) {
        *(void **)buf = fac->free;
        fac->free = buf;
//wprintf(L"FFREE: push to free list:%p\n", buf);
    } else {
        /* free of variable sized buffer -> do nothing */
    }
}


FastAlloc
    new_falloc (
        wchar_t *   type,
        int         size
)
{
    struct FastAllocatorContext * fac;

    fac = malloc( sizeof(struct FastAllocatorContext) );

    if (fac) {
        fac->vallocs = NULL;

        fac->total  = 0;

        fac->next   = NULL;
        fac->end    = NULL;

        fac->size   = size;
        fac->free   = NULL;

        fac->type   = type;
    }

    return (FastAlloc)fac;
}

void
    delete_falloc (
        FastAlloc   fa
)
{
    struct FastAllocatorContext * fac;

    struct VirtAlloc *  va;
    struct VirtAlloc *  next;

    fac = (struct FastAllocatorContext *)fa;

    for ( va = fac->vallocs; va != NULL; va = next ) {

        if (!VirtualFree( va->address, 0 /* va->size */, MEM_RELEASE )) {
wprintf( L"VirtualFree(%016X, %d) failed\n", va->address, va->size );
        } else {
//wprintf( L"VirtualFree(%016X, %d) done\n", va->address, va->size );
        }

        next = va->next;
        free( va );
    }

    free( fac );
}


