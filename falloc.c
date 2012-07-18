
#include <windows.h>
#include <stdio.h>

#include "falloc.h"



#define VIRTALLOC_MINSIZE   (64*1024) /* 64 kiB */
#define VIRTALLOC_MAXSIZE   (16384*1024) /* 16 MiB */

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

    int         push;
    int         new;
    int         pop;
    int         temp;

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

    if (fac->size != 0)  {

        if (fac->free != NULL && size == fac->size) {

            ++fac->pop;

            buf = fac->free;
            fac->free = *(void **)buf;

            fac->total += size;

            return buf;

        } else {
            /* round off to multiple of fac->size, for alignment issues */
            size = ((size + fac->size - 1) / fac->size) * fac->size;
        }

    }

    do {

        if ((fac->next + size) < fac->end) {

            buf = fac->next;

            ++fac->new;

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
        wprintf(L"enter to continue .."); getchar( );
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
        ++fac->push;

        *(void **)buf = fac->free;
        fac->free = buf;

        fac->total -= fac->size;
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

        fac->new = fac->pop = fac->push = fac->temp = 0;
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

wprintf( L"delete_Falloc %s %d\n", fac->type, fac->total );

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

void
    falloc_stat (
        FastAlloc   fa
)
{
    struct FastAllocatorContext * fac = (struct FastAllocatorContext *)fa;
    wprintf(L"falloc [type=\"%s\",size=%d] total=%I64d, new=%d push=%d pop=%d\n", fac->type, fac->size, fac->total, fac->new, fac->push, fac->pop);

    if ((fac->push + fac->temp) != (fac->pop + fac->new)) {
        wprintf(L"XXXXX    falloc buffers unaccounted for: %d\n", fac->new + fac->pop - fac->push - fac->temp); getchar();
        fac->temp = fac->new + fac->pop - fac->push;
    } else {
        wprintf(L"#####    falloc buffers accounted for\n");
    }
}

