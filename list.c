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


void
    print_list (
        struct List *   list,
        print_func      print
)
{
    struct Node *   node;

    wprintf(L"list: count=%d, head=%p, tail=%p\n", list->count, list->head, list->tail);

    for (node = list->head; node != NULL; node = node->next) {
        //wprintf(L"node: ptr=%p, next=%p, value=", node, node->next);
        print(node->data);
        wprintf(L"\n");
    }

    wprintf(L"\n");
}


struct List *
    merge_sort (
        struct List *   list,
        compare_func    compare
)
{
    if (list->count > 1) {

        struct List * left_list;
        struct List * right_list;

        struct Node * iter;

        int i;

        for (i = 1, iter = list->head; i < list->count/2; ++i) {
            iter =  iter->next;
        }

        left_list   = falloc( fa_List, sizeof(struct List) );
        right_list  = falloc( fa_List, sizeof(struct List) );

        if (left_list && right_list) {

            struct Node *   left;
            struct Node *   right;

            left_list->head     = list->head;
            left_list->tail     = iter; /* mid */
            left_list->count    = i;

            right_list->head    = iter->next; /* mid->next */
            right_list->tail    = list->tail;
            right_list->count   = list->count - i;

            iter->next = NULL;

            left_list = merge_sort(left_list, compare);

            right_list = merge_sort(right_list, compare);

            left    = left_list->head;
            right   = right_list->head;

            if (compare( left->data, right->data ) <= 0) {
                list->head  = left;
                left        = left->next;
            } else {
                list->head  = right;
                right       = right->next;
            }

            for( node = list->head; left != NULL || right != NULL; node = node->next ) {

                if (left != NULL && (right == NULL || (compare( left->data, right->data )) <= 0)) {

                    node->next  = left;
                    left        = left->next;

                } else {

                    node->next  = right;
                    right       = right->next;
                }

                node = node->next;

            }

            list->tail = node;

            ffree( right_list );
            ffree( left_list );

        } else {
wprintf(L"falloc left_list/right_list failed\n");
            list = NULL;
        }

    } else {
        /* return list; */
    }

    return list;
}

