#ifndef _DITTO_H_
#define _DITTO_H_

#include <string.h>

#include "list.h"

#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif


void ditto_init( void );

void ditto_cleanup( void );


void
    include_dir (
#ifdef __GNUC__  
        char *      arg
#else
        wchar_t *   arg
#endif
);


void scan( void );

void
    print_File (
        void *  file
);

void
    file_dittoer (
        struct List *   fzbuckets_list
);

void
    ditto_files (
        void
);

#endif

