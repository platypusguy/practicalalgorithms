/*--- unrle1.c ---------------------------- Listing 9-2 --------
 *  Uncompresses rle1 files
 *
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 30000

FILE *infile,
     *outfile;

int main ( int argc, char *argv[] )
{
    unsigned char *buffer,
         sentinel;
    int  bytes_read,
         count,
         i;

    /*--- process files open ---*/

    if ( argc != 3 )
    {
        fprintf ( stderr, "Uncompresses RLE1-compressed file\n"
                          "Usage: unrle1 infile outfile\n" );
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

    /*--- allocate input buffer ---*/

    buffer = (unsigned char *) malloc ( BUFFER_SIZE );
    if ( buffer == NULL )
    {
        fprintf ( stderr, "Unable to allocate %d bytes\n",
                    BUFFER_SIZE );
        return ( EXIT_FAILURE );
    }

    /*--- check the file header ---*/

    bytes_read = fread ( buffer, 1, 4, infile );
    if ( bytes_read != 4 )
    {
        fprintf ( stderr, "Unable to read %s\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    if ( buffer[0] == 'R' && buffer[1] == 'L' &&
         buffer[2] == '1' )
            sentinel = buffer[3];
    else
    {
        fprintf ( stderr, "%s is not an RLE 1 file\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    /*--- process the file ---*/

    while ( 1 )     /* loop until break occurs */
    {
        bytes_read = fread ( buffer, 1, BUFFER_SIZE, infile );
        if ( bytes_read == 0 )
            break;

        for ( i = 0; i < bytes_read; i++ )
        {
            if ( buffer[i] != sentinel )
                fputc ( buffer[i], outfile );
            else
            {       /* process sentinel */

                if ( i > bytes_read - 3 ) /* near buffer end */
                {
                    if ( i > bytes_read - 2 ) /* no bytes left */
                    {
                        bytes_read = fread ( buffer, 1,
                                    BUFFER_SIZE, infile );
                        if ( bytes_read < 2 )
                        {
                            fprintf ( stderr,
                                      "error in %s\n", argv[1] );
                            fclose ( infile );
                            unlink ( argv[2] );
                            return ( EXIT_FAILURE );
                        }
                        else
                        {
                            i = -1;
                            goto process_count;
                        }
                    }
                    else                      /* one byte left */
                    {
                        if ( buffer[i + 1] < 3 )
                            goto process_count;

                        count = buffer[i + 1];

                        bytes_read = fread ( buffer, 1,
                                    BUFFER_SIZE, infile );
                        if ( bytes_read < 1 )
                        {
                            fprintf ( stderr,
                                      "error in %s\n", argv[1] );
                            fclose ( infile );
                            unlink ( argv[2] );
                            return ( EXIT_FAILURE );
                        }

                        i = 0;
                        do
                        {
                            fputc ( buffer[i], outfile );
                        }
                            while ( --count );
                    }

                }
                else                    /* not end of buffer */
                {
                  process_count:

                    count = buffer[++i];

                    switch ( count )    /* what's the count? */
                    {
                        case 0:
                            fprintf ( stderr,
                                      "error in %s\n", argv[1] );
                            fclose ( infile );
                            unlink ( argv[2] );
                            return ( EXIT_FAILURE );
                        case 1:
                            fputc ( sentinel, outfile );
                            break;
                        case 2:
                            fputc ( sentinel, outfile );
                            fputc ( sentinel, outfile );
                            break;
                        default:
                            i += 1;
                            do
                            {
                                fputc ( buffer[i], outfile );
                            }
                                while ( --count );
                            break;
                    }
                }
            }
        }

        if ( bytes_read < BUFFER_SIZE )
            break;
    }

    fclose ( infile );
    fclose ( outfile );

    return ( EXIT_SUCCESS );
}
