#include "ditto.h"
#include "ditto_.h"

#include "list.h"

#include <stdio.h>      /* wprintf, getchar */
#include <windows.h>    /* CreateFile, CloseHandle, GetFullPathName */

#include <time.h>       /* clock */

//#include <ntdef.h>

#ifdef __GNUC__  
#   include <ddk/ntifs.h>   /* NtQueryDirectoryFile, etc */
#else

typedef enum BOOLEAN {
    BOOLEAN_TRUE    = 1,
    BOOLEAN_FALSE   = 0
};

typedef enum NTSTATUS {
    STATUS_SUCCESS = 0
};

typedef enum _FILE_INFORMATION_CLASS { 
  FileDirectoryInformation      = 1,
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_DIRECTORY_INFORMATION {
  ULONG         NextEntryOffset;
  ULONG         FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG         FileAttributes;
  ULONG         FileNameLength;
  WCHAR         FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;


NTSTATUS NtQueryDirectoryFile(
  HANDLE FileHandle,
  HANDLE Event,
  PVOID ApcRoutine,         /* PIO_APC_ROUTINE ApcRoutine, */
  PVOID ApcContext,
  PVOID IoStatusBlock,      /* PIO_STATUS_BLOCK IoStatusBlock, */
  PVOID FileInformation,
  ULONG Length,
  FILE_INFORMATION_CLASS FileInformationClass,
  BOOLEAN ReturnSingleEntry,
  PVOID FileName,            /* PUNICODE_STRING FileName, */
  BOOLEAN RestartScan
);


#endif

/*
#include <windef.h>
#include <winnt.h>
#include <ntdll.h>
#include <ntdef.h>
*/

#define MAX_PATH_BUFLEN                     (32768)

#define FILE_DIRECTORY_INFORMATION_BUFSIZE  (65536) /* for use with NtQueryDirectoryFile */



/* scratch variables */
FILE_DIRECTORY_INFORMATION * fdi_buf;   /* for use with NtQueryDirectoryFile */

FastAlloc fa_String;
FastAlloc fa_File;
FastAlloc fa_Directory;

FastAlloc fa_FilesizeBucket;



struct List * scan_paths;

struct List * all_files;
struct List * all_dirs;
struct List * all_misc;


/* passed into build_tree implicitly; globals to reduce stack context */
wchar_t     build_tree_path[ MAX_PATH_BUFLEN ];
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


    buf = malloc( sizeof(wchar_t) * MAX_PATH_BUFLEN );

    if (buf == NULL) {
        goto exit;
    }

    len = GetFullPathName( arg, sizeof(wchar_t) * MAX_PATH_BUFLEN, buf, NULL );

    if (len == 0 || len > (MAX_PATH_BUFLEN - 1)) {
        wprintf( L"error allocating buffer for path-name (gle=%d)\n", GetLastError( ) );
        goto exit;
    }


    if (len > 2 && buf[0] == L'\\' && buf[1] == L'\\') {

        if (buf[2] != L'\?') {
            prepend = L"\\\\\?\\UNC"; /* \\?\UNC + \\ */
        } else {
            prepend = L""; /* contains \\?\ prefix */
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
        enqueue( scan_dir->dir.path, scan_dir->dir.name);

        scan_dir->dir.n_files   = 0;
        scan_dir->dir.files     = NULL;

        scan_dir->dir.n_dirs    = 0;
        scan_dir->dir.dirs      = NULL;

        scan_dir->dir.n_misc    = 0;
        scan_dir->dir.misc      = NULL;

        scan_dir->prepend           = prepend;

        enqueue( scan_paths, scan_dir );

#ifdef verbose
        wprintf(L"include_dir: %s\n", scan_dir->dir.name);
#endif

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

wprintf(L"scanning \"%s\"\n", scan_dir->dir.name);
t0=clock();

        wcsncpy( build_tree_path, scan_dir->dir.name, MAX_PATH_BUFLEN );
        build_tree_path_len = wcslen( build_tree_path );

        build_tree( &scan_dir->dir );

wprintf(L"\rdone (%d ticks)                                                    \n", clock()-t0);
    }

    done( iter );

wprintf(L"scan complete: %d dirs, %d files, %d links\n", count( all_dirs ), count( all_files ), count( all_misc ));

    iter = iterator( scan_paths );

    for (scan_dir = next( iter ); scan_dir != NULL; scan_dir = next( iter )) {

        // print_tree( &scan_dir->dir );

    }

    done( iter );

// wprintf(L"files: %d files\n", count( all_files ));
// list_files( all_files );
// wprintf(L"enter to sort files by size .."); getchar();

//wprintf(L"enter to find ditto files .."); getchar();
    ditto_files( );
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

    struct File *       files_tail;
    struct Directory *  dirs_tail;
    struct Misc *       misc_tail;

    //struct List *   path;

//wprintf( L"traverse: %s (%d)\n", build_tree_path, build_tree_path_len );
wprintf(L"\r%d dirs, %d files ", count( all_dirs ), count( all_files ) );
//print_StringList( this->path, 256 );
//wprintf(L"     ");

    this->n_files   = 0;
    this->files     = NULL;
    files_tail      = NULL;

    this->n_dirs    = 0;
    this->dirs      = NULL;
    dirs_tail       = NULL;

    this->n_misc    = 0;
    this->misc      = NULL;
    misc_tail       = NULL;


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
                        enqueue( dir->path, dir->name );

                        dir->parent     = this;

                        /* 'queue' to child-dir list (maintain sort order) */

                        dir->sibling    = NULL;

                        if (this->dirs == NULL) {
                            this->dirs = dir;
                        }

                        if (dirs_tail != NULL) {
                            dirs_tail->sibling = dir;
                        }

                        dirs_tail = dir;

                        ++this->n_dirs;

                        /* add to all_dirs bucket */
                        enqueue( all_dirs, dir );

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
                        enqueue( misc->path, misc->name );

                        misc->parent    = this;

                        /* 'queue' to child-misc list (maintain sort order) */

                        misc->sibling   = NULL;

                        if (this->misc == NULL) {
                            this->misc = misc;
                        }

                        if (misc_tail != NULL) {
                            misc_tail->sibling = misc;
                        }

                        misc_tail = misc;

                        ++this->n_misc;

                        /* add to all_misc bucket */
                        enqueue( all_misc, misc );

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
                        enqueue( file->path, file->name );

                        file->parent    = this;


                        file->size = (long long int)fdi->EndOfFile.QuadPart;
                        file->context = NULL;

                        /* 'queue' to child-files list (maintain sort order) */
                        file->sibling   = NULL;

                        if (this->files == NULL) {
                            this->files = file;
                        }

                        if (files_tail != NULL) {
                            files_tail->sibling = file;
                        }

                        files_tail = file;

                        ++this->n_files;
                        /* hash into approriate filesize bucket */
                        hash_file( file );

                        /* add to all_files bucket */
                        enqueue( all_files, file );

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
                wcsncpy( build_tree_path + path_len0, dir->name, MAX_PATH_BUFLEN - path_len0 );

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

        struct Iter *           iter;
        struct ScanDirectory *  scan_dir;

        iter = iterator( scan_paths );

        for ( scan_dir = next( iter ); scan_dir != NULL; scan_dir = next( iter )) {
            print_tree( (struct Directory *)scan_dir );
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


void
    print_buffer (
        void *          buffer,
        int             len,
        unsigned int    offset,
        const wchar_t * prefix,
        unsigned int    mark_offset,
        const wchar_t   marker
)
{
    unsigned char * buf = (unsigned char *)buffer + offset;
    static const int radix = 16; /* default radix */

    unsigned int i, j;

    for (i = 0; i < len; ++i) {
        if ((i % 16) == 0) {
            wprintf( L"%s", prefix );
            wprintf( L"%04X:%c", offset + i - (i % 16),
                ((mark_offset != 0) && ((i + offset) == mark_offset)) ? marker : L' ' );
        }

        wprintf( (radix == 10) ? L"%3d%c" : L"%02x%c", buf[offset+i],
                ((mark_offset != 0) && ((i + 1 + offset) == mark_offset)) ? marker : L' ' );

        if ((i % 4) == 3) {
            wprintf( L" " );
        }

        if ((i % 16) == 15) {
            wprintf( L"| " );

            for (j = i - 15; j <= i; ++j) {
                wprintf( L"%c", ((buf[offset+j] < ' ') /* || (buf[offset+j] > '~') */) ? '.' : buf[offset+j] );
            }

            if (i != (len -1)) {
                wprintf( L"\n" );

                if ((i % 128) == 127) {
                    wprintf( L"\n" );

/*
                    if ((i % 256) == 255) {
                        wprintf( L"\n" );
                    }
*/
                }
            }
        }
    }

    for (j = ((radix == 10) ? 4 : 3) * (16 - (i % 16)) + (4 - ((i / 4) % 4)); j; --j) {
        wprintf( L" " );
    }

    if ((i - (i % 16)) < len) {
        wprintf( L"| " );
    }

    for (j = i - (i % 16); j < len; ++j) {
        wprintf( L"%c", ((buf[offset+j] < ' ') || (buf[offset+j] > '~')) ? '.' : buf[offset+j] );
    }

    wprintf( L"\n" );
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


int hash_num_files;
int hash_num_fzbuckets;

void
    hash_stats (
        void
)
{
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

    for ( fzbucket = next( iter );
            (fzbucket != NULL) && (size > fzbucket->size);
                fzbucket = next( iter) );

    if (fzbucket == NULL || (size != fzbucket->size)) {

        fzbucket = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );

        if (fzbucket == NULL) {
            wprintf( L"falloc FilesizeBucket failed\n");
            return;
        }

        fzbucket->offs  = 0;
        fzbucket->size  = size;
        fzbucket->files = new_List( );

        insert( hash_bucket, iter, fzbucket );

        ++hash_num_fzbuckets;
    }

    done( iter );

    enqueue( fzbucket->files, file );

    ++hash_num_files;
/*
{
    void print_full_filename( struct File * file );

    wprintf( L"%10I64d => ", size );
    print_full_filename( file );
    wprintf( L"\n" );
}
*/

}



int
    bufsum (
        void *  buf,
        int     len
)
{
    int i;
    int lenX;
    int c0, c1;

    c0 = 1; c1 = 0;

    lenX = len/sizeof(int);

    for (i = 0; i < lenX; ++i) {
        c0 += ((int *)buf)[i];
        c1 += c0;
    }

    for (i = i * sizeof(int); i < len; ++i) { /* for (i = 0; i < len; ++i) */
        c0 += ((char *)buf)[i];
        c1 += c0;
    }

    return c1; /* return checksum */
}

int
    bufcmp (
        char *  bufA,
        char *  bufB,
        int     len
)
{
    int i;

    for (i = 0; (i < len) && (bufA[i] == bufB[i]); ++i);

    return i;
}


#define START_OFFSET    (0)
#define BUF_SIZE        (256*1024)

#define ERROR_RETRIES   (3)


FastAlloc fa_PreDittoContext;
FastAlloc fa_Buffer;

struct DittoBuckets {
/*  long long int   size; */
    struct List *   files_list;  /* list of files that are _ditto_ */
};



struct List *   ditto_buckets; /* list of list-of-files that are _ditto_ */

struct List *   partial_fzbuckets; /* list of fzbuckets (list-of-files with size/offs) that are likely ditto */

struct List *   retry_fzbuckets; /* list of fzbuckets (list-of-files with size/offs) that need to be retried */

struct List *   unique_files;

struct List *   error_files;


long long int   total_ditto_size;
long long int   total_ditto_count;



#define BUF_SIZE_ALIGN  (4096)

#define MIN_SIZE        (0)


void
    ditto_files (
        void
)
{
    int i;
    //int pre_ditto, pre_partial;

//int t0=clock();

    ditto_buckets       = new_List( );

    retry_fzbuckets     = new_List( );
    partial_fzbuckets   = new_List( );

    unique_files        = new_List( );
    error_files         = new_List( );


    fa_PreDittoContext  = new_falloc( L"PreDittoContext", sizeof(struct PreDittoContext) );
    fa_Buffer           = new_falloc( L"Buffer", BUF_SIZE );

wprintf(L"dittoing files ..\n");
    for (i = 0; i < HASH_BUCKETS_NUM; ++i) {
        file_dittoer( &hash_buckets[ i ] );
    }

//wprintf(L"dittoing files ..\n");
//    file_dittoer( retry_fzbuckets );


//    pre_ditto   = ditto_buckets->count;
//    pre_partial = partial_fzbuckets->count;

wprintf( L"\rpartial dittoing done: ditto=%d, partial=%d                         \n", ditto_buckets->count, partial_fzbuckets->count );

/*
    file_dittoer( partial_fzbuckets );

wprintf( L"\rdittoing complete: ditto=%d (was ditto=%d, partial=%d)                        \n", ditto_buckets->count, pre_ditto, pre_partial );
*/
    delete_falloc( fa_Buffer );
    delete_falloc( fa_PreDittoContext );
}


void
    file_dittoer (
        struct List *   fzbuckets_list
)
{
    struct FilesizeBucket * fzbucket;

    while ((fzbucket = pop( fzbuckets_list )) != NULL) {

        struct Iter *   iter;
        struct File *   file;

        long long int   size;
        long long int   offs;

        LARGE_INTEGER   li_offs;

        int buf_size; /* to checksum/compare */

        struct PreDittoContext *   preDit_head;
        struct PreDittoContext *   preDit;
        struct PreDittoContext *   preDit_prev;

        int fzbucket_error = 0;

//int t0 = clock( );

        offs    = fzbucket->offs;
        size    = fzbucket->size;

wprintf(L"\rdittoing bucket (num=%d), size=%I64d, count=%d, offs=%I64d          ", ditto_buckets->count, size, fzbucket->files->count, offs);

        if (fzbucket->files->count == 1) {
            break; /* skip buckets with one file */
        }

        if (size < MIN_SIZE) {
            break; /* skip buckets with files less than MIN_SIZE */
        }

        preDit_head         = NULL;

        //buf_size            = size - ((size + BUF_SIZE_ALIGN) / BUF_SIZE_ALIGN);
        buf_size            = BUF_SIZE; /* FIXME: say if (files->count == 2) use a larger buf-size? */
        li_offs.QuadPart    = offs;


        if (offs >= size) {
            /* we would really get here only for 0-byte files */

            push( ditto_buckets, fzbucket->files );

            total_ditto_count   += fzbucket->files->count;
            total_ditto_size    += size * (fzbucket->files->count - 1); // redundant bytes

//wprintf(L"\rditto #%d count=%d, size=%I64d (ticks=%d) [total count=%I64d, size=%I64d]:                     \n",
//ditto_buckets->count, fzbucket->files->count, size, clock()-t0, total_ditto_count, total_ditto_size);
//print_list(fzbucket->files, print_File, 50);
//wprintf(L"--\n");

            goto skip_dittoing;
        }


        iter = iterator( fzbucket->files ); 

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

                for (preDit_prev = NULL, preDit = preDit_head;
                        (preDit != NULL) && (checksum > preDit->checksum);
                            preDit_prev = preDit, preDit = preDit->next);

                if ((preDit == NULL) || (checksum != preDit->checksum)) {

                    /* new checksum -> new preDit */

                    preDit = falloc( fa_PreDittoContext, sizeof(struct PreDittoContext) );

                    if (preDit == NULL) {
                        wprintf(L"falloc preDit failed!\n");
                        return;
                    }

                    preDit->checksum    = checksum;
                    preDit->diff_at     = len;

                    preDit->buf         = buf;
                    preDit->len         = len; // == buf_size */

                    preDit->files       = new_List( );

                    if (preDit->files == NULL) {
                        wprintf(L"preDit->files new_List failed!\n");
                        return;
                    }

                    enqueue( preDit->files, file );

                    if (preDit_prev != NULL) {
                        preDit->next        = preDit_prev->next;
                        preDit_prev->next   = preDit;
                    } else {
                        preDit->next    = preDit_head;
                        preDit_head     = preDit;
                    }

                } else { /* if ((preDit != NULL) && (checksum == preDit->checksum)) */

                    diff_at = bufcmp( buf, preDit->buf, len );

                    if (diff_at == len) {

                        enqueue( preDit->files, file );

                        ffree( fa_Buffer, buf ); /* don't need the buffer any more */ /* FIXME: optimize to just re-use for next file */

                    } else {

                        /* this is for the (very) uncommon case where the checksum is the same, but the buffers differ */

{ int print_offs, print_len;
  print_offs = (diff_at < 32) ? 0 : diff_at-32;
  print_len  = ((print_offs + 256) > len) ? (len - print_offs) : 256;
  wprintf( L"\n## checksum mismatch sum=0x%08X offs=0x%X (size=%I64d) ##\n", checksum, diff_at, size );
  print_buffer(buf,         print_len, print_offs, L"BUF ", diff_at, L'*' ); wprintf(L"--\n");
  print_buffer(preDit->buf, print_len, print_offs, L"PRE ", diff_at, L'*' ); /* getchar(); */ }

                        diff_at2 = 0;

                        for (preDit_prev = preDit, preDit = preDit->next;
                                preDit != NULL && (checksum == preDit->checksum) && (diff_at <= preDit->diff_at);
                                    preDit_prev = preDit, preDit = preDit->next) {

                            if (diff_at == preDit->diff_at) {

                                diff_at2 = bufcmp( buf+diff_at, preDit->buf+diff_at, len-diff_at );

                                if (diff_at2 == len) {
                                    enqueue( preDit->files, file );
                                    ffree( fa_Buffer, buf ); /* FIXME: optimize */

{ wprintf( L"checksum mismatch, found buffer match\n" ); }

                                    break;
                                }
                            }

                        }

                        if (diff_at2 != len) {

{ wprintf( L"checksum mismatch, and NO buffer match\n" ); }

                            preDit = falloc( fa_PreDittoContext, sizeof(struct PreDittoContext) );

                            if (preDit == NULL) {
                                wprintf(L"falloc preDit failed!\n");
                                return;
                            }

                            preDit->checksum    = checksum;
                            preDit->diff_at     = diff_at;

                            preDit->buf         = buf;
                            preDit->len         = len; /* == buf_size */

                            preDit->files       = new_List( );

                            if (preDit->files == NULL) {
                                wprintf(L"preDit->files new_List failed!\n");
                                return;
                            }

                            enqueue( preDit->files, file );

                            if (preDit_prev != NULL) {
                                preDit->next        = preDit_prev->next;
                                preDit_prev->next   = preDit;
                            } else {
                                preDit->next    = preDit_head;
                                preDit_head     = preDit;
                            }

                        }

                    }

                }

            } else {

                wchar_t *   path = full_filename( file ); /* FIXME: free path buffer */

                wprintf( L"\nerror opening/reading file (or allocating buffer)! (file=%s)\n", path);

                ffree( fa_Buffer, buf );

                if (fzbuckets_list != retry_fzbuckets) {
                    ++fzbucket_error;
                    push( retry_fzbuckets, fzbucket ); /* push to retry bucket */
wprintf(L"will retry later\n");
                    break; /* skip bucket -> to retry later */
                } else {
wprintf(L"error on retry: %s\n", path);
                    push( error_files, file );
                }
            }
        }

        done( iter );

        if (fzbucket_error > 0) {
            wprintf(L"skipping bucket\n");
            continue;
        }


        /* finished processing files in this fzbucket; we don't need the fzbucket any more */

        delete_List( fzbucket->files );
        ffree( fa_FilesizeBucket, fzbucket );


skip_dittoing:

        /*
            now go through all of the 'preDit' buckets and:

            - dispose off buckets with just one file ( there is not a ditto for these files )

            - files whose 'new_offs' exceeds size, put into ditto bucket ( we have checked every byte of the file to match )

            - push back rest of the buckets back into the process list after updating 'offs' ( requires more processing )
         */

        offs = offs + buf_size; /* compute offs to compare next */

        preDit = preDit_head;

        while (preDit != NULL) {

            if (preDit->files->count == 1) { /* UNIQUE file */

                file = (struct File *)preDit->files->head->data;

                CloseHandle( file->context );
                file->context = NULL;

                enqueue( unique_files, file );

                delete_List( preDit->files );

            } else if (offs >= size) { /* DITTO file : these files match byte-to-byte */

                /* close open file handles */

                iter = iterator( preDit->files ); 

                for (file = next( iter ); file != NULL; file = next( iter )) {
                    CloseHandle( file->context );
                    file->context = NULL;
                }

                done( iter );

                push( ditto_buckets, preDit->files );

                total_ditto_count   += preDit->files->count;
                total_ditto_size    += size * (preDit->files->count - 1); // redundant bytes

//wprintf(L"\rditto #%d count=%d, size=%I64d (ticks=%d) [total count=%I64d, size=%I64d]                       ",
//ditto_buckets->count, preDit->files->count, size, clock()-t0, total_ditto_count, total_ditto_size);
//print_list(preDit->files, print_File, 50);
//wprintf(L"--\n");

            } else { /* partially matched : push a new FilesizeBucket to compare next set of bytes */

                /* push a new FilesizeBucket to compare next set of bytes */

                struct FilesizeBucket * fzbucket_new;

                fzbucket_new = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) ); /* FIXME: check for failure */

                fzbucket_new->offs   = offs;
                fzbucket_new->size   = size;
                fzbucket_new->files  = preDit->files;

                push( partial_fzbuckets, fzbucket_new );
            }

            ffree( fa_Buffer, preDit->buf );

            {
                void *  free_preDit;

                free_preDit = preDit;
                preDit = preDit->next;

                ffree( fa_PreDittoContext, free_preDit );
            }
        }
    }
}



void
    fuzzy_match_dirs (
        struct Directory *  dir
)
{

}

void
    ditto_dirs (
        void
)
{
}
