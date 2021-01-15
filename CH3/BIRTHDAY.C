/*--- birthday.c --------------------------- Listing 3-1 --------
 * Reads input file of birthdays and lists any duplicates.
 * Uses a simple hash table to identify the duplicates.
 *
 * Input records consists of lines of text of the form
 * MMDDName where MM = month, DD = day, Name. For example:
 *                  0212Abraham Lincoln
 * Note: for simplicity, no error checking is done on records.
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 366  /* Maximum days in a year */

FILE *fin;              /* Birthday file */
int  NameCount,         /* Number of names read */
     DupeCount;         /* Number of dupes found */

int hash_birthday ( char * );   /* Our hash function */

#ifdef __STDC__         /* in ANSI C, there's no strdup() */
/* the source code for this function is discussed in Ch. 2 */
char *strdup ( const char * );  /* string duplication */
#endif

int main ( int argc, char *argv[] )
{
    char buffer[128];   /* where the records will be read */
    int  hash_value;    /* the hash value we will compute */
    char *name;         /* pointer to birthday name       */

    char *Table[TABLE_SIZE];   /* table of birthday folks */
    int i;              /* subscript to init the table    */

    if ( argc < 2 )
    {
        fprintf ( stderr, "Error! Expecting birthday file\n" );
        exit ( EXIT_FAILURE );
    }

    if (( fin = fopen ( argv[1], "rt" )) == NULL )
    {
        fprintf ( stderr, "Error! Cannot open %s\n", argv[1] );
        exit ( EXIT_FAILURE );
    }

    for ( i = 0; i < TABLE_SIZE; i++ )
          Table[i] = NULL;

    while ( ! feof ( fin ))
    {
        if ( fgets ( buffer, 128, fin ) == NULL )
            break;

        /* get rid of the '\n' at end of record */
        buffer [strlen ( buffer ) - 1] = '\0';

        NameCount += 1;

        hash_value = hash_birthday ( buffer );
        name = strdup ( buffer + 4 );

        if ( Table[hash_value] == NULL )  /* No duplicate, */
        {                                 /* so add name.  */
            Table[hash_value] = name;
            continue;
        }
        else
        {                                 /* Is duplicate, */
            DupeCount += 1;               /* so tell 'em.  */
            printf ( "%s and %s have the same birthday.\n",
                    name, Table[hash_value] );
            continue;
        }
    }

    if ( DupeCount == 0 )
    {
        printf ( "No duplicate matches found among %d people.\n",
                  NameCount );
        if ( NameCount > 50 )
            printf ( "How rare!\n" );
    }
    else
        printf ( "Among %d people, %d matches were found\n",
                NameCount, DupeCount );

    return ( EXIT_SUCCESS );
}

/*---------------------------------------------------------------
 * A simple hash algorithm. It converts the month and day
 * to the number of the day in the year. Adds the day in the
 * month to the number of days elapsed before that month began.
 *-------------------------------------------------------------*/

int hash_birthday ( char *data )
{
    const int days_elapsed[12] =
      { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

    int mm, dd;
    char record [128];  /* where we store our copy of the data */

    strcpy ( record, data );  /* make our own copy */

    record [4] = '\0';  /* Mark the spot where the name begins */
    dd = atoi ( record + 2 );   /* Convert the day to an int */
    record [2] = '\0';  /* Mark off the spot where day begins */
    mm = atoi ( record );       /* Convert the month to an int */

    /* since mm is going to be an index into days_elapsed,
     * it must be checked for the correct range.
     */

    if ( mm < 1 || mm > 12 )
    {
        fprintf ( stderr, "Error in record for %s: %02d/%02d\n",
                 (char *) data + 4, mm, dd );
        exit ( EXIT_FAILURE );
    }

    return ( days_elapsed[mm - 1] + dd );
}

