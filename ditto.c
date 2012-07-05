#include "ditto.h"
#include "ditto_.h"

#include "list.h"

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

FastAlloc fa_FilesizeBucket;



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
    void
        hashmap_init (
            void
    );

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
    void
        hashmap_cleanup (
            void
    );

    delete_List( all_misc );
    delete_List( all_dirs );
    delete_List( all_files );

    /* FIXME: free(scan_dir); */

    delete_List( scan_paths );

    delete_falloc( fa_Directory );
    delete_falloc( fa_File );
    delete_falloc( fa_String );

    hashmap_cleanup( );
    list_cleanup( );
    falloc_cleanup( );
}

#define MAX_PATH_LEN    (32767)

void
    include_dir (
#ifdef __GNUC__  
        char *      argA
#else
        wchar_t *   arg
#endif
)
{
    wchar_t *   buf;
    int         len;

    wchar_t *   prepend;
    wchar_t *   path;
    int         i;


    struct ScanDirectory *  scan_dir;


#ifdef __GNUC__  

    wchar_t *   arg;


    len = strlen(argA);

    arg = malloc( sizeof(wchar_t) * (len + 1) );

    if (arg == NULL) {
        return;
    }

    for (i = 0; i < len; ++i) {
        arg[i] = argA[i];
    }

    arg[len] = 0;

#endif


    buf = malloc( sizeof(wchar_t) * (MAX_PATH_LEN + 1) );

    if (buf == NULL) {
        goto exit;
    }

    len = GetFullPathName( arg, sizeof(wchar_t) * (MAX_PATH_LEN + 1), buf, NULL );

    if (len == 0 || len > MAX_PATH_LEN) {
        wprintf( L"error allocating buffer for path-name (gle=%d)\n", GetLastError( ) );
        goto exit;
    }


    if (len > 2 && buf[0] == L'\\' && buf[1] == L'\\') {

        if (buf[2] != L'\?') {
            prepend = L"\\\\\?\\UNC"; /* \\?\UNC + \\ */
        } else {
            prepend = L"";
        }

    } else {
        prepend = L"\\\\\?";
    }

    /* add trailing '\', if necessary */
    if (buf[len-1] != L'\\') {
        buf[len] = L'\\';
        ++len;
    }
    
    scan_dir    = malloc( sizeof(struct ScanDirectory) );
    path        = falloc( fa_String, sizeof(wchar_t) * (len + 1) );

    if (scan_dir && path) {

        for (i = 0; i < len; ++i) {
            path[i] = buf[i];
        }

        path[len] = 0;

        scan_dir->dir.name      = path;
        scan_dir->dir.parent    = NULL;
        scan_dir->dir.sibling   = NULL;

        scan_dir->dir.path      = new_List( );
        queue( scan_dir->dir.path, scan_dir->dir.name);

        scan_dir->dir.n_files   = 0;
        scan_dir->dir.files     = NULL;

        scan_dir->dir.n_dirs    = 0;
        scan_dir->dir.dirs      = NULL;

        scan_dir->dir.n_misc    = 0;
        scan_dir->dir.misc      = NULL;

        scan_dir->prepend           = prepend;

        queue( scan_paths, scan_dir );

wprintf(L"include_dir: %s\n", scan_dir->dir.name);

    } else {
        wprintf(L"falloc path/dir failed\n");
    }


exit:
    if (buf != NULL) {
        free( buf );
    }

#ifdef __GNUC__  
    if (arg != NULL) {
        free( arg );
    }
#endif

}


void
    scan (
        void
)
{
    struct Iter *           iter;
    struct ScanDirectory *  scan_dir;
int t0; /* clock */

    fdi_buf = malloc( FILE_DIRECTORY_INFORMATION_BUFSIZE );

    if (fdi_buf == NULL) {
        wprintf(L"malloc fdi_buf failed\n");
        return;
    }

    iter = iterator( scan_paths );

    for (scan_dir = next( iter ); scan_dir != NULL; scan_dir = next( iter )) {

wprintf(L"scanning %s:\n", scan_dir->dir.name);
t0=clock();

        wcsncpy( build_tree_path, scan_dir->dir.name, MAX_PATHNAME_LEN );
        build_tree_path_len = wcslen( build_tree_path );

        build_tree( &scan_dir->dir );
wprintf(L"\rdone (%d ticks)                                                    \n", clock()-t0);
    }

    done( iter );

wprintf(L"scan complete: %d dirs, %d files, %d links\n", count( all_dirs ), count( all_files ), count( all_misc ));
//wprintf(L"enter to continue .."); getchar();


    iter = iterator( scan_paths );

    for (scan_dir = next( iter ); scan_dir != NULL; scan_dir = next( iter )) {

        //print_tree( &scan_dir->dir );

    }

    done( iter );

// wprintf(L"files: %d files\n", count( all_files ));
// list_files( all_files );
// wprintf(L"enter to sort files by size .."); getchar();


wprintf(L"enter to find ditto files .."); getchar();
    file_dittoer( );
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

                        file->context = NULL;

                        hash_file( file );

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
    fullpathname (
        struct File *   file,
        wchar_t *       buf,
        //int             len,
        int             prepend /* include */
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
    print_StringList( file->path, 256 );
    wprintf( L" (%I64d)\n", file->size);
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




#define HASH_BUCKETS_NUM    (65536)

struct List *   hash_buckets;   /* array of <list of FilesizeBuckets> */


void
    hashmap_init (
        void
)
{
    hash_buckets = valloc( HASH_BUCKETS_NUM * sizeof(struct List) );

    fa_FilesizeBucket = new_falloc( L"FilesizeBucket", sizeof(struct FilesizeBucket) );

    if ( hash_buckets && fa_FilesizeBucket ) {

        /* VirtualAlloc returns a zeroed buffer */

        /*
        int i;

        for ( i = 0; i < HASH_BUCKETS_NUM; ++i) {
            hash_buckets[ i ].head  = NULL;
            hash_buckets[ i ].tail  = NULL;
            hash_buckets[ i ].count = 0;
        }
        */

    } else {
        wprintf( L"valloc hash_buckets or new_falloc(fa_FilesizeBucket) failed\n" );
    }

}

void
    hashmap_cleanup (
        void
)
{
    delete_falloc( fa_FilesizeBucket );
    vfree( hash_buckets );
}


void
    hash_file (
        struct File *   file
)
{
    long long int           size;
    struct Iter *           iter;
    struct List *           hash_bucket;
    struct FilesizeBucket * fzbucket;


    size = file->size;

    hash_bucket = &hash_buckets[ size % HASH_BUCKETS_NUM ];

    iter = iterator( hash_bucket );

    /* FIXME: use a sorted list */

    for ( fzbucket = next( iter );
            (fzbucket != NULL) && (fzbucket->size != size);
                fzbucket = next( iter) );

    done( iter );

    if (fzbucket == NULL) {

        fzbucket = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );

        if (fzbucket == NULL) {
            wprintf( L"falloc FilesizeBucket failed\n");
            return;
        }

        fzbucket->offs  = 0;
        fzbucket->size  = size;
        fzbucket->files = new_List( );

        queue( hash_bucket, fzbucket );
    }

    queue( fzbucket->files, file );


#if 0
{
    void print_full_filename( struct File * file );

    wprintf( L"%10I64d => ", size );
    print_full_filename( file );
    wprintf( L"\n" );
}
#endif

}



int
    bufsum (
        void *  buf,
        int     len
)
{
    int i;
    int c0;
    int c1;

    c0 = 1;
    c1 = 0;

    len /= sizeof(int); /* ignore last 'len % sizeof(int)' bytes */

    for (i = 0; i < len; ++i) {
        c0 += ((int *)buf)[i];
        c1 += c0;
    }

    return c1; /* return checksum */
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


#define START_OFFSET    (0)
#define BUF_SIZE        (256*1024)

#define ERROR_RETRIES   (3)


FastAlloc fa_PreDittoContext;
FastAlloc fa_Buffer;

struct DittoFiles {
/*  long long int   size; */
    struct List *   files_list;  /* list of files that are _ditto_ */
} ditto_files_list; /* FIXME: alloc */

long long int   total_ditto_size;

//struct List *   unique_files;


#define BUF_SIZE_ALIGN  (4096)

#define MIN_SIZE        (0)

void
    file_dittoer (
        void
)
{
    int i;
    int t0;
    int errors;

    ditto_files_list.files_list = new_List( );

    fa_PreDittoContext  = new_falloc( L"PreDittoContext", sizeof(struct PreDittoContext) );
    fa_Buffer           = new_falloc( L"Buffer", BUF_SIZE );

    for (i = 0; i < HASH_BUCKETS_NUM; ++i) {

        struct FilesizeBucket * fzbucket;

        errors = 0;

        while ((fzbucket = pop( &hash_buckets[ i ] )) != NULL) {

            struct Iter *   iter;
            struct File *   file;

            long long int   size;
            long long int   offs;
            struct List *   files;

            LARGE_INTEGER   li_offs;

            int buf_size; /* to checksum/compare */

            struct PreDittoContext *   preDit_list;
            struct PreDittoContext *   preDit;
            struct PreDittoContext *   preDit_prev;

t0 = clock( );

            offs    = fzbucket->offs;
            size    = fzbucket->size;
            files   = fzbucket->files;

wprintf(L"\rdittoing bucket #%d [size=%I64d, count=%d] offs=%I64d          ", i, size, files->count, offs);

            if (files->count == 1) {
                break; /* skip buckets with one file */
            }

            if (size < MIN_SIZE) {
                break; /* skip buckets with files less than MIN_SIZE */
            }

            preDit_list         = NULL;

            buf_size            = size - ((size + BUF_SIZE_ALIGN) / BUF_SIZE_ALIGN);
            buf_size            = BUF_SIZE; /* FIXME: say if (files->count == 2) use a larger buf-size? */
            li_offs.QuadPart    = offs;

            iter = iterator( files ); 

            for (file = next( iter ); file != NULL; file = next( iter )) {

                HANDLE  hF;
                char *  buf;
                int     len;

                int     checksum;
                int     diff_at;
                int     diff_at2;

                DWORD   bytes_read;

                hF = (HANDLE)file->context;

                if (hF == NULL || hF == INVALID_HANDLE_VALUE) {

                    wchar_t *   path = full_filename( file ); /* FIXME: free path buffer */

                    /* FIXME: use read-through/no-buffering?! */
                    hF = CreateFile( path, GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

                    file->context = (void *)hF;

                }

                buf = falloc( fa_Buffer, buf_size );
                len = buf_size;

                if ((hF != INVALID_HANDLE_VALUE) && (buf != NULL) && 
                        SetFilePointerEx( hF, li_offs, NULL, FILE_BEGIN ) &&
                            ReadFile( hF, buf, buf_size, &bytes_read, NULL )) {

                    len = bytes_read; /* FIXME: take care of len < buf_size in ditto-ing */

                    checksum = bufsum( buf, len );

                    for (preDit_prev = NULL, preDit = preDit_list;
                            (preDit != NULL) && (checksum > preDit->checksum);
                                preDit_prev = preDit, preDit = preDit->next);

                    if ((preDit != NULL) && (checksum == preDit->checksum)) {

                        diff_at = bufcmp( buf, preDit->buf, len );

                        if (diff_at == len) {

                            queue( preDit->files, file );

                            ffree( fa_Buffer, buf ); /* don't need the buffer any more */ /* FIXME: optimize to just re-use for next file */

                        } else {

                            /* this is for the uncommon case where the checksum is the same, but the buffers differ */

if (size > sizeof(int)) { wprintf( L"######### for checksum=%d, buf mismatched at offs %d (size=%I64d) ##############\n", checksum, diff_at, size ); }

                            diff_at2 = 0;

                            for (preDit_prev = preDit, preDit = preDit->next;
                                    preDit != NULL && (checksum == preDit->checksum) && (diff_at <= preDit->diff_at);
                                        preDit_prev = preDit, preDit = preDit->next) {

                                if (diff_at == preDit->diff_at) {

                                    diff_at2 = bufcmp( buf+diff_at, preDit->buf+diff_at, len-diff_at );

                                    if (diff_at2 == len) {
                                        queue( preDit->files, file );
                                        ffree( fa_Buffer, buf ); /* FIXME: optimize */
                                        break;
                                    }
                                }

                            }

                            if (diff_at2 != len) {

                                preDit = falloc( fa_PreDittoContext, sizeof(struct PreDittoContext) );

                                if (preDit == NULL) {
                                    wprintf(L"falloc preDit failed!\n");
                                    return;
                                }

                                preDit->checksum    = checksum;
                                preDit->diff_at     = diff_at;

                                // preDit->size        = max_size;
                                // preDit->offs        = offs;

                                preDit->buf         = buf;
                                preDit->len         = len; /* == buf_size */

                                preDit->files       = new_List( );

                                if (preDit->files == NULL) {
                                    wprintf(L"preDit->files new_List failed!\n");
                                    return;
                                }

                                queue( preDit->files, file );

                                if (preDit_prev != NULL) {
                                    preDit->next        = preDit_prev->next;
                                    preDit_prev->next   = preDit;
                                } else {
                                    preDit->next    = preDit_list;
                                    preDit_list     = preDit;
                                }

                            }

                        }

                    } else {

                        /* new checksum -> new preDit */

                        preDit = falloc( fa_PreDittoContext, sizeof(struct PreDittoContext) );

                        if (preDit == NULL) {
                            wprintf(L"falloc preDit failed!\n");
                            return;
                        }

                        preDit->checksum    = checksum;
                        preDit->diff_at     = len;

                        //preDit->size        = max_size;
                        //preDit->offs        = offs;

                        preDit->buf         = buf;
                        preDit->len         = len; // == buf_size */

                        preDit->files       = new_List( );

                        if (preDit->files == NULL) {
                            wprintf(L"preDit->files new_List failed!\n");
                            return;
                        }

                        queue( preDit->files, file );

                        if (preDit_prev != NULL) {
                            preDit->next        = preDit_prev->next;
                            preDit_prev->next   = preDit;
                        } else {
                            preDit->next    = preDit_list;
                            preDit_list     = preDit;
                        }

                        /* allocate new buf */

                    }

                } else {

                    wchar_t *   path = full_filename( file ); /* FIXME: free path buffer */

                    wprintf( L"error opening/reading file (or allocating buffer)! (file=%s)\n", path);

                    ++errors;

                    ffree( fa_Buffer, buf );

                    break;

                    if (errors < ERROR_RETRIES) {

                        wprintf(L"will retry later\n");
                        queue( &hash_buckets[ i ], fzbucket ); /* push to end so it is retried later */
                        continue; /* FIXME: ignore and continue?!! */

                    } else {

                        delete_List( files );
                        ffree( fa_FilesizeBucket, fzbucket );

                        wprintf(L"too many errors; giving up\n");
                        break;
                    }
                }

            }

            done( iter );

            if (errors > 0) {
            //if (errors == ERROR_RETRIES) {
                wprintf(L"skipping bucket\n");
                continue;
            }

            /* finished processing files in this fzbucket; we don't need the fzbucket any more */

            delete_List( files );
            ffree( fa_FilesizeBucket, fzbucket );


            {
                void *  free_preDit;

                preDit = preDit_list;

                while (preDit != NULL) {

                    if (preDit->files->count == 1) {

                        struct File * f;

                        f = (struct File *)preDit->files->head->data;

                        CloseHandle( f->context );
                        f->context = NULL;

// wprintf(L"unique file: "); print_File( f );

                        delete_List( preDit->files );

                    } else {

                        long long int   new_offs;

                        new_offs = offs + buf_size;

                        if (new_offs >= size) {

                            /* we have compared all of the file, so these files match byte-to-byte */

                            /* close handles */
                            iter = iterator( preDit->files ); 

                            for (file = next( iter ); file != NULL; file = next( iter )) {
                                CloseHandle( file->context );
                                file->context = NULL;
                            }

                            done( iter );

                            push( ditto_files_list.files_list, preDit->files );

                            total_ditto_size += size * (preDit->files->count - 1); // redundant bytes

wprintf(L"\rditto #%d [size=%I64d, ditto=%I64d, ticks=%d]:                     \n",
            ditto_files_list.files_list->count, size, total_ditto_size, clock()-t0);
print_list(preDit->files, print_File, 50);
wprintf(L"--\n");

                        } else {

                            /* push a new FilesizeBucket to compare next set of bytes */

                            struct FilesizeBucket * fzbucket_new;

                            fzbucket_new = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) ); /* FIXME: check for failure */

                            fzbucket_new->offs   = new_offs;
                            fzbucket_new->size   = size;
                            fzbucket_new->files  = preDit->files;

                            push( &hash_buckets[ i ], fzbucket_new );
                        }
                    }

                    ffree( fa_Buffer, preDit->buf );
                    free_preDit = preDit;

                    preDit = preDit->next;

                    ffree( fa_PreDittoContext, free_preDit );
                }

//falloc_stat( fa_Buffer );
            }

        }

//wprintf( L"\ndone processing hash_buckets[ %d ]\n", i );

    }
}


#define dbg //wprintf


#if 0
void
    dittoer (
        long long int   max_size,
        struct List *   file_list
)
{
    struct Iter *   iter;
    struct File *   file;

    long long int   offs = START_OFFSET;
    LARGE_INTEGER   li_offs;

    struct PreDittoFile *   predit_list;

    if (offs > max_size) {
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

                        predit->size        = max_size;
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

#endif
