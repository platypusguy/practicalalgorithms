/*--- crc16.c ----------------------------- Listing 10-7 -------
*   Performs CRC-16 check on a file.
*   Usage: crc16 finename
*
*   Modeled on William Hunt's work and Nigel Cort's port.
*-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#define DRIVER 1    /* brings in the main line */


/* Compute a CRC-16 value */

unsigned short int GetCRC16 (
            int start,      /* starting value */
            const char *p,  /* points to chars to process */
            int n )         /* how many chars to process */
{

    static unsigned int crc16_table[16] =   /* CRC-16s */
    {
          0x0000, 0xCC01, 0xD801, 0x1400,
          0xF001, 0x3C00, 0x2800, 0xE401,
          0xA001, 0x6C00, 0x7800, 0xB401,
          0x5000, 0x9C01, 0x8801, 0x4400
    } ;

    unsigned short int total;  /* the CRC-16 value we compute */
    int r1;

    total = start;

    /* process each byte */

    while ( n-- > 0 )
    {
        /* do the lower four bits */

        r1 = crc16_table[ total & 0xF ];
        total = ( total >> 4 ) & 0x0FFF;
        total = total ^ r1 ^ crc16_table[ *p & 0xF ];

        /* do the upper four bits */

        r1 = crc16_table[ total & 0xF ];
        total = ( total >> 4 ) & 0x0FFF;
        total = total ^ r1 ^ crc16_table[ ( *p >> 4 ) & 0xF ];

        /* advance to next byte */
        p++;
    }
    return ( total ) ;
}


#ifdef DRIVER
#define BUFSIZE 8192

main ( int argc, char *argv[] )
{
    FILE *fin;
    int  n;        /* number of bytes actually read */
    unsigned short int  crc;      /* CRC value */
    char *buffer;  /* buffer into which we read file */

    if ( argc < 2 )
    {
        fprintf ( stderr, "Error: must provide filename\n" );
        return ( EXIT_FAILURE );
    }

    if (( fin = fopen ( argv[1], "rb" )) == NULL )
    {
        fprintf ( stderr, "Cannot open %s \n", argv[1] );
        return ( EXIT_FAILURE );
    }

    buffer = (char *) malloc ( BUFSIZE );
    if ( buffer == NULL )
    {
        fprintf ( stderr, "Error allocating memory\n" );
        return ( EXIT_FAILURE );
    }

    crc = 0;

    /* principal processing loop */

    do
    {
        /* read file in 8KB blocks */

        n = fread ( buffer, 1, BUFSIZE, fin );

        /* Fold in this block's CRC-16 */

        crc = GetCRC16 ( crc, buffer, n );
    }
    while ( n == BUFSIZE );   /* Stop when done */

    fclose ( fin ) ;

    /* check for read error */

    if ( ferror ( fin ))
        fprintf ( stderr, "Error in Processing %s\n", argv[1] );

    /* No error, so report CRC-16 */

    else
        printf ( "CRC-16 for  %s = %04X\n", argv[1], crc );

    return ( EXIT_SUCCESS );
}
#endif
