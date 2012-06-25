/* build: gcc -o ditto.exe -mconsole -DUNICODE main.c -lntdll */

#include <stdio.h>

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

        ditto_cleanup( );

    } else {

        wprintf( L"ditto <dirA> [<dirB> [<dirC> [..]]]\n" );

    }

    return 0;
}

