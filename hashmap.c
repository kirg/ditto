#include "list.h"

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

struct FilesizeBucket *
    new_FilesizeBucket (
        void
)
{
    return falloc( fa_FilesizeBucket, sizeof(struct FilesizeBucket) );
}

void
    hashmap_init (
        void
)
{
    fa_FilesizeBucket = new_FastAllocator( L"FilesizeBucket" );
}

void
    hashmap_cleanup (
        void
)
{
    free( hash.zerosize_bucket );
    free( hash.buckets );

    free_FastAllocator( fa_FilesizeBucket );
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

    hash.zerosize_bucket = malloc( sizeof(struct FilesizeBucket) );

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


#define hash_dbg    //wprintf

void
    hash_file (
        long long int   size,
        void *          file
)
{
    struct HashBucketHead * bucket;
    struct FilesizeBucket * fzbucket;

/*
    {
        void print_full_filename( void * file );

        wprintf( L"%10I64d => ", size );
        print_full_filename( file );
        wprintf( L"\n" );
    }
*/

    if (size == 0) {

        add_list( &hash.zerosize_bucket->files, file );

    } else {

hash_dbg(L"0");
        bucket = &hash.buckets[ size % hash.num_buckets ];

hash_dbg(L"1");
        for ( fzbucket = (struct FilesizeBucket *)bucket->head.first;
                (fzbucket != NULL) && (fzbucket->size != size);
                    fzbucket = (struct FilesizeBucket *)fzbucket->link.next );

hash_dbg(L"2");
        if (fzbucket == NULL) {

hash_dbg(L"3");
            //fzbucket = malloc( sizeof(struct FilesizeBucket) );
            fzbucket = new_FilesizeBucket( );

hash_dbg(L"4");
            if (fzbucket == NULL) {
                wprintf( L"malloc FilesizeBucket failed\n");
                return;
            }
hash_dbg(L"5");

            fzbucket->files.count   = 0;
            fzbucket->files.first   = NULL;

            fzbucket->size = size;

hash_dbg(L"6");
            push( &bucket->head, &fzbucket->link );
hash_dbg(L"7");
        }

hash_dbg(L"8");
        add_list( &fzbucket->files, file );
hash_dbg(L"9\n");
    }
}

void
    dump_hash (
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

            if ((fzbucket->files.count > 1) && ((fzbucket->files.count * fzbucket->size) > (16*1024*1024))) {

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
        }
    }
}


struct DittoDir {

    struct Head     dirs;
};

struct DittoFile {

    struct Head     files;
};



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


