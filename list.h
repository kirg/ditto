#ifndef _LIST_H_
#define _LIST_H_

#include "falloc.h"

#include <stdio.h>


#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif

void
    list_init (
        void
);

void
    list_cleanup (
        void
);

struct List {
    struct Node *   head;
    struct Node *   tail;

    int             count;
};

struct Node {
    struct Node *   next;
    void *          data;
};




struct Iter {
    struct Node *   current;
};



extern FastAlloc fa_List;

struct List *
    new_List (
        void
);

struct Node *
    new_Node (
        void
);

void
    delete_Node (
        struct Node *   node
);

void
    delete_List (
        struct List *   list
);



void
    insert_node (
        struct List *   list,
        struct Node *   data
);

struct Node *
    remove_node (
        struct List *   list
);

void
    insert_node_tail (
        struct List *   list,
        struct Node *   node
);


static inline
int
    count (
        struct List *   list
)
{
    return list->count;
}




static inline
struct Iter *
    iterator (
        struct List *   list
)
{
    struct Iter * iter = falloc( fa_List, sizeof(struct Iter) );

    if (iter != NULL) {
        iter->current   = list->head;
    }

    return iter;
}

static inline
void *
    next (
        struct Iter *   iter
)
{
    void *  data;

    if (iter != NULL && iter->current != NULL) {
        data            = iter->current->data;
        iter->current   = iter->current->next;
    } else {
        data = NULL;
    }

    return data;
}

static inline
int
    has_next (
        struct Iter *   iter
)
{
    return (iter != NULL) && (iter->current != NULL);
}


static inline
void
    done (
        struct Iter *   iter
)
{
    if (iter != NULL) {
        ffree( fa_List, iter );
    }
}



static inline
void
    queue (
        struct List *   list,
        void *          data
)
{
    struct Node * node;

    node = falloc( fa_List, sizeof(struct Node) );


    if (node != NULL) {

        node->data  = data;
        node->next  = NULL;

        if (list->tail != NULL) {
            list->tail->next  = node;
        } else {
            list->head = node;
        }

        list->tail = node;

        ++list->count;
    }
}

static inline
void *
    dequeue (
        struct List *   list
)
{
    void *  data;

    data = NULL;

    if (list->head != NULL) {

        struct Node *  node;

        data        = list->head->data;

        node        = list->head;
        list->head  = list->head->next;

        if (list->tail == node) {
            list->tail = NULL;
        }

        ffree( fa_List, node );

        --list->count;

    }

    return data;
}



static inline
void
    push (
        struct List *   list,
        void *          data
)
{
    struct Node * node;

    node = falloc( fa_List, sizeof(struct Node) );

    if (node != NULL) {

        node->data  = data;

        node->next  = list->head;
        list->head  = node;

        ++list->count;

        if (list->tail == NULL) {
            list->tail = node;
        }
    } else {
        wprintf(L"falloc Node failed\n");
    }
}

static inline
void *
    pop (
        struct List *   list
)
{
    return dequeue( list );
}


static inline
struct List *
    clone (
        struct List *   list
)
{
    struct List * clone_list;

    clone_list = falloc( fa_List, sizeof(struct List) );

    if (clone_list) {

        if (list->count > 0) {

            struct Node *  l;
            struct Node *  c;
            struct Node *  tail;

            c = falloc( fa_List, sizeof(struct Node) );

            if (c) {

                l       = list->head;
                c->data = l->data;

                clone_list->head    = c;
                tail                = c;

                for (l = l->next; l != NULL; l = l->next) {

                    c = falloc( fa_List, sizeof(struct Node) );

                    if (c) {
                        c->data = l->data;

                        tail->next  = c;
                        tail        = c;
                    } else {
                        /* FIXME: free up buffers */
                        clone_list = NULL;
                        break;
                    }
                }

                if (clone_list != NULL) {

                    tail->next = NULL;

                    clone_list->tail    = tail;
                    clone_list->count   = list->count;
                }

            } else {
                /* FIXME: free up allocated buffers */
                clone_list = NULL;
            }

        } else {

            clone_list->count   = 0;
            clone_list->head    = NULL;
            clone_list->tail    = NULL;
        }
    }

    return clone_list;
}


typedef
int
    (* compare_func) (
        void * left,
        void * right
);


typedef
void
    (* print_func) (
        void * value
);

struct List *
    merge_sort (
        struct List *   list,
        compare_func    compare
);

void
    print_list (
        struct List *   list,
        print_func      print,
        int             num
);

#endif

