#ifndef _DITTO__H_
#define _DITTO__H_

#include <string.h>

#include "list.h"


struct File {

    struct File *       next;   /* next in bucket */

    wchar_t *           name;
    struct Directory *  parent;
    struct File *       sibling;


    long long int       size;
};

struct Misc {

    struct Misc *       next;   /* next in bucket */

    wchar_t *           name;
    struct Directory *  parent;
    struct Misc *       sibling;
};


struct Directory {

    struct Directory *  next;   /* next in bucket */

    wchar_t *           name;
    struct Directory *  parent;
    struct Directory *  sibling;
    
    int                 n_files;
    struct File *       files;

    int                 n_dirs;
    struct Directory *  dirs;

    int                 n_misc; // FIXME: n_links?
    struct Misc *       misc;   // FIXME: links?
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
        struct Head *   bucket
);

void
    hash_files (
        struct Head *   bucket
);

#endif

