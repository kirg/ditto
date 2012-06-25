#include "falloc.h"
#include "list.h"

FastAlloc   fa;

void
    list_init (
        void
)
{
    fa = new_FastAllocator( L"List",
            (sizeof(struct Head) > sizeof(struct Link)) ?
                sizeof(struct Head) :
                    sizeof(struct Link) );
}

void
    list_cleanup (
        void
)
{
    free_FastAllocator( fa );
}

struct Link *
    new_Link (
        void
)
{
    struct Link * link = falloc( fa, sizeof(struct Link) );

    if (link) {
        link->next = 0;
        link->data = NULL;
    }

    return link;
}

struct Head *
    new_Head (
        void
)
{
    struct Head * head = falloc( fa, sizeof(struct Head) );

    if (head) {
        head->first = NULL;
        head->count = 0;
    }

    return head;
}


void
    add_list (
        struct Head *   list,
        void *          data
)
{
    struct Link *   link;

    link = new_Link( );

    link->data      = data;

    link->next      = list->first;
    list->first     = link;

    ++list->count;
}


