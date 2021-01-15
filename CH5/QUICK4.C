/*--- quick4.c ---------------------------- Listing 5-10 --------
 * Quicksort with median-of-three partitioning
 * based on a randomly selected middle element,
 * insertion sort on small subfiles, removal of
 * end recursion, and use of pointer
 * incrementing rather than an index.
 *
 * Uses InsertionSort() from insert.c (Listing 5-5)
 *
 * #define DRIVER to compile a test driver
 * Driver must be linked to sortsub.c (Listing 5-1)
 *-------------------------------------------------------------*/

#include <stddef.h>     /* for typedef of ptrdiff_t */
#include <stdlib.h>     /* for rand() */
#include "sorthdr.h"

#define DRIVER 1

static CompFunc StoredCompare;

static void xQuickSort4 ( Element **pL, Element **pR )
{
    ptrdiff_t diff; /* ptrdiff_t is a signed type that can hold
                       the difference between two pointers */

    while (( diff = ( pR - pL )) >= 9 )  /* 10 elements a must */
    {
        int mid;
        Element *temp, **pmid, **pi, **pj, *ppivot;

        /* select a random mid element */
        mid = abs ( rand() ) % diff;
        if ( mid < 1 || mid > diff - 2 )
            mid = 1;
        pmid = pL + mid;

        /*
         * Sort Lth, Rth, and middle element. Then swap the
         * middle element with the R-1'th element. This will
         * obviate the need for bound checking.
         */
        if ( StoredCompare ( *pL, *pmid ) > 0 )
        {
            temp = *pL;
            *pL = *pmid;
            *pmid = temp;
        }
        if ( StoredCompare ( *pL, *pR ) > 0 )
        {
            temp = *pL;
            *pL = *pR;
            *pR = temp;
        }
        if ( StoredCompare ( *pmid, *pR ) > 0 )
        {
            temp = *pmid;
            *pmid = *pR;
            *pR = temp;
        }

        temp = *pmid;
        *pmid = *(pR-1);
        *(pR-1) = temp;

        /*
         * Now, we know that Array[L] <= Array[R-1] <= Array[R].
         * We use Array[R-1] as the pivot, so this relationship
         * gives us known sentinels. Also, we need to partition
         * only between L+1 and R-2.
         */
        pi = pL;       /* Scan up from here */
        pj = pR - 1;   /* Scan down from here */
        ppivot = *pj;
        for ( ;; )
        {
            /* Looking from left, find element >= Array[R-1] */
            while ( StoredCompare ( *++pi, ppivot ) < 0 )
                            ;

            /* Looking from right, find element <= Array[R-1] */
            while ( StoredCompare ( *--pj, ppivot ) > 0 )
                            ;

            if ( pi >= pj )
                break;

            /* swap ith and jth elements */
            temp = *pi;
            *pi = *pj;
            *pj = temp;
        }


        /* swap ith and the pivot */
        *(pR - 1) = *pi;
        *pi = ppivot;

        if ( pi - pL > pR - pi )  /* left half is larger */
        {
            xQuickSort4 ( pi+1, pR ); /* recurse on smaller half */
            pR = pi - 1;
        }
        else    /* right half is larger */
        {
            xQuickSort4 ( pL, pi-1 );
            pL = pi + 1;
        }
    }
}

void QuickSort4 ( Element **Array, int Items, CompFunc Compare )
{
    void InsertionSort ( Element **, int, CompFunc );

    /* Save some things */
    StoredCompare = Compare;

    /* Quicksort to get nearly sorted file */
    xQuickSort4 ( Array, Array + Items - 1 );

    /* Do an insertion sort on the now nearly sorted file */
    InsertionSort ( Array, Items, Compare );
}

#ifdef DRIVER
#include <stdio.h>
#include <string.h>
#include "sortsub.h"

/*
 * A comparison function
 */
int Cfunc ( Element *L, Element *R )
{
    return ( strncmp ( L->text, R->text, 5 ));
}

void main ( int argc, char *argv[] )
{
    Element **Array;
    int Items = 2000;

    if ( argc != 2 && argc != 3 )
    {
        fprintf ( stderr, "Usage: quick4 infile [maxitems]\n" );
        return;
    }

    if ( argc == 3 )
        Items = atoi ( argv[2] );

    if (( Items = LoadArray ( argv[1], Items, &Array )) == -1 )
        return; /* Couldn't load file */

    QuickSort4 ( Array, Items, (CompFunc) Cfunc );
    ShowArray ( Array, Items, (CompFunc) Cfunc );
}
#endif
