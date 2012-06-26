#include "falloc.h"
#include "list.h"

FastAlloc   fa_List;

void
    list_init (
        void
)
{
    fa = new_falloc( L"List",
            (sizeof(struct Head) > sizeof(struct Node)) ?
                sizeof(struct Head) :
                    sizeof(struct Node) );
}

void
    list_cleanup (
        void
)
{
    delete_falloc( fa );
}

struct Node *
    new_Node (
        void
)
{
    struct Node * link = falloc( fa, sizeof(struct Node) );

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
        head->head = NULL;
        head->count = 0;
    }

    return head;
}


