/*--- rle1.c ------------------------------ Listing 9-1 ---------
 *  Run-Length Encoding for files of all types
 *
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#define Sentinel    0xF0   /* the sentinel flag */
#define BUFFER_SIZE 30000

#define WriteCode(a,b,c) \
            fprintf( outfile, "%c%c%c", a, b, c )

FILE *infile,
     *outfile;

int main ( int argc, char *argv[] )
{
    unsigned char *buffer;
    unsigned char prev_char;
    int  bytes_read,
         count,
         eof,
         first_time,
         i;

    if ( argc != 3 )
    {
        fprintf ( stderr, "Performs standard RLE compression\n"
                          "Usage: rle1 infile outfile\n" );
        return ( EXIT_FAILURE );
    }

    if (( infile = fopen ( argv[1], "rb" )) == NULL )
    {
        fprintf ( stderr, "Error opening %s\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    if (( outfile = fopen ( argv[2], "wb" )) == NULL )
    {
        fprintf ( stderr, "Error opening %s\n", argv[2] );
        return ( EXIT_FAILURE );
    }

    /* write the header to the output file */

    fprintf ( outfile, "%c%c%c%c", 'R', 'L', '1', Sentinel );

    /* initialize the necessary variables */

    eof        = 0;
    first_time = 1;

    buffer = (unsigned char *) malloc ( BUFFER_SIZE );
    if ( buffer == NULL )
    {
        fprintf ( stderr, "Unable to allocate %d bytes\n",
                    BUFFER_SIZE );
        return ( EXIT_FAILURE );
    }

    /* process the input file */

    while ( ! eof )
    {
        bytes_read = fread ( buffer, 1, BUFFER_SIZE, infile );
        if ( bytes_read == 0 )
        {
            eof = 1;
            break;
        }

        for ( i = 0; i < bytes_read; i++ )
        {
            /* first time through is a special case */

            if ( first_time )
            {
                prev_char = buffer[i];
                count = 1;
                first_time = 0;
                i++;
            }

            if ( buffer[i] == prev_char )   /* repeated char */
            {
                count += 1;
                if ( count == 255 )
                {
                    WriteCode ( Sentinel, count, prev_char );
                    count = 0;
                }
                continue;
            }
            else    /* a new char, so write out all known data */
            {
                if ( count < 3 )
                {
                    if ( prev_char == Sentinel )
                    {
                        fprintf ( outfile, "%c%c",
                                        Sentinel, count );
                    }
                    else
                        do
                        {
                            fputc ( prev_char, outfile );
                        }
                            while ( --count );
                }
                else
                    WriteCode ( Sentinel, count, prev_char );

                prev_char = buffer[i];
                count = 1;
            }
        }

        /* we're at end of bytes_read, is it EOF? */

        if ( bytes_read < BUFFER_SIZE )
            eof = 1;
    }

    /* at EOF, so flush out any remaining bytes to be written */

    if ( count < 3 )
    {
        if ( prev_char == Sentinel )
        {
            fprintf ( outfile, "%c%c",
                            Sentinel, count );
        }
        else
            do
            {
                fputc ( prev_char, outfile );
            }
                while ( --count );
    }
    else
        WriteCode ( Sentinel, count, prev_char );

    fclose ( infile );
    fclose ( outfile );

    return ( EXIT_SUCCESS );
}
