/*--- llgen.c ------------------------------ Listing 2-3 --------
 *  Generic primitive functions for doubly linked lists.
 *  Contains no application-specific functions.
 *  Functions are in alphabetical order.
 *------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#define IN_LL_LIB   1   /* in the library of primitives */

#include "llgen.h"

/*--- Aliases to make the code more readable ---*/

#define LLHead (L->LHead)      /* The head of the current list */
#define LLTail (L->LTail)      /* The tail of the current list */
#define NodeCount (L->LCount)  /* Nodes in the current list */

#define CreateData     (*(L->LCreateData))
#define DeleteData     (*(L->LDeleteData))
#define DuplicatedNode (*(L->LDuplicatedNode))
#define NodeDataCmp    (*(L->LNodeDataCmp))

/*----------------------------------------------------
 * Add a node at head: first allocate the space for
 * the data, then allocate a node with a pointer to
 * the data, then add the node to the list.
 *--------------------------------------------------*/
int AddNodeAtHead ( struct List *L, void *nd )
{
    Link pn;

    pn = CreateNode ( L, nd );
    if ( pn == NULL )
        return ( 0 );

    /*--- Add the node ---*/
    if ( LLHead == NULL )   /* is it the first node? */
    {
        LLHead = LLTail = pn; /*--- yes ---*/
    }
    else                      /*--- no  ---*/
    {
        LLHead->prev = pn; /* first goes node before Head */
        pn->next = LLHead; /* put Head next */
        LLHead = pn;       /* then point Head to us */
    }

    NodeCount += 1;
    return ( 1 );
}

/*----------------------------------------------------
 * Add ascending. Adds a node to an ordered list.
 *--------------------------------------------------*/
int AddNodeAscend ( struct List *L, void *nd )
{
    Link        pn;         /* to node we're creating */
    Link        prev, curr; /* our current search */
    struct Node dummy;      /* a dummy node */
    int         compare;

    pn = CreateNode ( L, nd );
    if ( pn == NULL )
        return ( 0 );

    /* attach dummy node to head of list */
    dummy.next = LLHead;
    dummy.prev = NULL;
    if ( dummy.next != NULL )
        dummy.next->prev = &dummy;

    prev = &dummy;
    curr = dummy.next;
    for ( ; curr != NULL; prev = curr, curr = curr->next )
    {
        compare = NodeDataCmp ( pn->pdata, curr->pdata );
        if ( compare <= 0 )
            break; /* new node equals or precedes curr */
    }

    if ( curr != NULL && compare == 0 )
    {
        compare = DuplicatedNode ( pn, curr );
        if ( compare == 2 )
            /* do nothing -- will get inserted */;
        else
        {
            /* first, repair the linked list */
            LLHead = dummy.next;
            LLHead->prev = NULL;

            /* delete the duplicated node, if appropriate */
            if ( compare == 1 )
            {
                DeleteData( pn->pdata );
                free ( pn );
            }
            return ( 1 );
        }
    }

    prev->next = pn;
    pn->prev = prev;
    pn->next = curr;
    if ( curr != NULL )
        curr->prev = pn;
    else
        LLTail = pn; /* this node is the new tail */

    NodeCount += 1;

    /* now, unhook the dummy head node */
    LLHead = dummy.next;
    LLHead->prev = NULL;
    return ( 1 );
}

/*---------------------------------------------------------------
 * Creates a linked-list structure and returns a pointer to it.
 * On error, returns NULL. This functions accepts pointers
 * to the four list-specific functions and initializes the
 * linked-list structure with them.
 *-------------------------------------------------------------*/
struct List * CreateLList (
                void * ( * fCreateData ) ( void * ),
                int    ( * fDeleteData ) ( void * ),
                int    ( * fDuplicatedNode ) ( Link, Link ),
                int    ( * fNodeDataCmp )  ( void *, void * ))
{
    struct List * pL;

    pL = (struct List *) malloc ( sizeof ( struct List ));
    if ( pL == NULL )
        return NULL;

    pL->LHead = NULL;
    pL->LTail = NULL;
    pL->LCount = 0;

    pL->LCreateData = fCreateData;
    pL->LDeleteData = fDeleteData;
    pL->LDuplicatedNode = fDuplicatedNode;
    pL->LNodeDataCmp = fNodeDataCmp;

    return ( pL );
}

/*---------------------------------------------------------------
 * Creates a node and then calls the application-specific
 * function CreateData() to create the node's data structure.
 * Returns NULL on error.
 *-------------------------------------------------------------*/
Link CreateNode ( struct List *L, void *data )
{
    Link new_node;

    new_node = (Link) malloc ( sizeof ( struct Node ));
    if ( new_node == NULL )
        return ( NULL );

    new_node->prev = NULL;
    new_node->next = NULL;

    /*--- now call the application-specific data allocation ---*/
    new_node->pdata = CreateData( data );
    if ( new_node->pdata == NULL )
    {
        free ( new_node );
        return ( NULL );
    }
    else
        return ( new_node );
}

/*---------------------------------------------------------------
 *  Deletes the node pointed to by to_delete.
 *  Function calls list-specific function to delete data.
 *-------------------------------------------------------------*/
int DeleteNode ( struct List *L, Link to_delete )
{
    Link pn;

    if ( to_delete == NULL )        /* Double check before */
        return ( 0 );               /* deleting anything.  */

    if ( to_delete->prev == NULL )  /* we're at the head */
    {
        LLHead = to_delete->next;   /* update head */
        LLHead->prev = NULL;        /* update next node */
    }

    else if ( to_delete->next == NULL )
    {                               /* we're at the tail */
        pn = to_delete->prev;       /* get the previous node */
        pn->next = NULL;
        LLTail = pn;                /* update tail */
    }

    else                        /* we're in the list */
    {
        pn = to_delete->prev;       /* get the previous node */
        pn->next = to_delete->next; /* update previous node to */
                                    /* point to the next one. */
        pn = to_delete->next;       /* get the next node */
        pn->prev = to_delete->prev; /* update it to point to */
                                    /* the previous one. */
    }

    DeleteData ( to_delete->pdata );  /* delete the data */
    free ( to_delete );               /* free the node */

    NodeCount -= 1;

    return ( 1 );
}

/*---------------------------------------------------------------
 *  Finds node by starting at the head of the list, stepping
 *  through each node, and comparing data items with the search
 *  key. The Ascend version checks that the data in the node
 *  being examined is not larger than the search key. If it is,
 *  we know the key is not in the list. Returns pointer to node
 *  on success or NULL on failure.
 *-------------------------------------------------------------*/
Link FindNode ( struct List *L, void *nd )
{
    Link pcurr;             /* the node we're examining */

    if ( LLHead == NULL )        /* empty list */
        return ( NULL );

    for ( pcurr = LLHead; pcurr != NULL; pcurr = pcurr->next)
    {
        if ( NodeDataCmp ( nd, pcurr->pdata ) == 0 )
            return ( pcurr );
    }
    return ( NULL );             /* could not find node */
}

Link FindNodeAscend ( struct List *L, void *nd )
{
    Link    pcurr;          /* the node we're examining */
    int cmp_result;

    if ( LLHead == NULL )        /* empty list */
        return ( NULL );

    for ( pcurr = LLHead; pcurr != NULL; pcurr = pcurr->next)
    {
        cmp_result =  NodeDataCmp ( nd, pcurr->pdata );

        if ( cmp_result < 0 )
             return ( NULL );    /* too far */

        if ( cmp_result == 0 )   /* just right */
             return ( pcurr );
    }

    return ( NULL );             /* could not find node */
}

/*---------------------------------------------------------------
 *  The Goto functions return the pointer to the requested node
 *  or NULL on error.
 *-------------------------------------------------------------*/

Link GotoNext ( struct List *L, Link pcurr )
{
    if ( pcurr->next == NULL || pcurr == LLTail )
        return ( NULL );
    else
        return ( pcurr->next );
}

Link GotoPrev ( struct List *L, Link pcurr )
{
    if ( pcurr->prev == NULL || pcurr == LLHead )
        return ( NULL );
    else
        return ( pcurr->prev );
}
