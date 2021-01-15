/*--- qstest.c ---------------------------- Listing 5-20 --------
 * Test compiler's qsort()
 * Link with sortsub.c (Listing 5-1)
 *
 * Has built-in driver
 *-------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "sorthdr.h"
#include "sortsub.h"

/*
 * A comparison function
 */
int pCfunc ( Element **L, Element **R )
{
    return ( strncmp ( (*L)->text, (*R)->text, 5 ));
}

void main ( int argc, char *argv[] )
{
    Element **Array;
    int Items = 2000;
    time_t a, b;

    if ( argc != 2 && argc != 3 )
    {
        fprintf ( stderr, "Usage: qstest infile [maxitems]\n" );
        return;
    }

    if ( argc == 3 )
        Items = atoi ( argv[2] );

    if (( Items = LoadArray ( argv[1], Items, &Array )) == -1 )
        return; /* Couldn't load file */

    time ( &a );
    qsort( Array, Items, sizeof(Element *),
          (int (*) (const void *, const void*))pCfunc );
    time ( &b );
    printf ( "Sorted %d items in %.0lf seconds.\n",
                Items, difftime ( b, a ));
}

