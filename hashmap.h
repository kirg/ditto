#ifndef __HASHMAP_H__
#define __HASHMAP_H__

void
    hashmap_init (
        void
);

void
    hashmap_cleanup (
        void
);


void
    hash_init (
        int             num_files,
        long long int   min_filesize,
        long long int   max_filesize
);

void
    hash_file (
        long long int   size,
        void *          file
);

void
    dump_hash (
        long long int min_size
);

#endif

