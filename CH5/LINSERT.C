/*--- linsert.c --------------------------- Listing 5-16 --------
 * Insertion sort of a linked list
 *
 * #define DRIVER to compile a test driver
 * Driver must be linked to lsortsub.c (Listing 5-13)
 *-------------------------------------------------------------*/

#include <stdlib.h>
#include "lsorthdr.h"

/* #define DRIVER 1 */

/* Sort from *ListHead to ListEnd */
void InsertionSortLink ( Node **ListHead, Node *ListEnd,
                         CompFunc Compare )
{
    Node *newlist;  /* we build up the new list here */
    Node *walk, *save;

    newlist = ListEnd;  /* new list will end with this pointer */
    walk = *ListHead;   /* get ready to walk the list */

    for ( ; walk != ListEnd; walk = save )
    {
        /*
         * Run thru newlist, looking for an element that is
         * greater than walk. Watch the value in pnewlink
         * carefully--it points to the 'next' element of
         * of the structure that points to the structure
         * currently under scrutiny. Thus, pnewlist is
         * the equivalent of the 'last' pointer that is
         * required by linked-list insertion algorithms.
         */
        Node **pnewlink;
        for ( pnewlink = &newlist;
             *pnewlink != ListEnd &&
               Compare(walk, *pnewlink ) >= 0;
              pnewlink = &((*pnewlink)->next) )
                    ;

        save = walk->next;      /* save this */
        walk->next = *pnewlink; /* and link in */
        *pnewlink = walk;
    }

    *ListHead = newlist;
}

#ifdef DRIVER
#include <stdio.h>
#include <string.h>
#include "lsortsub.h"

/*
 * A comparison function
 */
int Cfunc ( Node *L, Node *R )
{
    return ( strncmp ( L->text, R->text, 5 ));
}


void main ( int argc, char *argv[] )
{
    Node *ListHead;

    if ( argc != 2 )
    {
        fprintf ( stderr, "Usage: linsert infile\n" );
        return;
    }

    if ( LoadList ( argv[1], &ListHead ) == -1 )
        return; /* Couldn't load file */

    InsertionSortLink ( &ListHead, NULL, (CompFunc) Cfunc );
    ShowArray ( ListHead, (CompFunc) Cfunc );
}
#endif
