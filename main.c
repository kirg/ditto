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
#ifdef __GNUC__  
            include_dirA( argv[i] );
#else
            include_dir( argv[i] );
#endif
        }

        scan( );

        {
            void
                dump_hash (
                    long long int min_size
            );

#if 1 //dbg
          //print_tree( NULL );
          //list_files( NULL );
          dump_hash( 0 );
#endif
        }

        ditto_cleanup( );

    } else {

        wprintf( L"ditto <dirA> [<dirB> [<dirC> [..]]]\n" );

    }

    return 0;
}

