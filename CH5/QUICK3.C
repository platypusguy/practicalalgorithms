/*--- quick3.c ---------------------------- Listing 5-9 ---------
 * Quicksort with median-of-three partitioning,
 * insertion sort on small subfiles, and removal
 * of end recursion.
 *
 * Uses InsertionSort() from insert.c (Listing 5-5)
 *
 * #define DRIVER to compile a test driver
 * Driver must be linked to sortsub.c (Listing 5-1)
 *-------------------------------------------------------------*/

#include "sorthdr.h"

#define DRIVER 1

static CompFunc StoredCompare;
static Element **StoredArray;

static void xQuickSort3 ( int L, int R )
{
    while ( R - L >= 9 )  /* if there are at least 10 elements */
    {
        int i, j, mid;
        Element *temp;

        /*
         * Sort Lth, Rth, and middle element. Then swap the
         * middle element with the R-1'th element. This will
         * obviate the need for bound checking.
         */
        mid = ( L + R ) / 2; /* this is the middle element */

        if ( StoredCompare ( StoredArray[L],
                                StoredArray[mid] ) > 0 )
        {
            temp = StoredArray[L];
            StoredArray[L] = StoredArray[mid];
            StoredArray[mid] = temp;
        }
        if ( StoredCompare ( StoredArray[L],
                                StoredArray[R]) > 0 )
        {
            temp = StoredArray[L];
            StoredArray[L] = StoredArray[R];
            StoredArray[R] = temp;
        }
        if ( StoredCompare ( StoredArray[mid],
                                StoredArray[R]) > 0 )
        {
            temp = StoredArray[mid];
            StoredArray[mid] = StoredArray[R];
            StoredArray[R] = temp;
        }

        temp = StoredArray[mid];
        StoredArray[mid] = StoredArray[R-1];
        StoredArray[R-1] = temp;

        /*
         * Now, we know that Array[L] <= Array[R-1] <= Array[R].
         * We use Array[R-1] as the pivot, so this relationship
         * gives us known sentinels. Also, we need to partition
         * only between L+1 and R-2.
         */
        i = L;     /* Scan up from here   */
        j = R - 1; /* Scan down from here */
        for ( ;; )
        {
            /* Looking from left, find element >= Array[R-1] */
            while ( StoredCompare ( StoredArray[++i],
                                 StoredArray[R - 1] ) < 0 )
                            ;

            /* Looking from right, find element <= Array[R-1] */
            while ( StoredCompare ( StoredArray[--j],
                                 StoredArray[R - 1]) > 0 )
                            ;

            if ( i >= j )
                break;

            /* swap ith and jth elements */
            temp = StoredArray[i];
            StoredArray[i] = StoredArray[j];
            StoredArray[j] = temp;
        }


        /* swap ith and R-1'th elements */
        temp = StoredArray[i];
        StoredArray[i] = StoredArray[R - 1];
        StoredArray[R - 1] = temp;

        /*
         * This, and the conversion of the main loop from
         * "if (R - L >= 9)" to "while (R - L >= 9)" are the
         * only places we differ from quick2.c. These small
         * changes have a big effect: by recursing only on the
         * small half and simply looping on the large half
         * of each partition, we eliminate the possiblity
         * that worst-case input could cause us to make N
         * recursive calls. Instead, the worst case becomes
         * log2 N calls.
         */
        if ( i - L > R - i )  /* left half is larger */
        {
            xQuickSort3 ( i + 1, R ); /* recurse on small half */
            R = i - 1;
        }
        else  /* right half is larger */
        {
            xQuickSort3 ( L, i - 1 );
            L = i + 1;
        }
    }
}

void QuickSort3 ( Element **Array, int Items, CompFunc Compare )
{
    void InsertionSort ( Element **, int, CompFunc );

    /* Save some things */
    StoredCompare = Compare;
    StoredArray = Array;

    /* Quicksort to get nearly sorted file */
    xQuickSort3 ( 0, Items - 1 );

    /* Do an insertion sort on the now nearly sorted file */
    InsertionSort ( Array, Items, Compare );
}

#ifdef DRIVER
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
        fprintf( stderr, "Usage: quick3 infile [maxitems]\n" );
        return;
    }

    if ( argc == 3 )
        Items = atoi(argv[2]);

    if (( Items = LoadArray ( argv[1], Items, &Array )) == -1 )
        return; /* Couldn't load file */

    QuickSort3 ( Array, Items, (CompFunc) Cfunc );
    ShowArray ( Array, Items, (CompFunc) Cfunc );
}
#endif
