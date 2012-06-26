#ifndef _LIST_H_
#define _LIST_H_

#include "falloc.h"

struct List {
    struct Node *   head;
    struct Node *   tail;
    int             count;
};


struct Node {
    struct Node *   next;
    void *          data;
};


struct Iterator {
    struct Node *   next;
};



extern FastAlloc fa_List;

struct Node *
    new_Node (
        void
);

struct List *
    new_List (
        void
);

struct List *
    new_List (
        void
);



void
    insert_list (
        struct List *   list,
        void *          data
);

Node *
    remove_list (
        struct List *   list
);

void
    insert_tail (
        struct List *   list,
        void *          data
);



static inline
struct Iterator *
    iterator (
        struct List *   list
)
{
    struct Iterator * iter = new_Iterator( );

    if (iter != NULL) {
        iter->next  = list->head;
    }
}


Node *
    get (
        struct Iterator *   iter
)
{
    Node * next;

    if (iter != NULL) {
        next        = iter->next;
        iter->next  = next->next;
    }
}



static inline
void
    push (
        struct List *   list,
        void *          data
)
{
    struct Head * head = falloc( fa, sizeof(struct Head) );

    node = new_Node( );

    if (!node) {

        node->data  = data;

        node->next  = list->head;

        list->head  = node;
        ++list->count;
    }
}

static inline
void *
    pop (
        struct List *   list
)
{
    Node *  head;
    void *  data = NULL;

    head    = list->head;
    data    = NULL;

    if (head != NULL) {

        data    = head->data;

        list->head  = head->next;
        --list->count;

        ffree( head );
    }

    return data;
}

#endif

