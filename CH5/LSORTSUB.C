/*--- lsortsub.c -------------------------- Listing 5-13 --------
 * I/O subroutines for sorting routines
 * Loads data into an array
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lsorthdr.h"
#include "lsortsub.h"

#define MAX_ITEM_SIZE 500 /* max length of a text item */

/*
 * Loads text strings from FileName into a linked list. Returns
 * the number of items in the list of -1 for failure.
 */
int LoadList ( char *FileName, Node **ListHead )
{
    FILE *infile;
    char buffer[MAX_ITEM_SIZE], *s;
    int i;
    Node **current;

    if (( infile = fopen ( FileName, "r" )) == NULL )
    {
        fprintf ( stderr, "Can't open file %s\n", FileName);
        return ( -1 );
    }

    i = 0;
    current = ListHead;
    while ( fgets ( buffer, MAX_ITEM_SIZE, infile ))
    {
        /* trim trailing control characters */
        s = buffer + strlen ( buffer );
        while ( iscntrl ( *s ))
            *s-- = 0;

        /* make space and store it */
        *current = malloc ( sizeof (Node) );
        if ( *current == NULL )
        {
            fprintf ( stderr, "Can't get memory for data\n" );
            return ( -1 );
        }

        (*current)->text = malloc ( strlen ( buffer ) + 1 );
        if ( (*current)->text == NULL )
        {
            fprintf ( stderr, "Can't get memory for data\n" );
            return ( -1 );
        }
        strcpy ( (*current)->text, buffer );

        current = &( (*current)->next ); /* advance */
        i++; /* keep count */
    }
    *current = NULL; /* terminate the list */

    /*
     * If the linked list contains only one element, and that
     * element is an empty string, return an empty list.
     */
    if ( *ListHead != NULL &&
        (*ListHead)->next == NULL &&
        (*ListHead)->text[0] == 0 )
    {

        *ListHead = NULL;
        i = 0;
    }

    fclose ( infile );
    return ( i );
}

/* Display array of items */
void ShowArray ( Node *ListHead, CompFunc Compare )
{
    int i = 0, sorted = 1, column = 1;

    for ( ; ListHead != NULL; ListHead = ListHead->next )
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
        printf ( "%3d: %s", i++, ListHead->text );
        column += 5 + strlen ( ListHead->text );
        if ( ListHead->next )
        {
            if ( Compare ( ListHead, ListHead->next ) > 0 )
                sorted = 0;
        }
    }

    if ( sorted )
        printf ( "\n\nThe linked list is sorted.\n" );
    else
        printf ( "\n\nThe linked list is not sorted.\n" );
}
