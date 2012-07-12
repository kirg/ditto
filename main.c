/* build: gcc -o ditto.exe -mconsole -DUNICODE main.c -lntdll */

#include <stdio.h>

#include "ditto.h"

#ifdef __GNUC__  
int
    main (
        int     argc,
        char *  argv[]
)
#else
int
    wmain (
        int         argc,
        wchar_t *   argv[]
)
#endif
{
    int i;

    if (argc > 1) {

        ditto_init( );

        for (i = 1; i < argc; ++i) {
            ditto_dir( argv[i] );
        }

        ditto_start( );

        ditto_cleanup( );

    } else {

        wprintf( L"ditto <dirA> [<dirB> [<dirC> [..]]]\n" );

    }

    return 0;
}

