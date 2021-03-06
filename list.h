#ifndef _LIST_H_
#define _LIST_H_

#include "falloc.h"

#include <stdio.h>


#ifdef _MSC_VER
#   ifndef inline
#       define inline  __inline
#   endif
#endif


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
    struct Node *   previous;
    struct List *   list;       /* used by 'l_next()' to find head */
};



extern FastAlloc fa_List;
extern FastAlloc fa_Node;

void
    list_init (
        void
);

void
    list_cleanup (
        void
);


static inline
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

static inline
void
    delete_List (
        struct List *   list
)
{
    struct Node * node;
    struct Node * next;

    for (node = list->head; node != NULL; node = next) {
        next = node->next;
        ffree( fa_Node, node );
    }

    ffree( fa_List, list );
}





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
    l_iterator (
        struct List *   list
)
{
    struct Iter * iter = falloc( fa_List, sizeof(struct Iter) );

    if (iter != NULL) {
        iter->current   = NULL;
        iter->previous  = NULL;
        iter->list      = list;
    }

    return iter;
}

static inline
void *
    l_next (
        struct Iter *   iter
)
{
    void *  data;

    //wprintf(L"l_next: iter=%p, list=%p, curr=%p, prev=%p\n", iter, iter->list, iter->current, iter->previous);

    if (iter != NULL) {

        if (iter->current == NULL) {
            iter->previous  = NULL;
            iter->current   = iter->list->head;

        } else {
            iter->previous  = iter->current;
            iter->current   = iter->current->next;
        }

        data = (iter->current != NULL) ? iter->current->data : NULL/* EOL */;

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
    return (iter != NULL) && (iter->current != NULL) && (iter->current->next != NULL);
}



static inline
void
    l_insert (
        struct Iter *   iter,
        void *          data
)
{
    struct Node * node;

    node = falloc( fa_Node, sizeof(struct Node) );

    if (node != NULL) {

        node->data  = data;

        if (iter->previous != NULL) {

            node->next              = iter->previous->next;
            iter->previous->next    = node;

            if (iter->previous == iter->list->tail) {
                iter->list->tail = node;
            }

        } else /* if (iter->current != NULL) */ {

            node->next  = iter->list->head;
            iter->list->head  = node;

            if (iter->list->tail == NULL) {
                iter->list->tail = node;
            }
        }

        iter->previous = node;

        ++iter->list->count;
    }
}

static inline
void *
    l_delete (
        struct Iter *   iter
)
{
    if (iter->current != NULL) {

        struct Node *   node;

        node            = iter->current;
        iter->current   = node->next;

        if (iter->previous != NULL) {
            iter->previous->next    = node->next;
        } else {
            iter->list->head        = node->next;
        }
            
        if (iter->list->tail == node) {
            iter->list->tail = iter->previous;
        }

        --iter->list->count;

        ffree( fa_Node, node );

        return (iter->current != NULL) ? iter->current->data : NULL;

    } else {
        return NULL;
    }
}

static inline
void
    l_done (
        struct Iter *   iter
)
{
    if (iter != NULL) {
        ffree( fa_List, iter );
    }
}


static inline
void
    l_enqueue (
        struct List *   list,
        void *          data
)
{
    struct Node * node;

    node = falloc( fa_Node, sizeof(struct Node) );

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
    l_dequeue (
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

        ffree( fa_Node, node );

        --list->count;

    }

    return data;
}



static inline
void
    l_push (
        struct List *   list,
        void *          data
)
{
    struct Node * node;

    node = falloc( fa_Node, sizeof(struct Node) );

    if (node != NULL) {

        node->data  = data;

        node->next  = list->head;
        list->head  = node;

        if (list->tail == NULL) {
            list->tail = node;
        }

        ++list->count;

    } else {
        wprintf(L"falloc Node failed\n");
    }
}

static inline
void *
    l_pop (
        struct List *   list
)
{
    return l_dequeue( list );
}


static inline
struct List *
    l_clone (
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

            c = falloc( fa_Node, sizeof(struct Node) );

            if (c) {

                l       = list->head;
                c->data = l->data;

                clone_list->head    = c;
                tail                = c;

                for (l = l->next; l != NULL; l = l->next) {

                    c = falloc( fa_Node, sizeof(struct Node) );

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


static inline
void
    l_print (
        struct List *   list,
        print_func      print,
        int             num
)
{
    struct Node *   node;
    int i;

    if (num == 0) {
        for (node = list->head; node != NULL; node = node->next) {
            print(node->data);
        }
    } else {
        for (node = list->head, i = 0;
                node != NULL && i < num;
                    node = node->next, ++i) {
            print(node->data);
        }
    }
}


/* merge sort (in-place) list */
static inline
struct List *
    l_sort (
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

            left_list = l_sort(left_list, compare);
            right_list = l_sort(right_list, compare);

            left    = left_list->head;
            right   = right_list->head;

            if (compare( left->data, right->data ) <= 0) {
                list->head  = left;
                left        = left->next;
            } else {
                list->head  = right;
                right       = right->next;
            }

            for( iter = list->head;
                    left != NULL || right != NULL;
                        iter = iter->next ) {

                if (left != NULL &&
                        (right == NULL ||
                            (compare( left->data, right->data )) <= 0)) {

                    iter->next  = left;
                    left        = left->next;

                } else {

                    iter->next  = right;
                    right       = right->next;
                }
            }

            list->tail = iter;


            ffree( fa_List, right_list );
            ffree( fa_List, left_list );

        } else {
wprintf(L"falloc left_list/right_list failed\n");
            list = NULL;
        }

    } else {
        /* list->count is 0 or 1, do nothing and return list */
    }

    return list;
}


struct Collection {
    struct List *   list;

    int             elems;
    int             unique_elems;
};

struct Elem {
    void *  key;
    int     count;
};


static inline
struct Collection *
    new_Collection (
        void
)
{
    struct Collection * collection;

    collection = falloc( fa_List, sizeof(struct Collection) );

    if (collection) {

        collection->list = new_List( );

        if (collection->list) {
            collection->elems = 0;
            collection->unique_elems = 0;
        } else {
            ffree( fa_List, collection );
            wprintf(L"alloc Collection failed\n");
        }
    }

    return collection;
}

static inline
void
    delete_Collection (
        struct Collection * collection
)
{
    delete_List( collection->list );
    ffree( fa_List, collection );
}


static inline
int
    collect (
        struct Collection * collection,
        void *              key
)
{
    int             exists;
    struct Iter *   iter;
    struct Elem *   elem;

    iter = l_iterator( collection->list );

    for (elem = l_next( iter );
            (elem != NULL) && (key > elem->key);
                elem = l_next( iter ));

    l_done( iter );

    if ((elem == NULL) || (key != elem->key)) {

        exists = 0;

        elem = falloc( fa_Node, sizeof(struct Elem) );

        if (elem == NULL) {
            wprintf( L"alloc Elem failed\n" );
            return exists;
        }

        elem->key    = key;
        elem->count  = 1;

        l_insert( iter, elem );

        ++collection->unique_elems;


    } else {

        exists = 1;

        ++elem->count;

    }

    ++collection->elems;

    return exists;
}

static inline
int
    collect_n (
        struct Collection * collection,
        void *              key,
        int                 count
)
{
    int             exists;
    struct Iter *   iter;
    struct Elem *   elem;

    iter = l_iterator( collection->list );

    for (elem = l_next( iter );
            (elem != NULL) && (key > elem->key);
                elem = l_next( iter ));

    l_done( iter );

    if ((elem == NULL) || (key != elem->key)) {

        exists = 0;

        elem = falloc( fa_Node, sizeof(struct Elem) );

        if (elem == NULL) {
            wprintf( L"alloc Elem failed\n" );
            return exists;
        }

        elem->key    = key;
        elem->count  = count;

        l_insert( iter, elem );

        ++collection->unique_elems;

    } else {

        exists = 1;
        elem->count += count;

    }

    collection->elems += count;

    return exists;
}


#endif

