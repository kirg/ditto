#include "list.h"

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


    dump_fzbucket( hash.zerosize_bucket );


    if (max_bucket_count_fzbucket != NULL) {
        wprintf(L"======================\n");
        wprintf(L"==== max bucket ====\n");
        dump_fzbucket( max_bucket_count_fzbucket );
    }
}


struct DittoDir {

    struct List     dirs;
};

struct DittoFile {

    struct List     files;
};



#if 0
void
    dittoing (
        struct FilesizeBucket * fzbucket
)
{
    struct FileContext {
        HANDLE  hF;
        char *  buf;
        int     len;

        File *  file;
    };

    struct Node * node;

    int num = fzbucket->files.count;

    struct FileContext * files = malloc( num * sizeof(struct FileContext) );

    long long int   size = fzbucket->size;
    long long int   offs = START_OFFSET;


    for (i = 0; i < num; ++i) {
        files[i].hF = CreateFile(..);
        files[i].len = BUF_SIZE;
        files[i].buf = falloc( BUF_SIZE );

        SetFilePointerEx( offs );
        ReadFile( hF, BUF_SIZE );
    }


    for (node = fzbucket->files.first; node != NULL; node = node->next) {

        struct File *   file;

        file = (struct File *)node->data;

        wprintf( L"  "); print_full_filename( file ); wprintf( L"\n" );


    }

}

void
    find_dittofiles (
        void
)
{
    struct HashBucketHead * bucket;
    struct FilesizeBucket * fzbucket;

    void print_full_filename( void * file );

    int i;

    for (i = 0; i < hash.num_buckets; ++i) {
        
        for ( fzbucket = (struct FilesizeBucket *)hash.buckets[ i ].head.first;
                (fzbucket != NULL);
                    fzbucket = (struct FilesizeBucket *)fzbucket->node.next ) {

            struct Node * node;

            if (fzbucket->files.count > 1) {

                for (node = fzbucket->files.first; node != NULL; node = node->next) {

                    struct File *   file;

                    file = (struct File *)node->data;

                    wprintf( L"  "); print_full_filename( file ); wprintf( L"\n" );


                }

                wprintf( L"\n" );

            } else {

                /* FIXME: free up fzbuckets with just one file (implies unique file) */
            }
        }
    }
}

#endif

