/*--- mslzunc.c --------------------------- Listing 9-6 --------
 *  Uncompress a file compressed with Microsoft's compress.exe
 *  program. compress.exe uses a Microsoft variant of the LZ
 *  family of sliding-window compresssion algorithms.
 *
 *  if DRIVER is #defined a driver mainline is compiled.
 *-------------------------------------------------------------*/

#define DRIVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mslzunc.h"

/* 4KB Uncompression window */
char Window[WINSIZE];

/*--------------------------------------------------------------
 * Principal routine that uncompresses the input data
 * using Microsoft's own implementation of LZ algorithms.
 *-------------------------------------------------------------*/
void UncompressData ( FILE *infile, FILE *outfile,
                            unsigned long uncomp_size )
{

    unsigned char   bit_map, byte1, byte2;
    int             length, counter;
    long            location, curr_pos = 0L;

    /* Init our window to spaces */
    memset ( Window, ' ', WINSIZE );

    /* Go through until done */
    while ( curr_pos < uncomp_size )
    {

        /* Get bit_map of flag indicating codes and bytes */
        bit_map = fgetc ( infile );
        if ( feof ( infile ))
            return;

        /* Go through and decode data */
        for ( counter = 0; counter < 8; counter++ )
        {

            /* It's a code, so decode it and copy the data */
            if ( ! BITSET ( bit_map, counter ))
            {
                byte1 = fgetc ( infile );

                /* Shouldn't be EOF, but just in case... */

                if ( feof ( infile ))
                    return;

                byte2 = fgetc ( infile );
                length = LENGTH ( byte2 );
                location = OFFSET ( byte1, byte2 );

                /* Copy data from 'window' */
                while ( length != 0 )
                {
                    byte1 = Window[WRAPFIX(location)];
                    Window[WRAPFIX(curr_pos)] = byte1;
                    fputc ( byte1, outfile );
                    curr_pos++;
                    location++;
                    length--;
                }
            }
            else    /* it's just a data byte */
            {
                byte1 = fgetc ( infile );
                Window[WRAPFIX(curr_pos)] = byte1;
                fputc ( byte1, outfile );
                curr_pos++;
            }

            if ( feof ( infile ))
                return;

        }
    }
}

/*--------------------------------------------------------------
 * Verifies the header of the compressed file looking for the
 * Microsoft signature and the size of the uncompressed file.
 * Returns the latter upon success or 0L upon failure.
 *-------------------------------------------------------------*/
unsigned long VerifyHeader ( FILE *infile )
{

    COMPHEADER header;
    unsigned long comp_size;

    fseek ( infile, 0, SEEK_END );
    comp_size = ftell ( infile );
    fseek ( infile, 0, SEEK_SET );
    fread ( &header, sizeof ( header ), 1, infile );
    if (( header.Magic1 != MAGIC1 ) ||
        ( header.Magic2 != MAGIC2 ))
    {
        fprintf ( stderr, "input file is not a valid "
                            "compressed file!\n" );
        return ( 0L );
    }

    printf ( "Uncompressing file from %lu bytes to %lu bytes\n",
                comp_size, header.UncompSize );

    return ( header.UncompSize );
}

#ifdef DRIVER
/*--------------------------------------------------------------
 * Driver to exercise previous decompression routines. Accepts
 * exactly two command-line arguments: the name of the compressed
 * file and the name to use for the output compressed file.
 *-------------------------------------------------------------*/
int main ( int argc, char *argv[] )
{

    FILE *infile, *outfile;
    unsigned long uncompressed_size;

    if ( argc != 3 )
    {
        fprintf ( stderr, "Usage: mslzunc"
                    "compressed-file uncompressed-file\n" );
        return ( EXIT_FAILURE );
    }

    if (( infile = fopen ( argv[1], "rb" )) == NULL)
    {
        fprintf( stderr, "Can't open %s for input\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    if (( outfile = fopen ( argv[2], "wb" )) == NULL )
    {
        fprintf ( stderr, "Can't open %s for output\n", argv[2]);
        return ( EXIT_FAILURE );
    }

    uncompressed_size =  VerifyHeader ( infile );
    if ( uncompressed_size != 0L )   /* no error occurred */
        UncompressData ( infile, outfile, uncompressed_size );

    fclose ( infile );
    fclose ( outfile );

    if ( uncompressed_size == 0L )  /* previous error occurred */
        return ( EXIT_FAILURE );
    else
        return ( EXIT_SUCCESS );
}
#endif
