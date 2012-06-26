#include "falloc.h"
#include "list.h"

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


struct List *
    new_List (
        void
)
{
    struct List * list = falloc( fa_List, sizeof(struct List) );

    if (list) {
        list->head = NULL;
        list->tail = NULL;
        list->count = 0;
    }

    return list;
}

struct Node *
    new_Node (
        void
)
{
    struct Node * node = falloc( fa_List, sizeof(struct Node) );

    if (node) {
        node->next = 0;
        node->data = NULL;
    }

    return node;
}

void
    delete_Node (
        struct Node *   node
)
{
    ffree( fa_List, node );
}

void
    delete_List (
        struct List *   list
)
{
    struct Node * node;
    struct Node * next;

    for (node = list->head; node != NULL; node = next) {
        next = node->next;
        ffree( fa_List, node );
    }

    ffree( fa_List, list );
}

