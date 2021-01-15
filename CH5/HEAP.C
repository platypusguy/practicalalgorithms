/*--- heap.c ------------------------------ Listing 5-12 --------
 * Heapsort on an array. Uses basic heapsort
 * algorithm, with Floyd's modification to
 * reduce order from 2N lg N to N lg N.
 *
 * #define DRIVER to compile a test driver
 * Driver must be linked to sortsub.c (Listing 5-1)
 *-------------------------------------------------------------*/

#include "sorthdr.h"

#define DRIVER 1

static CompFunc StoredCompare;
static Element **StoredArray;

/* Enforce heap condition between first and last */
static void downheap ( int first, int last )
{
    int child, parent, i;
    Element *temp;
    for ( i = first; ( child = i * 2 + 1 ) <= last; i = child )
    {
        if ( child + 1 <= last &&
            StoredCompare ( StoredArray[child + 1],
                            StoredArray[child] ) > 0 )
            child += 1;

        /* child is the larger child of i */
        temp = StoredArray[i];
        StoredArray[i] = StoredArray[child];
        StoredArray[child] = temp;
    }

    while ( 1 )
    {
        parent = ( i - 1 ) / 2;

        if ( parent < first || parent == i ||
            StoredCompare ( StoredArray[parent],
                            StoredArray[i] ) > 0 )
            break;
        temp = StoredArray[i];
        StoredArray[i] = StoredArray[parent];
        StoredArray[parent] = temp;

        i = parent;
    }
}

void HeapSort ( Element **Array, int N, CompFunc Compare )
{
    int i;
    Element *temp;

    /* Make array and compare function available to all */
    StoredCompare = Compare;
    StoredArray = Array;

    /* Make N equal to largest index */
    N -= 1;

    /* First, ensure heap property for array */
    for ( i = ( N - 1 ) / 2; i >= 0; i-- )
        downheap ( i, N );

    /*
     * Now sort by taking advantage of the fact that the
     * largest element is in Array[0]. If we remove this element
     * and move it to Array[N-1], that element is now in place.
     * We continue to sort by reenforcing the heap property on
     * Array[0 .. N-2] and then taking the next largest element
     * and moving it to Array[N-2]. By repeating this process,
     * we will ultimately sort the array.
     */
    for ( i = N; i > 0; )
    {
        temp = Array[i];
        Array[i] = Array[0];
        Array[0] = temp;
        downheap ( 0, --i );
    }
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
        fprintf ( stderr, "Usage: heap infile [maxitems]\n" );
        return;
    }

    if ( argc == 3 )
        Items = atoi ( argv[2] );

    if (( Items = LoadArray ( argv[1], Items, &Array )) == -1 )
        return; /* Couldn't load file */

    HeapSort ( Array, Items, (CompFunc) Cfunc );
    ShowArray ( Array, Items, (CompFunc) Cfunc );
}
#endif
