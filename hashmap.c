#include "list.h"

#include <malloc.h>

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
    struct Link     link;

    long long int   size;
    struct Head     files; /* list of File */
};

struct HashBucketHead {
    struct Head     head; /* list of FilesizeBucket */
};

struct Hash {

    int                 num_buckets;

    struct FilesizeBucket * zerosize_bucket;

    /* hashed by size % num_hash_buckets */
    struct HashBucketHead * buckets;        /* array */

    /* hashed by size / num_size_buckets */
    struct HashBucketHead * buckets_bysize;   /* array */
};


struct Hash hash;


FastAlloc fa_FilesizeBucket;

void
    hashmap_init (
        void
)
{
    fa_FilesizeBucket = new_falloc( L"FilesizeBucket", sizeof(struct FilesizeBucket) );
}

void
    hashmap_cleanup (
        void
)
{
    free( hash.buckets );

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

    hash.buckets = malloc( hash.num_buckets * sizeof(struct HashBucketHead) );

    hash.zerosize_bucket = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );

    if ( hash.buckets && hash.zerosize_bucket ) {
        int i;

        for ( i = 0; i <= hash.num_buckets; ++i) {
            hash.buckets[ i ].head.count  = 0;
            hash.buckets[ i ].head.first  = NULL;
        }

        hash.zerosize_bucket->size  = 0;

        hash.zerosize_bucket->files.count   = 0;
        hash.zerosize_bucket->files.first   = NULL;

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
    struct HashBucketHead * bucket;
    struct FilesizeBucket * fzbucket;


#if 0
    {
        void print_full_filename( void * file );

        wprintf( L"%10I64d => ", size );
        print_full_filename( file );
        wprintf( L"\n" );
    }
#endif


    if (size == 0) {

        add_list( &hash.zerosize_bucket->files, file );

    } else {

        bucket = &hash.buckets[ size % hash.num_buckets ];

        for ( fzbucket = (struct FilesizeBucket *)bucket->head.first;
                (fzbucket != NULL) && (fzbucket->size != size);
                    fzbucket = (struct FilesizeBucket *)fzbucket->link.next );

        if (fzbucket == NULL) {

            fzbucket = falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );

            if (fzbucket == NULL) {
                wprintf( L"malloc FilesizeBucket failed\n");
                return;
            }

            fzbucket->files.count   = 0;
            fzbucket->files.first   = NULL;

            fzbucket->size = size;

            insert_list( &bucket->head, &fzbucket->link );
        }

        add_list( &fzbucket->files, file );
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
    struct Link * link;

    void print_full_filename( void * file );

    wprintf( L"####: %I64d bytes (count=%d)\n", fzbucket->size, fzbucket->files.count );

    for (link = fzbucket->files.first; link != NULL; link = link->next) {

        struct File *   file;

        file = (struct File *)link->data;

        wprintf( L"  ");
        print_full_filename( file );
        wprintf( L"\n" );

    }

    wprintf( L"\n" );
}


void
    dump_hash (
        long long int min_size
)
{
    struct HashBucketHead * bucket;
    struct FilesizeBucket * fzbucket;

    void print_full_filename( void * file );

    int i;

    for (i = 0; i < hash.num_buckets; ++i) {
        
        for ( fzbucket = (struct FilesizeBucket *)hash.buckets[ i ].head.first;
                (fzbucket != NULL);
                    fzbucket = (struct FilesizeBucket *)fzbucket->link.next ) {

            if ((fzbucket->files.count > 1) && ((fzbucket->files.count * fzbucket->size) > min_size)) {

                dump_fzbucket( fzbucket );
            }

            if (fzbucket->files.count > max_bucket_count) {
                max_bucket_count            = fzbucket->files.count;
                max_bucket_count_fzbucket   = fzbucket;
            }
        }
    }

    dump_fzbucket( hash.zerosize_bucket );


    wprintf(L"======================\n");
    wprintf(L"==== max bucket ====\n");
    dump_fzbucket( max_bucket_count_fzbucket );
}


struct DittoDir {

    struct Head     dirs;
};

struct DittoFile {

    struct Head     files;
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

    struct Link * link;

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


    for (link = fzbucket->files.first; link != NULL; link = link->next) {

        struct File *   file;

        file = (struct File *)link->data;

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
                    fzbucket = (struct FilesizeBucket *)fzbucket->link.next ) {

            struct Link * link;

            if (fzbucket->files.count > 1) {

                for (link = fzbucket->files.first; link != NULL; link = link->next) {

                    struct File *   file;

                    file = (struct File *)link->data;

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

