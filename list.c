#include "falloc.h"
#include "list.h"

#include <stdio.h>


FastAlloc   fa_List;

#define MIN_OF(A, B) (((A) < (B)) ? (A) : (B))
#define MAX_OF(A, B) (((A) > (B)) ? (A) : (B))
#define MAX_OF_3(A, B, C) MAX_OF( A, MAX_OF( B, C ) )

void
    list_init (
        void
)
{
    fa_List = new_falloc( L"List",
        MAX_OF_3( sizeof(struct List), sizeof(struct Node), sizeof(struct Iter) ) );
}

void
    list_cleanup (
        void
)
{
    delete_falloc( fa_List );
}

