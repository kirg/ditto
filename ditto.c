#include "ditto.h"
#include "ditto_.h"

#include "list.h"
#include "hashmap.h"

#include <stdio.h>
#include <windows.h>
#include <time.h>

// #include <windef.h>
// #include <winnt.h>
// #include <ntdll.h>
// #include <ntdef.h>

#ifdef __GNUC__  
#   include <ddk/ntifs.h>
#else
#   include <ntifs.h>
#endif


#define MAX_PATHNAME_LEN (32768)

#define FILE_DIRECTORY_INFORMATION_BUFSIZE (65536)



/* scratch variables */
FILE_DIRECTORY_INFORMATION * fdi_buf;

FastAlloc fa_String;
FastAlloc fa_File;
FastAlloc fa_Directory;

struct List * scan_paths;

struct List * all_files;
struct List * all_dirs;
struct List * all_misc;


/* passed into build_tree implicitly; globals to reduce stack context */
wchar_t     build_tree_path[MAX_PATHNAME_LEN];
int         build_tree_path_len;


void
    ditto_init (
        void
)
{
    falloc_init( );
    list_init( );
    hashmap_init( );

    fa_String       = new_falloc( L"String", 0 );
    fa_File         = new_falloc( L"File", sizeof(struct File) );
    fa_Directory    = new_falloc( L"Directory", sizeof(struct Directory) );

    scan_paths      = new_List( );

    all_files       = new_List( );
    all_dirs        = new_List( );
    all_misc        = new_List( );

}

void
    ditto_cleanup (
        void
)
{
    delete_List( all_misc );
    delete_List( all_dirs );
    delete_List( all_files );

    delete_List( scan_paths );

    delete_falloc( fa_Directory );
    delete_falloc( fa_File );
    delete_falloc( fa_String );

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

    path    = falloc( fa_String,    sizeof(wchar_t) * (wcslen(pathW) + 2) );
    dir     = falloc( fa_Directory, sizeof(struct Directory) );

    if (path && dir) {
        int i;

        for (i = 0; pathW[i] != 0; ++i) {
            path[i] = pathW[i];
        }

        /* add trailing '\' */
        if (path[i-1] != L'\\') {
            path[i] = L'\\';
            ++i;
        }

        path[i] = 0;

        dir->name       = path;
        dir->parent     = NULL;
        dir->sibling    = NULL;

        dir->path       = new_List( );
        queue( dir->path, dir->name);

        dir->n_files    = 0;
        dir->files      = NULL;

        dir->n_dirs     = 0;
        dir->dirs       = NULL;

        dir->n_misc     = 0;
        dir->misc       = NULL;

        queue( scan_paths, dir );

wprintf(L"include_dir: %s\n", dir->name);

    } else {
        wprintf(L"falloc path/dir failed\n");
    }
}

void
    include_dirA (
        char *      pathA
)
{
    wchar_t *           path;
    struct Directory *  dir;

    path    = falloc( fa_String,    sizeof(wchar_t) * (strlen(pathA) + 2) );
    dir     = falloc( fa_Directory, sizeof(struct Directory) );

    if (path && dir) {
        int i;

        for (i = 0; pathA[i] != 0; ++i) {
            path[i] = pathA[i];
        }

        /* check and remove trailing '\' */
        if (path[i-1] != L'\\') {
            path[i] = L'\\';
            ++i;
        }

        path[i] = 0;

        dir->name       = path;
        dir->parent     = NULL;
        dir->sibling    = NULL;

        dir->path       = new_List( );
        queue( dir->path, dir->name);

        dir->n_files    = 0;
        dir->files      = NULL;

        dir->n_dirs     = 0;
        dir->dirs       = NULL;

        dir->n_misc     = 0;
        dir->misc       = NULL;

        queue( scan_paths, dir );

wprintf(L"include_dirA: %s\n", dir->name);

    } else {
        wprintf(L"malloc path/dir failed\n");
    }
}


void
    scan (
        void
)
{
    struct Iter *       iter;
    struct Directory *  dir;
int t0; /* clock */

    fdi_buf = malloc( FILE_DIRECTORY_INFORMATION_BUFSIZE );

    if (fdi_buf == NULL) {
        wprintf(L"malloc fdi_buf failed\n");
        return;
    }

    iter = iterator( scan_paths );

    for (dir = next( iter ); dir != NULL; dir = next( iter )) {

wprintf(L"scanning %s:\n", dir->name);
t0=clock();

        wcsncpy( build_tree_path, dir->name, MAX_PATHNAME_LEN );
        build_tree_path_len = wcslen( build_tree_path );

        build_tree( dir );
wprintf(L"\rdone (%d ticks)                                                    \n", clock()-t0);
    }

    done( iter );

wprintf(L"scan complete: %d dirs, %d files, %d links\n", count( all_dirs ), count( all_files ), count( all_misc ));
//wprintf(L"enter to continue .."); getchar();


    iter = iterator( scan_paths );

    for (dir = next( iter ); dir != NULL; dir = next( iter )) {

        //print_tree( dir );

    }

    done( iter );


wprintf(L"enter to sort files by size .."); getchar();
wprintf(L"files: %d files\n", count( all_files ));

    list_files( all_files );


wprintf(L"enter to hash files .."); getchar();
    hash_init( count( all_files ), 0, 0 );

wprintf(L"hashing files: ");
t0=clock();
    hash_files( all_files );
wprintf(L"done (%d ticks)\n", clock()-t0);

    //find_dittos( );
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

    //struct List *   path;

//wprintf( L"traverse: %s (%d)\n", build_tree_path, build_tree_path_len );
wprintf(L"\r%d dirs, %d files (%s)                                  ", count( all_dirs ), count( all_files ), this->name);

    this->n_files   = 0;
    this->files     = NULL;

    this->n_dirs    = 0;
    this->dirs      = NULL;

    this->n_misc    = 0;
    this->misc      = NULL;


    /* using pre-setup 'build_tree_path' and 'build_tree_path_len' globals */

    hD = CreateFile( build_tree_path, FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS |
                        FILE_FLAG_OPEN_REPARSE_POINT | FILE_CREATE_TREE_CONNECTION, NULL );

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

                if (fdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

                    struct Directory *  dir;

                    name = falloc( fa_String, sizeof(wchar_t) * (len + 2) );

                    if (name == NULL) {

                        if (fdi->NextEntryOffset == 0) {
                            break;
                        }

                        continue; // FIXME: break?
                    }

                    wcsncpy( name, fdi->FileName, len );

                    name[ len ]     = L'\\'; /* add trailing '\' to directories */
                    name[ len + 1 ] = 0;

                    dir = falloc( fa_Directory, sizeof(struct Directory) );

                    if (dir != NULL) {

                        dir->name       = name;
                        dir->path       = clone( this->path );
                        queue( dir->path, dir->name );

                        dir->parent     = this;

                        /* add to child-dir list */
                        dir->sibling    = this->dirs;
                        this->dirs      = dir;
                        ++this->n_dirs;

                        /* add to all_dirs bucket */
                        queue( all_dirs, dir );

                    } else {
                        wprintf( L"malloc( sizeof(struct Directory) ) failed\n");
                        return -1;
                    }
                    
                } else if (fdi->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {

                    struct Misc *   misc;

                    name = falloc( fa_String, sizeof(wchar_t) * (len + 1) );

                    if (name == NULL) {

                        if (fdi->NextEntryOffset == 0) {
                            break;
                        }

                        continue; // FIXME: break?
                    }

                    wcsncpy( name, fdi->FileName, len );

                    name[ len ] = 0;

                    misc = malloc( sizeof(struct Misc) );


                    if (misc != NULL) {

                        misc->name      = name;
                        misc->path      = clone( this->path );
                        queue( misc->path, misc->name );

                        misc->parent    = this;

                        /* add to child-misc list */
                        misc->sibling   = this->misc;
                        this->misc      = misc;
                        ++this->n_misc;

                        /* add to all_misc bucket */
                        queue( all_misc, misc );

                    } else {
                        wprintf( L"malloc( sizeof(struct Node) ) failed\n");
                        return -2;
                    }


                } else {

                    struct File *   file;

                    name = falloc( fa_String, sizeof(wchar_t) * (len + 1) );

                    if (name == NULL) {

                        if (fdi->NextEntryOffset == 0) {
                            break;
                        }

                        continue; // FIXME: break?
                    }

                    wcsncpy( name, fdi->FileName, len );

                    name[ len ] = 0;

                    file = falloc( fa_File, sizeof(struct File) );

                    if (file != NULL) {

                        file->name      = name;
                        file->path      = clone( this->path );
                        queue( file->path, file->name );

                        file->parent    = this;

                        /* add to child-files list */
                        file->sibling   = this->files;
                        this->files     = file;
                        ++this->n_files;

                        /* add to all_files bucket */
                        queue( all_files, file );

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
        //FIXME: add to error list //
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

    if (dir == NULL) {

        struct Iter *   iter;
        struct Node *   node;

        iter = iterator( scan_paths );

        for ( node = next( iter ); node != NULL; node = next( iter )) {

            struct Directory * dir;

            dir = node->data;

            print_tree( dir );

        }

        done( iter );

        return;
    }

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

struct List *
    full_filename (
        struct File *   file
)
{
    struct List *       list;
    struct Directory *  p;

    list = new_List( );

    push( list, file->name );

    for (p = file->parent; p != NULL; p = p->parent) {
        push( list, p->name );
    }

    return list;
}

void
    print_full_filename (
        struct File *   file
)
{
    struct Iter *   iter;
    wchar_t *       name;

    iter = iterator( full_filename( file ) );

    for (name = next( iter ); name != NULL; name = next( iter )) {
        wprintf( L"%s", name );
    }

    done( iter );
}

void
    print_String (
        void * data
)
{
    wprintf(L"%s", data);
}

void
    print_File (
        void * data
)
{
    struct File * file = data;
    wprintf( L"%20I64d : ", file->size);
    print_list( file->path, print_String, 100 );
    wprintf( L"\n" );
}

int
    compare_File_by_size (
        struct File *   left,
        struct File *   right
)
{
    return (left->size < right->size) ? -1 : (left->size == right->size) ? 0 : 1;
}

void
    sort_files_by_size (
        struct List *   bucket
)
{
    int t0;
    struct List *   copy;


    if (bucket == NULL) {
        bucket = all_files;
    }

    wprintf(L"clone and sort: ");
    t0=clock();

    copy = clone( bucket );

    merge_sort(copy, (compare_func) compare_File_by_size);

    wprintf(L"done (%d ticks)\n", clock()-t0);
    delete_List(copy);

    print_list(copy, print_File, 1000);
}

void
    list_files (
        struct List *   bucket
)
{
    //struct Iter *   iter;
    //struct File *   f;

    // long long int   max = -1;
    // wchar_t *       max_name = NULL;

    if (bucket == NULL) {
        bucket = all_files;
    }

    sort_files_by_size( bucket );

/*
    iter = iterator( bucket );

    for (f = next( iter ); f != NULL; f = next( iter )) {

        wprintf( L"%20I64d bytes: ", f->size );
        print_full_filename( f );
        wprintf( L"\n" );

        if (f->size > max) {
            max = f->size;
            max_name = f->name;
        }
    }

    done( iter );

    wprintf( L"max filesize: %I64d (%s)\n", max, max_name );
*/

}

void
    hash_files (
        struct List *   bucket
)
{
    struct Iter *   iter;
    struct File *   f;

//  hash_init( count( all_files ), 0, 0 );

    iter = iterator( bucket );
    
    for (f = next( iter ); f != NULL; f = next( iter )) {

        hash_file( f->size, f );
    }

    done( iter );
}


#if 0
#define START_OFFSET    (256*1024)

void
    dittoer (
        long long int   size,
        struct List *   file_list
)
{
    struct FileContext {
        HANDLE  hF;
        char *  buf;
        int     len;

        File *  file;
    };

    struct Node * node;

    int num = file_list->count;

    struct FileContext *    fc = malloc( num * sizeof(struct FileContext) );

    long long int   size = fzbucket->size;
    long long int   offs = START_OFFSET;

    if (offs > size) {
        offs = 0;
    }

    for (i = 0; i < num; ++i) {
        files[i].hF     = CreateFile(..);
        files[i].len    = BUF_SIZE;
        files[i].buf    = falloc( BUF_SIZE );

        SetFilePointerEx( offs );
        ReadFile( hF, BUF_SIZE );
    }


    for (node = fzbucket->files.first; node != NULL; node = node->next) {

        struct File *   file;

        file = (struct File *)node->data;

        wprintf( L"  "); print_full_filename( file ); wprintf( L"\n" );


    }

}
#endif

