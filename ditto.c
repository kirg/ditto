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

    wprintf(L"\n");
    hash_stats( );
    //find_dittos( );
}


void
    print_String (
        void * data
)
{
    wprintf(L"%s", data);
}

void
    print_StringList (
        void *  data,
        int     max
)
{
    print_list( data, print_String, max );
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
wprintf(L"\r%d dirs, %d files ", count( all_dirs ), count( all_files ) );
//print_StringList( this->path, 256 );
//wprintf(L"     ");

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
    full_filename_list (
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

wchar_t *
    full_filename (
        struct File * file
)
{
    struct List *       list;
    struct Directory *  p;
    wchar_t *           name;
    int                 len;

    list = new_List( );

    push( list, file->name );
    len = wcslen(file->name);

    for (p = file->parent; p != NULL; p = p->parent) {
        push( list, p->name );
        len += wcslen(p->name);
    }

    name = falloc( fa_String, sizeof(wchar_t) * (len + 1) );

    if (name) {
        struct Iter *   iter;
        wchar_t *       component;
        int             end;

        end = 0;

        iter = iterator( list );

        for (component = next( iter ); component != NULL; component = next( iter )) {
            wcsncpy( name+end, component, len-end );
            end += wcslen( component );
        }

        name[end] = 0;

        done( iter );
        
    }

    return name;
}


void
    print_full_filename (
        struct File *   file
)
{
    struct Iter *   iter;
    wchar_t *       name;

    iter = iterator( full_filename_list( file ) );

    for (name = next( iter ); name != NULL; name = next( iter )) {
        wprintf( L"%s", name );
    }

    done( iter );
}

void
    print_File (
        void * data
)
{
    struct File * file = data;
    wprintf( L"%20I64d : ", file->size);
    print_StringList( file->path, 256 );
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

    print_list(copy, print_File, 50);
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


int
    find_checksum (
        void *  buf,
        int     len
)
{
    int i;
    int c0;
    int c1;

    c0 = 1;
    c0 = 0;

    len /= sizeof(int); /* ignore last 'len % sizeof(int)' bytes */

    for (i = 0; i < len; ++i) {
        c0 += ((int *)buf)[i];
        c1 += c0;
    }

    return c1;
}

int
    bufcmp (
        char *  buf,
        char *  buf0,
        int     len
)
{
    int i;

    for (i = 0; (i < len) && (buf[i] == buf0[i]); ++i);

    return i;
}


/*
struct Range {
    long long int   offset;
    long long int   size;
};
*/


/*
struct FileContext {
    HANDLE      hF;
    wchar_t *   name;
    File *      file;
};
*/


struct PreDittoFile {
    struct PreDittoFile *   next;


    int             checksum;
    int             diff_at; /* same checksum, different at bufoffs */

    long long int   size;

    long long int   offs;
    char *          buf;
    int             len;

    struct List *   files;

};

#define START_OFFSET    (256*1024)
#define BUF_SIZE        (256*1024)

#define dbg //wprintf

void
    dittoer (
        long long int   size,
        struct List *   file_list
)
{
    struct Iter *   iter;
    struct File *   file;

    long long int   offs = START_OFFSET;
    LARGE_INTEGER   li_offs;

    struct PreDittoFile *   predit_list;

    if (offs > size) {
        offs = 0;
    }

    predit_list = NULL;



    li_offs.QuadPart = offs;

    iter = iterator( file_list ); 

    for (file = next( iter ); file != NULL; file = next( iter )) {

        char *          buf;
        int             len;

        int             checksum;
        int             diff_at;
        int             diff_at2;

        HANDLE          hF;
        DWORD           bytes_read;
        wchar_t *       name;

        name = full_filename( file ); /* FIXME: free name buffer */

dbg( L"CreateFile( %s )\n", name );

        hF = CreateFile( name, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        buf = malloc( BUF_SIZE );
        len = BUF_SIZE;

        if ((hF != INVALID_HANDLE_VALUE) && (buf != NULL) && 
                SetFilePointerEx( hF, li_offs, NULL, FILE_BEGIN ) &&
                    ReadFile( hF, buf, BUF_SIZE, &bytes_read, NULL )) {

            struct PreDittoFile *   predit;
            struct PreDittoFile *   prev;


dbg( L"seek to offset: %I64d, read %d bytes (requested %d bytes)\n", offs, bytes_read, BUF_SIZE );

            len = bytes_read;

            checksum = find_checksum( buf, len );

dbg( L"checksum(buf=%p, len=%d) = %d\n", buf, len, checksum );

            for (prev = NULL, predit = predit_list;
                    (predit != NULL) && (checksum > predit->checksum);
                        prev = predit, predit = predit->next);

            if ((predit != NULL) && (checksum == predit->checksum)) {

dbg( L"matching checksum found; comparing buffers\n" );

                diff_at = bufcmp( buf, predit->buf, len );

dbg( L"diff_at=%d\n", diff_at );

                if (diff_at == len) {

dbg( L"complete match\n" );
                    /* buffers match byte to byte */
                    queue( predit->files, file );
                    free(buf); /* don't need the buffer any more */ /* FIXME: optimize to just re-use for next file */

                } else {

wprintf( L"mismatched at diff_at=%d\n", diff_at );

                    diff_at2 = 0;

                    for (prev = predit, predit = predit->next;
                            predit != NULL && (checksum == predit->checksum) && (diff_at <= predit->diff_at);
                                prev = predit, predit = predit->next) {

dbg(L"predit=%p, predit->checksum=%d, predit->diff_at=%d\n", predit, predit->checksum, predit->diff_at);

                        if (diff_at == predit->diff_at) {

dbg( L"matching checksum and diff_at; comparing buffers\n");

                            diff_at2 = bufcmp( buf+diff_at, predit->buf+diff_at, len-diff_at );

dbg( L"diff_at2=%d\n", diff_at2);

                            if (diff_at2 == len) {
dbg( L"diff_at2 matched!\n", diff_at2);
                                queue( predit->files, file );
                                free(buf); /* FIXME: optimize */
                                break;
                            }
                        }
dbg( L"next predit\n");

                    }

                    if (diff_at2 != len) {

dbg( L"no matching checksum+diff_at+diff_at2\n");

                        predit = malloc( sizeof(struct PreDittoFile) );

                        if (predit == NULL) {
                            wprintf(L"malloc predit failed!\n");
                            return;
                        }

                        predit->checksum    = checksum;
                        predit->diff_at     = diff_at;

                        predit->size        = size;
                        predit->offs        = offs;

                        predit->buf         = buf;
                        predit->len         = len; /* == BUF_SIZE */

                        predit->files       = new_List( );

                        if (predit->files == NULL) {
                            wprintf(L"predit->files new_List failed!\n");
                            return;
                        }

                        queue( predit->files, file );

                        if (prev != NULL) {
                            predit->next    = prev->next;
                            prev->next      = predit;
                        } else {
                            predit->next    = NULL;
                            predit_list     = predit;
                        }
                    }

                }

            } else {

                /* new checksum -> add preDit */

dbg( L"new checksum; adding predit\n");

                predit = malloc( sizeof(struct PreDittoFile) );

                if (predit == NULL) {
                    wprintf(L"malloc predit failed!\n");
                    return;
                }

                predit->checksum    = checksum;
                predit->diff_at     = len;

                predit->size        = size;
                predit->offs        = offs;

                predit->buf         = buf;
                predit->len         = len; // == BUF_SIZE */

                predit->files       = new_List( );

                if (predit->files == NULL) {
                    wprintf(L"predit->files new_List failed!\n");
                    return;
                }

                queue( predit->files, file );

                if (prev != NULL) {
                    predit->next    = prev->next;
                    prev->next      = predit;
                } else {
                    predit->next    = NULL;
                    predit_list     = predit;
                }

                /* allocate new buf */

            }

        } else {
            wprintf( L"error opening/reading file (or allocating buffer)! (file=%s)\n", name);
        }

//dbg(L"enter for next:");getchar();
    }

    done( iter );

dbg(L"done\n");
    /* at this stage we have a list of buckets, with each bucket containing:

        0. predit->files->count == 1    -> no duplicates; can be removed/ignored from further evaluation
        1. len==BUF_SIZE    -> list of files that match byte to byte for the given offs
        2. len < BUF_SIZE   ->
    */

//wprintf(L"list pre-ditto list of files\n");
    {
        struct PreDittoFile *   predit;

        for (predit = predit_list; predit != NULL; predit = predit->next) {

            if (predit->files->count > 1) {
                wprintf(L"almost ditto files:\n");
                print_list(predit->files, print_File, 50);
                wprintf(L"\n");
            } else {
//wprintf(L"IGNORE pre-ditto bucket ( %s )\n", ((struct File *)predit->files->head)->name);
            }

            free(predit->buf);
        }

    }
}

