/*--- cksmtest.c -------------------------- Listing 10-3 -------
 *  Test checksum methods to see the distribution of checksums
 *  when transpositions occur. Performs the testing on 100
 *  random 10-digit numbers all the way to 100 2-digit
 *  numbers. Benign transpositions (see text) are not included.
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char table [100] [10]; /* the table of random digits */
                       /* max: 10 numbers of 10 digits each */

void DisplayUsage ( void );
void LoadTableRandom ( char * );
int  GetChecksum ( char *, int, int );

int main( int argc, char *argv[] )
{
    int  i, j, size, method;
    char *p, *pmax;
    char buffer[10];

    p = (char *) table;     /* start of table */
    pmax = p + 1000;        /* end of table */

    if ( argc != 2 )
    {
        DisplayUsage();
        return ( EXIT_FAILURE );
    }
    else
        method = atoi ( argv[1] );

    switch ( method )
    {
        case 1: printf ( "Using 7 mod 11\n\n" );    break;
        case 2: printf ( "Using 9 mod 11\n\n" );    break;
        case 3: printf ( "Using 7 mod 26\n\n" );    break;
        case 4: printf ( "Using 7 mod 10\n\n" );    break;
        case 5: printf ( "Using 9 mod 10\n\n" );    break;
        default:
                DisplayUsage();
                return ( EXIT_FAILURE );
    }

    /* we loop through for every size from 10 to 2.
     * We don't handle 1-digit numbers since they
     * offer no possibility of transpositions.
     */

    for ( size = 10; size > 1; size-- )
    {
        int collisions = 0;

        /* Load the table with random digits */

        LoadTableRandom ( pmax );

        /* main test loop: we process 100 numbers
         * of length size. For each number, we do every
         * possible transposition and get its checksum.
         */

        for ( i = 0, p = (char *)table; i < 100;
                                           i++, p += size )
        {
            int k, l, orig_cksum;

            /* get the original number's checksum */

            orig_cksum = GetChecksum ( p, size, method );

            /* now copy the number to buffer, where we'll
             * do the transpositions.
             */

            memcpy ( buffer, p, size );

            /* do the transpositions and checksums */

            for ( k = 0; k < size - 1; k++ )
            {
                /* is there a transposition possible?
                 * if both digits are the same, they cannot
                 * be transposed; so skip these, otherwise
                 * they will generate bogus collisions.
                 */

                if ( buffer[k] == buffer[k + 1] )
                    continue;

                /* otherwise, do the transposition */

                l = buffer[k + 1];
                buffer[k + 1] = buffer[k];
                buffer[k] = (char) l;

                /* get the checksum */

                j = GetChecksum ( buffer, size, method );

                if ( j == orig_cksum )
                    collisions += 1;

                /* now undo this transposition */

                l = buffer[k + 1];
                buffer[k + 1] = buffer[k];
                buffer[k] = (char) l;
            }

        }   /* end of loop processing numbers of one size */

        printf ( "%2d digits avg. %1.2f collisions\n",
                    size, (float) ( collisions / 100.0 ));

        collisions = 0;
    }

    return ( EXIT_SUCCESS );
}

void LoadTableRandom ( char *table_end )
{
    int i;
    unsigned short int j;
    char *p, *p2;

    /* 200 random ints will get us close to 1000 digits
     * we presume it gets us at least 500 digits, which is
     * a safe bet, but will not always be the case.
     */

    p = (char *) table;

    for ( i = 0; i < 200; i++ )
    {
        j = (short int) rand();
        sprintf ( p, "%u", j );
        p += strlen ( p );
    }

    /* how many digits left to fill out? */

    i = table_end - p;

    p2 = p - 1;

    /* go backwards from p and copy digits into the
     * remaining table space. This will copy i digits
     * prior to p and place them in reverse order after p.
     */
    while ( i )
    {
        *p++ = *p2--;
        i--;
    }
}

int GetChecksum ( char *data, int length, int which_test )
{
    int  i, j;
    int  position;
    unsigned long int sum;
    long int *powers;

    static long int powers_of_7 [10] = { 1, 7, 49, 343, 2401,
            16807, 117649, 823543, 5764801, 40353607 };

    static long int powers_of_9 [10] = { 1, 9, 81, 729, 6561,
            59049, 531441, 4782969, 43046721, 387420489 };

    if ( which_test == 2 || which_test == 5 )
        powers = powers_of_9;
    else
        powers = powers_of_7;

    /* make sure all digits can be looked up */

    if ( length > 10 )
        return ( -1 );

    sum = 0L;
    position = 0; /* what digit in the data are we at? */

    for ( i = length; i > 0; i--, position++ )
    {
        /* do we have a digit ? */

        if ( ! isdigit ( data[i-1] ))
            return ( -2 );

        /* convert digit character to a number */

        j = data[i-1] - '0';

        /* look up power, multiply by j, add to sum */

        sum += powers [position] * j;
    }

    /* get the sum modulus depending on method */

    switch ( which_test )
    {
        case 1:
        case 2:
            i = (int) ( sum % 11L );
            if ( i == 10 )
                i = 0;
            break;
        case 3:
            i = (int) ( sum % 26L );    break;
        case 4:
        case 5:
            i = (int) ( sum % 10L );    break;
        default:
            DisplayUsage();
            exit ( EXIT_FAILURE );
    }

    return ( i );
}

void DisplayUsage()
{
    fprintf ( stderr,
        "Error: need to specify checksum method to test\n"
        "1. Powers of 7 Mod 11\n"
        "2. Powers of 9 Mod 11\n"
        "3. Powers of 7 Mod 26\n"
        "4. Powers of 7 Mod 10\n"
        "5. Powers of 9 Mod 10\n" );
}
