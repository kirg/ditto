#include <windows.h>

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
    struct VirtAlloc * allocs;

    long long   total;

    char *      next_free;
    char *      end;

    wchar_t *   type;
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

    do {

        if ((fac->next_free + size) < fac->end) {

            buf = fac->next_free;

            fac->next_free += size;
            fac->total += size;

            break;

        } else {

            struct VirtAlloc *  v;
            int                 valloc_size;

            valloc_size = (fac->allocs == NULL) ?
                                VIRTALLOC_MINSIZE :
                                    fac->allocs->size * VIRTALLOC_FACTOR;

            if (valloc_size > VIRTALLOC_MAXSIZE) {
                valloc_size = VIRTALLOC_MAXSIZE;
            }

            v = malloc( sizeof(struct VirtAlloc) );

            if (v) {

                v->address  = VirtualAlloc( NULL, valloc_size,
                                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

                if (v->address) {

                    v->next     = fac->allocs;
                    fac->allocs = v;

                    v->size     = valloc_size;

wprintf(L"VirtualAlloc(%d bytes) for %s: %p\n", valloc_size, fac->type, v->address);

                    fac->next_free  = v->address;
                    fac->end        = (char *)v->address + v->size;

                } else {

                    free(v);
                }

            }

        }

    } while (1);

    return buf;
}

FastAlloc
    new_FastAllocator (
        wchar_t *   type
)
{
    struct FastAllocatorContext * fac;

    fac = malloc( sizeof(struct FastAllocatorContext) );

    if (fac) {
        fac->allocs     = NULL;

        fac->total      = 0;

        fac->next_free  = NULL;
        fac->end        = NULL;

        fac->type       = type;
    }

    return (FastAlloc)fac;
}

void
    free_FastAllocator (
        FastAlloc   fa
)
{
    struct FastAllocatorContext * fac = (struct FastAllocatorContext *)fa;

    struct VirtAlloc *  va;
    struct VirtAlloc *  next;

    for ( va = fac->allocs; va != NULL; va = next ) {

        if (!VirtualFree( va->address, 0 /* va->size */, MEM_RELEASE )) {
            wprintf( L"VirtualFree(%016X, %d) failed\n", va->address, va->size );
        } else {
            wprintf( L"VirtualFree(%016X, %d) done\n", va->address, va->size );
        }

        next = va->next;
        free( va );
    }

    free( fac );
}

