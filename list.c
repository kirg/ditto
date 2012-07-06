#include "falloc.h"
#include "list.h"

#include <stdio.h>


FastAlloc   fa_List;

#define MIN_OF(A, B) (((A) < (B)) ? (A) : (B))
#define MAX_OF(A, B) (((A) > (B)) ? (A) : (B))

void
    list_init (
        void
)
{
    fa_List = new_falloc( L"List",
                    MAX_OF( MAX_OF( sizeof(struct List), sizeof(struct Node) ),
                        sizeof(struct Iter) ) );
}

void
    list_cleanup (
        void
)
{
    delete_falloc( fa_List );
}

