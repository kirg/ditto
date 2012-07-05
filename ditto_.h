#ifndef _DITTO__H_
#define _DITTO__H_

#include <string.h>

#include "list.h"


#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif


struct File {

    struct File *       next;   /* next in bucket */

    wchar_t *           name;
    struct List *       path;   /* path string-list (debug) */
    struct Directory *  parent;
    struct File *       sibling;

    long long int       size;

    void *              context;
};

struct Misc {

    struct Misc *       next;   /* next in bucket */

    wchar_t *           name;
    struct List *       path;   /* path string-list (debug) */
    struct Directory *  parent;
    struct Misc *       sibling;
};


struct Directory {

    struct Directory *  next;   /* next in bucket */

    wchar_t *           name;
    struct List *       path;   /* path string-list (debug) */
    struct Directory *  parent;
    struct Directory *  sibling;

    int                 n_files;
    struct File *       files;

    int                 n_dirs;
    struct Directory *  dirs;

    int                 n_misc; // FIXME: n_links?
    struct Misc *       misc;   // FIXME: links?
};


struct ScanDirectory {

    struct Directory    dir;

    wchar_t *           prepend;
};


struct FilesizeBucket {
    long long int   size;   /* size of files in the bucket */
    long long int   offs;   /* offset upto which files are ditto */

    struct List *   files;  /* the list of files */
};

struct PreDittoContext {
    struct PreDittoContext *   next;

    int             checksum;
    int             diff_at;    /* for two bufs with same checksum that differ; diff_at -> buf offs */

    char *          buf;
    int             len;

    //long long int   size;
    //long long int   offs;

    struct List *   files; /* list of 'struct File's */
};




int
    build_tree (
        struct Directory *  this
/*      wchar_t *           path,   
        int                 path_len */
);

void
    print_tree (
        struct Directory *  dir
);

void
    list_files (
        struct List *   bucket
);

void
    hash_file (
        struct File *   file
);

#endif

