/*--- ck7mod11.c -------------------------- Listing 10-2 -------
 * Demonstrates weighted checksum using powers of 7 and
 * modulo 11
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DRIVER   1 /* generate a main line when #defined */
#define VERBOSE  1 /* print information about computation */

int Get7Mod11Chksum ( const char *data, int length )
{
    int  i, j;
    int  position;
    unsigned long int sum;

    static long int powers [10] = { 1, 7, 49, 343, 2401,
            16807, 117649, 823543, 5764801, 40353607 };

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

    /* get the sum modulus 11 */

    i = (int) ( sum % 11L );


    if ( i == 10 )
        i = 0;

#ifdef VERBOSE
    printf ( "Sum is %ld, checksum is %d\n", sum, i );
#endif

    return ( i );
}


#ifdef DRIVER
main ( int argc, char *argv[] )
{
    int i;

    if ( argc < 2 )
    {
        fprintf ( stderr, "Generates checksum using 7 mod 11.\n"
                 "Requires a command line # to validate.\n" );
        return ( EXIT_FAILURE );
    }

#ifdef VERBOSE
    printf ( "Computing 7 mod 11 checksum for: %s\n", argv[1] );
#endif

    i = Get7Mod11Chksum ( argv[1], strlen ( argv[1] ));

    if ( i == -1 )
        fprintf ( stderr,
                    "Number must be ten digits or less\n" );

    if ( i == -2 )
        fprintf ( stderr, "Number is invalid.\n" );

    return ( EXIT_SUCCESS );
}
#endif
