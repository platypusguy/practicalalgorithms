/*--- intelchk.c -------------------------- Listing 10-1 -------
 * A small demonstration of an Intel checksum being generated
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERBOSE 1 /* prints the results to the screen */
#define DRIVER  1 /* do the mainline */


char IntelCheckSum ( const char *data, int length )
{
	char sum;
	int i;

    for ( i = 0, sum = 0; i < length; i++ )
		sum += data[i];

#if VERBOSE
    printf ( "string: %s\nsum: %d\n", data, sum );
#endif

    return ( -sum );
}

#if DRIVER
main ( int argc, char *argv[] )
{
    int cksum;

    if ( argc < 2 )
    {
        fprintf ( stderr,
            "Computes Intel checksum on command line string.\n"
            "Usage: intelchk string\n" );
        return (EXIT_FAILURE );
    }

    cksum = IntelCheckSum ( argv[1], strlen ( argv[1] ));

    printf ( "checksum: %d\n", cksum );
    return ( EXIT_SUCCESS );
}
#endif
