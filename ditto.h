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


void include_dir( wchar_t * path );

void include_dirA( char * path );


void scan( void );

void
    print_File (
        void *  file
);

void
    dittoer (
        long long int   size,
        struct List *   file_list
);

#endif
