/*--- lquick1.c --------------------------- Listing 5-17 --------
 * Quicksort for linked lists. Uses an insertion
 * sort on small subfiles and eliminates tail
 * recursion to minimize stack usage.
 *
 * Uses InsertionSortLink() from linsert.c (Listing 5-16)
 *
 * Adapted from an article by Jeff Taylor in
 * C Gazette, Vol 5, No. 6, 1991.
 *
 * #define DRIVER to compile a test driver
 * Driver must be linked to lsortsub.c (Listing 5-13)
 *-------------------------------------------------------------*/

#include "lsorthdr.h"

#define DRIVER 1

static CompFunc StoredCompare;
void InsertionSortLink ( Node **, Node *, CompFunc );

static void xQuickSortL1 ( Node **Head, Node *End )
{
    int left_count, right_count, count;
    Node **left_walk, *pivot, *old;
    Node **right_walk, *right;

    if ( *Head != End )
    do {
        pivot = *Head;       /* Take first element as pivot */
        left_walk = Head;    /* Set up left & right halves */
        right_walk = &right;
        left_count = right_count = 0;

        /* Now, walk the list */
        for ( old = (*Head)->next; old != End; old = old->next )
        {
            if ( StoredCompare ( old, pivot ) < 0 )
            {
                /* Less than pivot, so goes on left */
                left_count += 1;
                *left_walk = old;
                left_walk = &(old->next);
            }
            else
            {
                /* greater than or equal, so goes on right */
                right_count += 1;
                *right_walk = old;
                right_walk = &(old->next);
            }
        }

        /* Now glue the halves together... */
        *right_walk = End;   /* Terminate right list */
        *left_walk = pivot;  /* Put pivot after things on left */
        pivot->next = right; /* And right list after that */

        /* Now sort the halves in more detail */
        if ( left_count > right_count )
        {
            /*
             * Recursively sort (smaller) right half and then
             * reset local pointers so that when we loop, we
             * will see the left half as the entire list. Also,
             * if the right half has fewer than 10 elements,
             * sort it by insertion rather than by quicksort.
             */
            if ( right_count >= 9 )
                xQuickSortL1 ( &(pivot->next), End );
            else
                InsertionSortLink ( &(pivot->next), End,
                                    StoredCompare );
            End = pivot;
            count = left_count;
        }
        else
        {
            /* Converse case */
            if ( left_count >= 9 )
                xQuickSortL1 ( Head, pivot );
            else
                InsertionSortLink ( Head, pivot, StoredCompare );
            Head = &(pivot->next);
            count = right_count;
        }
    }
    while ( count > 1 );    /* end of do-while */
}

void QuickSortLink ( Node **Head, Node *End, CompFunc Compare )
{

    /* Save address of comparison function */
    StoredCompare = Compare;

    /* Quicksort the list */
    xQuickSortL1 ( Head, End );
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
        fprintf ( stderr, "Usage: lquick1 infile\n" );
        return;
    }

    if ( LoadList ( argv[1], &ListHead ) == -1 )
        return; /* Couldn't load file */

    QuickSortLink ( &ListHead, NULL, (CompFunc) Cfunc );
    ShowArray ( ListHead, (CompFunc) Cfunc );
}
#endif
