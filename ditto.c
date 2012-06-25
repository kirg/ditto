#include "ditto.h"
#include "ditto_.h"

#include <stdio.h>
#include <windows.h>

// #include <windef.h>
// #include <winnt.h>
// #include <ntdll.h>
// #include <ntdef.h>
#include <ddk/ntifs.h>


#define MAX_PATHNAME_LEN (32768)

#define FILE_DIRECTORY_INFORMATION_BUFSIZE (65536)



/* scratch variables */
FILE_DIRECTORY_INFORMATION * fdi_buf;

struct Head scan_paths;


FastAlloc fa_String;
FastAlloc fa_File;
FastAlloc fa_Directory;

static inline
wchar_t *
    new_String (
        int length
)
{
    return falloc( fa_String, length * sizeof(wchar_t) );
}

static inline
struct File *
    new_File (
        void
)
{
    return falloc( fa_File, sizeof(struct File) );
}

static inline
struct Directory *
    new_Directory (
        void
)
{
    return falloc( fa_Directory, sizeof(struct Directory) );
}

/* passed into build_tree implicitly; globals to reduce stack context */
wchar_t     build_tree_path[MAX_PATHNAME_LEN];
int         build_tree_path_len;



struct Head all_files;
struct Head all_dirs;
struct Head all_misc;


void
    ditto_init (
        void
)
{
    falloc_init( );
    list_init( );
    hashmap_init( );

    fa_String       = new_FastAllocator( L"String" );
    fa_File         = new_FastAllocator( L"File" );
    fa_Directory    = new_FastAllocator( L"Directory" );
}

void
    ditto_cleanup (
        void
)
{
    free_FastAllocator( fa_Directory );
    free_FastAllocator( fa_File );
    free_FastAllocator( fa_String );

    hashmap_cleanup( );
    list_cleanup( );
    falloc_cleanup( );
}

void
    include_dir (
        wchar_t *   pathW
)
{
    wchar_t *           path;
    struct Directory *  dir;
    struct Link *       link;

    path    = new_String( wcslen(pathW) + 1);
    dir     = new_Directory( );
    link    = new_Link( );

    if (path && dir && link) {
        int i;
        int len;

        for (i = 0; pathW[i] != 0; ++i) {
            path[i] = pathW[i];
        }

        len = i;

        /* check and remove trailing '\' */
        if (path[len-1] == L'\\') {
            --len;
        }

        path[len] = 0;

        dir->name       = path;
        dir->parent     = NULL;
        dir->sibling    = NULL;

        dir->n_files    = 0;
        dir->files      = NULL;

        dir->n_dirs     = 0;
        dir->dirs       = NULL;

        dir->n_misc     = 0;
        dir->misc       = NULL;

        add_list( &scan_paths, dir );

wprintf(L"include_dir: %s\n", dir->name);
    } else {
        wprintf(L"falloc path/dir/link failed\n");
    }
}

void
    include_dirA (
        char *      pathA
)
{
    wchar_t *           path;
    struct Directory *  dir;
    struct Link *       link;

    path    = new_String( strlen(pathA) + 1 );
    dir     = new_Directory( );
    link    = new_Link( );

    if (path && dir && link) {
        int i;

        for (i = 0; pathA[i] != 0; ++i) {
            path[i] = pathA[i];
        }

        /* check and remove trailing '\' */
        if (path[i-1] == L'\\') {
            --i;
        }

        path[i] = 0;

        dir->name       = path;
        dir->parent     = NULL;
        dir->sibling    = NULL;

        dir->n_files    = 0;
        dir->files      = NULL;

        dir->n_dirs     = 0;
        dir->dirs       = NULL;

        dir->n_misc     = 0;
        dir->misc       = NULL;

wprintf(L"include_dirA: %s\n", dir->name);
        add_list( &scan_paths, dir );
    } else {
        wprintf(L"malloc path/dir/link failed\n");
    }
}


void
    scan (
        void
)
{
    struct Link *       link;

    fdi_buf = malloc( FILE_DIRECTORY_INFORMATION_BUFSIZE );

    if (fdi_buf == NULL) {
        wprintf(L"malloc fdi_buf failed\n");
        return;
    }

wprintf(L"scan\n");

    for (link = scan_paths.first; link != NULL; link = link->next) {
        struct Directory * dir;

        dir = link->data;

wprintf(L"\nscan dir=\"%s\"\n", dir->name);

        wcsncpy( build_tree_path, dir->name, MAX_PATHNAME_LEN );
        build_tree_path_len = wcslen( build_tree_path );

        build_tree( dir );
    }

wprintf(L"tree: %d dirs, %d files, %d links\n", all_dirs.count, all_files.count, all_misc.count);

//wprintf(L"enter to continue .."); getchar();


    for (link = scan_paths.first; link != NULL; link = link->next) {
        struct Directory * dir;

        dir = link->data;

        //print_tree( dir );

    }

//wprintf(L"enter to continue .."); getchar();
wprintf(L"files: %d files\n", all_files.count);

    //list_files( &all_files );

    hash_files( &all_files );

wprintf(L"hashing done\n");

    find_dittos( );

    dump_hash( );
}


int
    build_tree (
        struct Directory *  this
/*      wchar_t *           build_tree_path,   
        int                 build_tree_path_len */
)
{
/* 'build_tree_path' and 'build_tree_path_len' passed in as globals, to reduce stack context */

    HANDLE  hD;

    /* add trailing '\' */
    build_tree_path[build_tree_path_len]      = L'\\';
    build_tree_path[++build_tree_path_len]    = 0;

//wprintf( L"traverse: %s (%d)\n", build_tree_path, build_tree_path_len );

    this->n_files   = 0;
    this->files     = NULL;

    this->n_dirs    = 0;
    this->dirs      = NULL;

    this->n_misc    = 0;
    this->misc      = NULL;


    /* using pre-setup 'build_tree_path' and 'build_tree_path_len' globals */

    hD = CreateFile( build_tree_path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_CREATE_TREE_CONNECTION, NULL );

    if (hD != INVALID_HANDLE_VALUE) {

        IO_STATUS_BLOCK                 iosb;
        NTSTATUS                        status;

        /* enumerate children */
        status = NtQueryDirectoryFile( hD, NULL, NULL, NULL, &iosb, fdi_buf,
                    FILE_DIRECTORY_INFORMATION_BUFSIZE, FileDirectoryInformation, FALSE, NULL, TRUE );

        while (status == STATUS_SUCCESS) {

            FILE_DIRECTORY_INFORMATION *    fdi;

            for (fdi = fdi_buf; 1; fdi = (FILE_DIRECTORY_INFORMATION *)((char *)fdi + fdi->NextEntryOffset)) {

                wchar_t *   name;
                int         len;

                len = fdi->FileNameLength / sizeof(wchar_t);

                /* skip '.' and '..' entries */
                if ((fdi->FileName[0] == L'.') && ((len == 1) ||
                        ((len == 2) && fdi->FileName[1] == L'.'))) {

                    if (fdi->NextEntryOffset == 0) {
                        break;
                    }

                    continue;
                }

                name = new_String( len + 1 );

                if (name == NULL) {

                    if (fdi->NextEntryOffset == 0) {
                        break;
                    }

                    continue; // FIXME: break?
                }

                wcsncpy( name, fdi->FileName, len );
                name[ len ] = 0;

                if (fdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                    struct Directory *  dir;

                    dir = new_Directory( );

                    if (dir != NULL) {

                        dir->name       = name;
                        dir->parent     = this;

                        /* add to child-dir list */
                        dir->sibling    = this->dirs;

                        this->dirs      = dir;
                        ++this->n_dirs;

                        /* add to all_dirs bucket */
                        add_list( &all_dirs, dir );

                    } else {
                        wprintf( L"malloc( sizeof(struct Directory) ) failed\n");
                        return -1;
                    }
                    
                } else if (fdi->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {

                    struct Misc *   misc;

                    misc = malloc( sizeof(struct Misc) );


                    if (misc != NULL) {

                        misc->name      = name;
                        misc->parent    = this;

                        /* add to child-misc list */
                        misc->sibling   = this->misc;

                        this->misc      = misc;
                        ++this->n_misc;

                        /* add to all_misc bucket */
                        add_list( &all_misc, misc );

                    } else {
                        wprintf( L"malloc( sizeof(struct Link) ) failed\n");
                        return -2;
                    }


                } else {

                    struct File *   file;

                    //file = malloc( sizeof(struct File) );
                    file = new_File( );

                    if (file != NULL) {

                        file->name      = name;
                        file->parent    = this;

                        /* add to child-files list */
                        file->sibling   = this->files;

                        this->files     = file;
                        ++this->n_files;

                        /* add to all_files bucket */
                        add_list( &all_files, file );

                        file->size = (long long int)fdi->EndOfFile.QuadPart;

                    } else {
                        wprintf( L"malloc( sizeof(struct File) ) failed\n");
                        return -3;
                    }
                }

                if (fdi->NextEntryOffset == 0) {
                    break;
                }
            }

            status = NtQueryDirectoryFile( hD, NULL, NULL, NULL, &iosb, fdi_buf,
                    FILE_DIRECTORY_INFORMATION_BUFSIZE, FileDirectoryInformation, FALSE, NULL, FALSE );
        }

        CloseHandle( hD );


        /* traverse child dirs */

        {
            int path_len0;
            struct Directory * dir;

            /* save original/incoming build_tree_path-len */
            path_len0 = build_tree_path_len;

            for (dir = this->dirs; dir != NULL; dir = dir->sibling) {

                /* setup globals 'build_tree_path' and 'build_tree_path_len' for recursive call */
                wcsncpy( build_tree_path + path_len0, dir->name, MAX_PATHNAME_LEN - path_len0 );

                build_tree_path_len = path_len0 + wcslen(dir->name);
                build_tree_path[build_tree_path_len] = 0;

                build_tree( dir /*, build_tree_path, build_tree_path_len + 1 + len */ );
            }

            /* restore 'build_tree_path' and 'build_tree_path_len' globals */
            /* build_tree_path_len = path_len0 - 1;
               build_tree_path[build_tree_path_len] = 0; */ /* not necessary; ie, not relied upon */
        }


    } else {
//wprintf(L"error opening directory: %s\n", build_tree_path);
    }

    return 0;
}


void
    print_tree (
        struct Directory *  dir
)
{
    static wchar_t prefix[1024];
    static int prefix_len = 0;

    struct File *       f;
    struct Misc *       m;
    struct Directory *  d;

    int    old_prefix_len;

    wprintf( L"%s%s [%d dirs, %d files, %d links]\n", prefix, dir->name, dir->n_dirs, dir->n_files, dir->n_misc );

    if (prefix_len == 0) {
        wcscpy(prefix, L"|-----");
    } else {
        wcscpy(prefix + prefix_len - 5, L"     |-----");
    }

    prefix_len += 6;

    for (f = dir->files; f != NULL; f = f->sibling) {
        wprintf( L"%s%s [%I64d bytes]\n", prefix, f->name, f->size);
    }

    for (m = dir->misc; m != NULL; m = m->sibling) {
        wprintf( L"%s%s [link]\n", prefix, m->name);
    }

    for (d = dir->dirs; d != NULL; d = d->sibling) {
        print_tree(d);
    }

    prefix_len -= 6;
    wcscpy(prefix + prefix_len - 5, L"------");
}

struct Head *
    full_filename (
        struct File *   f
)
{
    struct Head *       list;
    struct Directory *  p;

    list = new_Head( );

    add_list( list, f->name );

    for (p = f->parent; p != NULL; p = p->parent) {
        add_list( list, p->name );
    }

    return list;
}

void
    print_full_filename (
        struct File *   f
)
{
    struct Link *   name;

    for ( name = full_filename( f )->first; name != NULL; name = name->next ) {
        wprintf( L"%s", name->data );

        if (name->next != NULL) {
            wprintf( L"\\" );
        }
    }
}


void
    list_files (
        struct Head *   bucket
)
{
    struct Link *   l;

    long long int   max = -1;
    wchar_t *       max_name = NULL;

    for (l = bucket->first; l != NULL; l = l->next) {
        struct File *   f;

        f = (struct File *)l->data;

        wprintf( L"%20I64d bytes: ", f->size );
        print_full_filename( f );
        wprintf( L"\n" );

        if (f->size > max) {
            max = f->size;
            max_name = f->name;
        }
    }

    wprintf( L"max filesize: %I64d (%s)\n", max, max_name );
}

void
    hash_files (
        struct Head *   bucket
)
{
    struct Link *   l;

    hash_init( all_files.count, 0, 0 );

    for (l = all_files.first; l != NULL; l = l->next) {

        hash_file( ((struct File *)l->data)->size, l->data );
    }
}


