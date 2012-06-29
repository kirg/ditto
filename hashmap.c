#include "list.h"
#include "hashmap.h"
#include "ditto.h"

#include <malloc.h>
#include <stdio.h>

/*


Hash

bucket[0]   bucket[1]   bucket[2] ..
|           |           |
|           |           |->FilesizeBucket->FilesizeBucket->FilesizeBucket->..
|           |                                              |
|           |                                              |->File->File->File..
|           |->FilesizeBucket->FilesizeBucket..
|
|
|->FilesizeBucket->FilesizeBucket..


*/

struct FilesizeBucket {
    long long int   size;
    struct List *   files; /* list of Files */
};

#if 0
struct HashBucketHead {
    struct List     fzbuckets; /* list of FilesizeBucket */
};
#endif

struct Hash {

    int                     num_buckets;

    struct FilesizeBucket * zerosize_bucket;

    /* hashed by size % num_hash_buckets */
    struct List *           buckets;        /* array of <list of FilesizeBucket> */

    /* hashed by size / num_size_buckets */
    struct List *           buckets_bysize;   /* array of <list of FilesizeBucket> */
};


struct Hash hash;


FastAlloc fa_FilesizeBucket;
FastAlloc fa_HashBucketHead;

void
    hashmap_init (
        void
)
{
    fa_FilesizeBucket = new_falloc( L"FilesizeBucket", sizeof(struct FilesizeBucket) );
    fa_HashBucketHead = new_falloc( L"HashBucketHead", sizeof(struct List) );
}

void
    hashmap_cleanup (
        void
)
{
    delete_falloc( fa_HashBucketHead );
    delete_falloc( fa_FilesizeBucket );
}

#define MIN_HASH_BUCKETS    (16)

void
    hash_init (
        int             num_files,
        long long int   min_filesize,
        long long int   max_filesize
)
{
    hash.num_buckets = num_files / 256;

    if (hash.num_buckets < MIN_HASH_BUCKETS) {
        hash.num_buckets = MIN_HASH_BUCKETS;
    }

wprintf(L"hash_init: num_files=%d, num_buckets=%d\n", num_files, hash.num_buckets);

    hash.buckets = falloc( fa_HashBucketHead, hash.num_buckets * sizeof(struct List) );

    hash.zerosize_bucket = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );

    if ( hash.buckets && hash.zerosize_bucket ) {
        int i;

        for ( i = 0; i <= hash.num_buckets; ++i) {
            hash.buckets[ i ].head  = NULL;
            hash.buckets[ i ].tail  = NULL;
            hash.buckets[ i ].count = 0;
        }

        hash.zerosize_bucket->size  = 0;
        hash.zerosize_bucket->files = new_List( );

    } else {
        wprintf( L"malloc hash.buckets or hash.zerosize_bucket failed\n" );
    }


}

long long int   max_filesize;
void *          max_filesize_file;

void
    hash_file (
        long long int   size,
        void *          file
)
{
    struct List *           bucket;
    struct FilesizeBucket * fzbucket;


#if dbg
    {
        void print_full_filename( void * file );

        wprintf( L"%10I64d => ", size );
        print_full_filename( file );
        wprintf( L"\n" );
    }
#endif


    if (size == 0) {

        queue( hash.zerosize_bucket->files, file );

    } else {

        struct Iter *   iter;

        bucket = &hash.buckets[ size % hash.num_buckets ];

        iter = iterator( bucket );

        for ( fzbucket = next( iter );
                (fzbucket != NULL) && (fzbucket->size != size);
                    fzbucket = next( iter) );

        done( iter );

        if (fzbucket == NULL) {

            fzbucket = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );

            if (fzbucket == NULL) {
                wprintf( L"malloc FilesizeBucket failed\n");
                return;
            }

            fzbucket->size  = size;
            fzbucket->files = new_List( );

            queue( bucket, fzbucket );
        }

        queue( fzbucket->files, file );
    }

    if (size > max_filesize) {
        max_filesize        = size;
        max_filesize_file   = file;
    }
        
}


int max_bucket_count;
struct FilesizeBucket * max_bucket_count_fzbucket;

void
    dump_fzbucket (
        struct FilesizeBucket * fzbucket
)
{
    struct Iter *   iter;
    struct File *   file;

    void print_full_filename( void * file );

    wprintf( L"####: %I64d bytes (count=%d)\n", fzbucket->size, count( fzbucket->files) );

    iter = iterator( fzbucket->files );

    for (file = next( iter ); file != NULL; file = next( iter )) {

        wprintf( L"  ");
        print_full_filename( file );
        wprintf( L"\n" );

    }

    done( iter );

    wprintf( L"\n" );
}


void
    dump_hash (
        long long int min_size
)
{
    struct FilesizeBucket * fzbucket;

    void print_full_filename( void * file );

    int i;

    for (i = 0; i < hash.num_buckets; ++i) {

        struct Iter *   iter;

        iter = iterator( &hash.buckets[ i ] );
        
        for ( fzbucket = next( iter );
                (fzbucket != NULL);
                    fzbucket = next( iter ) ) {

//            if ((count( fzbucket->files ) > 1) &&
//                    ((count( fzbucket->files ) * fzbucket->size) > min_size)) {

//              dump_fzbucket( fzbucket );
//            }

            if (count( fzbucket->files ) > max_bucket_count) {
                max_bucket_count            = count( fzbucket->files );
                max_bucket_count_fzbucket   = fzbucket;
            }
        }

        done( iter );
    }


    //dump_fzbucket( hash.zerosize_bucket );


    if (max_bucket_count_fzbucket != NULL) {
        wprintf(L"======================\n");
        wprintf(L"==== max bucket ====\n");
        dump_fzbucket( max_bucket_count_fzbucket );
    }
}


int
    compare_FilesizeBucket_by_count (
        struct FilesizeBucket * left,
        struct FilesizeBucket * right
)
{
    return (left->files->count > right->files->count) ? -1 :
                (left->files->count == right->files->count) ? 0 : 1;
}

int
    compare_FilesizeBucket_by_filesize (
        struct FilesizeBucket * left,
        struct FilesizeBucket * right
)
{
    return (left->size > right->size) ? -1 :
                (left->size == right->size) ? 0 : 1;
}

int
    compare_FilesizeBucket_by_totalsize (
        struct FilesizeBucket * left,
        struct FilesizeBucket * right
)
{
    return ((left->size * left->files->count) > (right->size * right->files->count)) ? -1 :
                ((left->size * left->files->count) == (right->size * right->files->count)) ? 0 : 1;
}

void
    print_FilesizeBucket (
        struct FilesizeBucket * fz
)
{
    wprintf( L"%8d files -> %8I64d bytes\n", fz->files->count, fz->size );
    print_list( fz->files, print_File, 5 );
}


void
    hash_stats (
        void
)
{
    struct List * twoplus;
    struct List * twoplus_by_count;
    struct List * twoplus_by_filesize;
    struct List * twoplus_by_totalsize;

    int num;

    int i;

    twoplus = new_List( );

    num = 1; /* 1 -> zerosize_bucket */

    if (count( hash.zerosize_bucket->files ) > 1) {
        push( twoplus, hash.zerosize_bucket );
    }

    for (i = 0; i < hash.num_buckets; ++i) {

        struct FilesizeBucket * fzbucket;
        struct Iter *           iter;

        iter = iterator( &hash.buckets[ i ] );
        
        for ( fzbucket = next( iter );
                (fzbucket != NULL);
                    fzbucket = next( iter ) ) {

            ++num;

            if (count( fzbucket->files ) > 1) {
                push( twoplus, fzbucket );
            }
        }

        done( iter );
    }

    twoplus_by_count = clone( twoplus );
    merge_sort( twoplus_by_count, (compare_func)compare_FilesizeBucket_by_count );
    
    twoplus_by_filesize = clone( twoplus );
    merge_sort( twoplus_by_filesize, (compare_func)compare_FilesizeBucket_by_filesize );

    {
        struct FilesizeBucket * fzbucket;
        struct Iter *           iter;

        iter = iterator( twoplus_by_filesize );
        
        for ( fzbucket = next( iter );
                (fzbucket != NULL);
                    fzbucket = next( iter ) ) {

            if (count( fzbucket->files ) > 1) {
                dittoer( fzbucket->size, fzbucket->files );
            }
        }

        done( iter );
    }

    twoplus_by_totalsize = clone( twoplus );
    merge_sort( twoplus_by_totalsize, (compare_func)compare_FilesizeBucket_by_totalsize );
/*
    wprintf( L"filesize buckets:        %d\n", num );
    wprintf( L"2-plus filesize buckets: %d\n", twoplus->count );

    wprintf( L"0-byte filesize bucket:  %d files\n", hash.zerosize_bucket->files->count );
    wprintf( L"2-plus filesize buckets:   %d\n", twoplus->count );

    wprintf( L"\ntop 50 buckets by count:\n" );
    print_list( twoplus_by_count, (print_func)print_FilesizeBucket, 50 );

    wprintf( L"\ntop 25 (2-plus) buckets by filesize:\n" );
    print_list( twoplus_by_filesize, (print_func)print_FilesizeBucket, 25 );

    wprintf( L"\ntop 25 (2-plus) buckets by total size:\n" );
    print_list( twoplus_by_totalsize, (print_func)print_FilesizeBucket, 25 );

    wprintf( L"\n" );
*/
    delete_List( twoplus_by_totalsize );
    delete_List( twoplus_by_filesize );
    delete_List( twoplus_by_count );
    delete_List( twoplus );

/*

    {
        void
            find_dittofiles (
                void
        );
        find_dittofiles( );
    }
*/
}


/*
struct DittoDir {

    struct List     dirs;
};

struct DittoFile {

    struct List     files;
};
*/



void
    find_dittofiles (
        void
)
{
    int i;

    for (i = 0; i < hash.num_buckets; ++i) {

        struct FilesizeBucket * fzbucket;
        struct Iter *           iter;

        iter = iterator( &hash.buckets[ i ] );
        
        for ( fzbucket = next( iter );
                (fzbucket != NULL);
                    fzbucket = next( iter ) ) {

            if (count( fzbucket->files ) > 1) {
                dittoer( fzbucket->size, fzbucket->files );
            }
        }

        done( iter );
    }
}

