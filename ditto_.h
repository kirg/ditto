#ifndef _DITTO__H_
#define _DITTO__H_

#include <string.h>

#include "list.h"

#include <windows.h>


#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif


struct File {

    struct File *       sibling;

    wchar_t *           name;
    struct List *       path;   /* path string-list (debug) */
    struct Directory *  parent;

    long long int       size;

    /* below two could be made a "union" */
    HANDLE              hF;
    struct List *       bucket;
};

struct Misc {

    struct Misc *       sibling;

    wchar_t *           name;
    struct List *       path;   /* path string-list (debug) */
    struct Directory *  parent;
};


struct SimilarDir {
    struct Directory *  dir;

    int                 score;
};



struct Directory {

    struct Directory *  sibling;

    wchar_t *           name;
    struct List *       path;   /* path string-list (debug) */
    struct Directory *  parent;

    int                 n_files;
    struct File *       files;

    int                 n_dirs;
    struct Directory *  dirs;

    int                 n_misc; // FIXME: n_links?
    struct Misc *       misc;   // FIXME: links?

    long long int       n_all_files;
    long long int       n_all_dirs;
    long long int       n_all_misc;

    struct List *       similar;
};


struct ScanDirectory {

    struct Directory    dir;

    wchar_t *           prepend;
    wchar_t *           base_path;
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
    ditto_files (
        void
);

void
    ditto_dirs (
        void
);


void
    fzhash_init (
        void
);

void
    fzhash_file (
        struct File *   file
);


void
    fzhash_cleanup (
        void
);

#endif

