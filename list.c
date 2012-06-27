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

struct List *
    merge_sort (
        struct List *   list,
        compare_func    compare,
        print_func      print
        
)
{
    struct List *   left_list;
    struct List *   right_list;

    struct Node *   mid;

    int i;

{
    struct Node *   node;
    wprintf(L"PRE sort list: count=%d, head=%p, tail=%p\n", list->count, list->head, list->tail);

    for (node = list->head; node != NULL; node = node->next) {
        wprintf(L"node: ptr=%p, next=%p, value=", node, node->next);
        print(node->data);
        wprintf(L"\n");
    }

    wprintf(L"\n");
}

    if (list->count <= 1) {
wprintf(L"0");
        return list;
    }

    for (i = 0, mid = list->head; i < list->count/2; ++i) {
wprintf(L"1");
        mid =  mid->next;
    }

    left_list   = falloc( fa_List, sizeof(struct List) );
    right_list  = falloc( fa_List, sizeof(struct List) );

wprintf(L"2");
    if (left_list && right_list) {

        struct Node *   left;
        struct Node *   right;
        struct Node *   node;

wprintf(L"3");
        mid->next       = NULL;

        left_list->head     = list->head;
        left_list->tail     = mid;
        left_list->count    = (list->count + 1)/2;

        right_list->head    = mid->next;
        right_list->tail    = list->tail;
        right_list->count   = (list->count - 1)/2;

wprintf(L" LEFT ");
        left_list   = merge_sort(left_list, compare, print);

wprintf(L" RIGHT ");
        right_list  = merge_sort(right_list, compare, print);

wprintf(L" DONE ");
        left    = left_list->head;
        right   = right_list->head;

wprintf(L"6");
        if (compare(left->data, right->data) <= 0) {
            node    = left;
            left    = left->next;
wprintf(L"7");
        } else {
            node    = right;
            right   = right->next;
wprintf(L"8");
        }

        list->head = node;

wprintf(L"9\n");
        do {
            
            if (left != NULL && (right == NULL || compare(left->data, right->data)) <= 0) {

                node->next  = left;
                left        = left->next;

            } else {

                node->next  = right;
                right       = right->next;
            }

            node = node->next;

wprintf(L"6");
        } while (node != NULL);

        /* list->count remains unchanged */

wprintf(L"7\n");
{
    struct Node *   node;
    wprintf(L"PRE sort list: count=%d, head=%p, tail=%p\n", list->count, list->head, list->tail);

    for (node = list->head; node != NULL; node = node->next) {
        wprintf(L"node: ptr=%p, next=%p, value=", node, node->next);
        print(node->data);
        wprintf(L"\n");
    }

    wprintf(L"\n");
}
    }

    return list;
}
