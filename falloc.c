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

    if (size < fac->size) {

        buf == NULL;

    } else {

        // FIXME: make size multiple of fac->size

        if (fac->free != NULL) {

            buf = fac->free;
            fac->free = *(void **)fac->free;

        } else {

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

                            v->next     = fac->vallocs;
                            fac->vallocs = v;

                            v->size     = valloc_size;

                            wprintf(L"VirtualAlloc(%d bytes) for %s: %p\n", valloc_size, fac->type, v->address);

                            fac->next   = v->address;
                            fac->end    = (char *)v->address + v->size;

                        } else {

                            free(v);
                        }

                    }

                }

            } while (1);
        }

        if (buf == NULL) {
            wprintf(L"falloc returned NULL (type=%s)\n", fac->type);
        }

        return buf;
}

#if 0
void *
    xalloc (
        FastAlloc   fa
)
{
    struct FastAllocatorContext * fac = (struct FastAllocatorContext *)fa;
    void * buf;

    if (fac->size != 0) {

        if (fac->free != NULL) {

            buf = fac->free;
            fac->free = *(void **)fac->free;

        } else {

            buf = falloc( fa, fac->size );

        }

    } else {
        buf = NULL;
    }

    return buf;
}
#endif

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
            wprintf( L"VirtualFree(%016X, %d) done\n", va->address, va->size );
        }

        next = va->next;
        free( va );
    }

    free( fac );
}

