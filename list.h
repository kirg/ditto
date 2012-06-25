#ifndef _LIST_H_
#define _LIST_H_

#include "falloc.h"

struct Head {
    struct Link *   first;
    int             count;
};

struct Link {
    struct Link *   next;
    void *          data;
};

struct Link *
    new_Link (
        void
);


struct Head *
    new_Head (
        void
);


void
    add_list (
        struct Head *   list,
        void *          data
);

static inline
void
    push (
        struct Head *   list,
        struct Link *   link
)
{
    link->next      = list->first;
    list->first     = link;

    ++list->count;
}

#endif
