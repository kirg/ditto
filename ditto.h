#ifndef _DITTO_H_
#define _DITTO_H_

#include <string.h>

#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif


void
    ditto_init (
        void
);



#ifdef __GNUC__  
/* mingw passes in 'char *' argv */
void
    ditto_dir (
        char *      arg
);
#else
void
    ditto_dir (
        wchar_t *   arg
);
#endif


void
    ditto_start (
        void
);

void
    ditto_cleanup (
        void
);


#endif

