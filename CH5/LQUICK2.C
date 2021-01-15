/*--- lquick2.c --------------------------- Listing 5-18 --------
 * Quicksort for linked lists. Uses an insertion
 * sort on small subfiles, eliminates tail
 * recursion to minimize stack usage, and uses
 * a randomly selected pivot to avoid problems
 * with ordered input files.
 *
 * Uses InsertionSortLink() from linsert.c (Listing 5-16)
 *
 * Adapted from an article by Jeff Taylor in
 * C Gazette, Vol 5, No. 6, 1991.
 *
 * #define DRIVER to compile a test driver
 * Driver must be linked to lsortsub.c (Listing 5-13)
 *-------------------------------------------------------------*/

#include <stdlib.h>
#include "lsorthdr.h"

#define DRIVER 1

static CompFunc StoredCompare;
void InsertionSortLink ( Node **, Node *, CompFunc );

static void xQuickSortL2 ( Node **Head, Node *End, int N )
{
    int left_count, right_count, npivot;
    Node **left_walk, *pivot, *old;
    Node **right_walk, *right;

    while ( N > 1 )
    {
        if ( N <= 9 )  /* Insertion sort small lists */
        {
            InsertionSortLink ( Head, End, StoredCompare );
            break;
        }

        /* Select a pivot, but not at either end! */
        npivot = abs ( rand() ) % N;
        if ( npivot < 2 || npivot > N - 2 )
            npivot = 2;

        /* Run thru the list to the randomly selected point */
        old = *Head;
        while ( npivot-- )
            old = old->next;
        pivot = old->next;       /* Take as pivot */
        old->next = pivot->next; /* Cut from chain */

        /* Logic is now basically the same as lquick1.c */
        left_walk = Head;    /* Set up left & right halves */
        right_walk = &right;
        left_count = right_count = 0;

        /* Now walk the list */
        for ( old = *Head; old != End; old = old->next )
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
                /* Greater than or equal to, so goes on right */
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
             * will see the left half as the entire list,
             */
            xQuickSortL2 ( &(pivot->next), End, right_count );
            End = pivot;
            N = left_count;
        }
        else
        {
            /* Converse case */
            xQuickSortL2 ( Head, pivot, left_count );
            Head = &(pivot->next);
            N = right_count;
        }
    }
}

void QuickSortLink ( Node **Head, Node *End, CompFunc Compare )
{
    Node *walk;
    int count = 0;
    /* Save address of comparison function */
    StoredCompare = Compare;

    /* Count the list */
    for ( walk = *Head; walk != End; walk = walk->next )
        count += 1;

    /* Quicksort the list */
    xQuickSortL2 ( Head, End, count );
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
        fprintf ( stderr, "Usage: lquick2 infile\n" );
        return;
    }

    if ( LoadList ( argv[1], &ListHead ) == -1 )
        return; /* Couldn't load file */

    QuickSortLink ( &ListHead, NULL, (CompFunc) Cfunc );
    ShowArray ( ListHead, (CompFunc) Cfunc );
}
#endif
