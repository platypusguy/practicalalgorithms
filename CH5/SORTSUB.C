/*--- sortsub.c --------------------------- Listing 5-1 ---------
 * I/O subroutines for sorting routines
 *
 * Note: Loads data into an array
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sorthdr.h"
#include "sortsub.h"

#define MAX_ITEM_SIZE 500 /* maximum length of a text item */

/*--------------------------------------------------------------
 * Load up to MaxItems text strings from FileName into an array
 * of pointers to the text strings. Returns number of items
 * in array or -1 for failure.
 *------------------------------------------------------------*/

int LoadArray ( char *FileName,
                int MaxItems,
                Element ***Array)
{
    FILE *infile;
    char buffer[MAX_ITEM_SIZE], *s;
    int i;

    if (( infile = fopen ( FileName, "r" )) == NULL )
    {
        fprintf ( stderr, "Can't open file %s\n", FileName );
        return  ( -1 );
    }

    *Array =
        (Element **) malloc ( sizeof(Element **) * MaxItems );

    if ( *Array == NULL )
    {
        fprintf ( stderr, "Can't allocate array of pointers\n" );
        return ( -1 );
    }

    i = 0;
    while ( fgets ( buffer, MAX_ITEM_SIZE, infile ))
    {
        if ( i >= MaxItems )  /* limit on # of items */
        {
            printf ( "Entire data file not loaded\n" );
            break;
        }

        /* trim trailing control characters */
        s = buffer + strlen ( buffer );
        while( iscntrl ( *s ))
            *s-- = 0;

        /* make space and store it */
        ( *Array )[i] = malloc ( sizeof ( Element ));
        if (( *Array )[i] == NULL )
        {
            fprintf ( stderr, "Can't get memory for data\n" );
            return ( -1 );
        }

        ( *Array )[i]->text = malloc ( strlen ( buffer ) + 1 );
        if (( *Array )[i]->text == NULL )
        {
            fprintf ( stderr, "Can't get memory for data\n" );
            return ( -1 );
        }
        strcpy (( *Array )[i++]->text, buffer );
    }

    /*
     * Special case: if the array contains only 1 item, and
     * the item is an empty string, return an empty array.
     */
    if ( i == 1 && *(( *Array )[0]->text ) == 0 )
        i = 0;

    fclose ( infile );
    return ( i );
}

/*--- Display array of items ---*/
void ShowArray ( Element **Array, int Items, CompFunc Compare )
{
    int i, sorted = 1, column = 1;

    for ( i = 0; i < Items; i++ )
    {
        if ( column > 61 )
        {
            printf ( "\n" );
            column = 1;
        }
        else  while (( column - 1 ) % 20 )
        {
            printf ( " " );
            column += 1;
        }
        printf ( "%3d: %s", i, Array[i]->text );
        column += 5 + strlen ( Array[i]->text );

        if ( i > 0 )
        {
            if ( Compare(Array[i-1], Array[i]) > 0 )
                sorted = 0;
        }
    }

    if ( sorted )
        printf ( "\n\nThe array is sorted.\n" );
    else
        printf ( "\n\nThe array is not sorted.\n" );
}

