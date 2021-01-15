/*--- lzwunc.c ---------------------------- Listing 9-10 -------
 * Decompress an LZW-compressed file
 *
 * Entry point:     int LZWDecode(FILE *infile, FILE *outfile)
 *
 *                  Will decompress infile into outfile. Both
 *                  files should already be open in binary mode.
 *                  They will be closed by LZWDecode().
 *
 *                  LZWDecode() returns:
 *                      0: success
 *                      1: invalid signature byte
 *                      2: bad malloc
 *                      3: read error
 *                      4: write error
 *
 *
 * Switches:        DRIVER - compiles a test driver
 *                  CODES  - displays input codes
 *-------------------------------------------------------------*/
#define DRIVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lzw.h"

#if defined( CODES )
unsigned long outcount;
#endif

/* structure for data dictionary */
struct Dictionary {
    unsigned char c;
    CodeType parent;
};

/* decode buffer */
static unsigned char *DecodeBuffer;
static unsigned DecodeBufferSize = 0;
static unsigned LZWLoadBuffer( unsigned, CodeType );

/* the input routines and data storage routines */
#if BITS == 12
/* These are the 12-bit input routines */
/* data dictionary */
struct Dictionary dictionary[ MAX_CODE ];

static int havenibble;
static int olddata;
static CodeType incode;
static int LZWInInit ( void )
{
    havenibble = 0;
    if ( ! DecodeBufferSize )
    {
        DecodeBufferSize = 1000;
        DecodeBuffer = malloc( DecodeBufferSize );
        if ( DecodeBuffer == NULL )
            return ( 2 );
    }
#  if defined( CODES )
    outcount = 0;
#  endif
    return 0;
}

static int LZWIn ( FILE *infile )
{
    int data;
    if ( havenibble )
    {
        incode = olddata << 8;
        if (( data = fgetc ( infile )) == EOF )
            return ( 4 );
        incode |= ( data & 0xFF );
        havenibble = 0;
#  if defined( CODES )
        {
            unsigned count;
            printf ( "%6ld (1): %4x, \"", outcount, incode );
            count = LZWLoadBuffer ( 0, incode );
            while ( count )
                if ( DecodeBuffer[ --count ] < 32 )
                    printf ( "'0x%X'", DecodeBuffer[ count ] );
                else
                    fputc ( DecodeBuffer[ count ], stdout );
            printf ( "\"\n" );
        }
#  endif
        return ( 0 );
    }
    else
    {
        if (( data = fgetc ( infile )) == EOF )
            return ( 4 );
        incode = ( data & 0xFF ) << 4;
        if (( data = fgetc ( infile )) == EOF )
            return ( 4 );
        incode |= ( data >> 4 ) & 0xF;
#  if defined( CODES )
        {
            unsigned count;
            printf( "%6ld (0): %4x, \"", outcount, incode );
            count = LZWLoadBuffer ( 0, incode );
            while ( count )
                if ( DecodeBuffer[ --count ] < 32 )
                    printf ( "'0x%X'", DecodeBuffer[ count ] );
                else
                    fputc ( DecodeBuffer[ count ], stdout );
            printf ( "\"\n" );
        }
#  endif
        olddata = data & 0xF;
        havenibble = 1;
        return ( 0 );
    }
}

#elif BITS == 16
/* These are the 16-bit routines */
/* data dictionary */
struct Dictionary *dictionary;

static CodeType incode;
static int LZWInInit( void )
{
#  if defined( CODES )
    outcount = 0;
#  endif
    if (!DecodeBufferSize)
    {
        DecodeBufferSize = 1000;
        DecodeBuffer = malloc ( DecodeBufferSize );
        if ( DecodeBuffer == NULL )
            return ( 2 );
    }
    dictionary = malloc( DICTIONARY_SIZE *
                    sizeof( struct Dictionary ) );
    if ( dictionary == NULL )
        return ( 2 );

    return ( 0 );
}

static int LZWIn ( FILE *infile )
{
    if ( fread ( &incode, sizeof( CodeType ), 1, infile ) != 1 )
        return ( 4 );
#  if defined( CODES )
    {
        unsigned count;
        printf ( "%6ld (0): %4x, \"", outcount, incode );
        count = LZWLoadBuffer ( 0, incode );
        while ( count )
            if ( DecodeBuffer[ --count ] < 32 )
                printf ( "'0x%X'", DecodeBuffer[ count ] );
            else
                fputc( DecodeBuffer[ count ], stdout );
        printf( "\"\n" );
    }
#  endif
    return ( 0 );
}

#else
#  error Undefined value for BITS!
#endif

/* the actual decompression routine */
static IndexType freecode;
static unsigned LZWLoadBuffer ( unsigned count, CodeType code )
{
    if ( code >= freecode )
    {
        printf( "LZWLoad: code %u out of range!", code );
        return ( 0 );
    }
    while ( code >= PRESET_CODE_MAX )
    {
        DecodeBuffer[ count++ ] = dictionary[ code ].c;
        if ( count == DecodeBufferSize )
        {
            DecodeBuffer =
               realloc ( DecodeBuffer, DecodeBufferSize + 1000 );
            if ( ! DecodeBuffer )
            {
                /* out of memory */
                DecodeBufferSize = 0;
                return ( 0 );
            }
            else
                DecodeBufferSize += 1000;
        }
        code = dictionary[ code ].parent;
    }
    DecodeBuffer[ count++ ] = code;
    return ( count );
}

int LZWDecode ( FILE *infile, FILE *outfile )
{
    char buffer[ 10 ];
    int retval = 0;
    unsigned int inchar;
    unsigned count;
    CodeType oldcode;

    /* check the signature */
    fgets ( buffer, strlen ( SIGNATURE ) + 2, infile );
    if ( strcmp ( buffer, SIGNATURE ))
    {
        retval = 1;
        goto done;
    }

    /* prime the pump */
    if ( retval = LZWInInit() )
        goto done;
 priming:
    freecode = STARTING_CODE;
    if ( retval = LZWIn ( infile ))
        goto done;
    if ( incode == END_OF_INPUT )
        goto done;

    /* the first character always is itself */
    oldcode = incode;
    inchar = incode;
    if ( fputc( incode, outfile ) == EOF )
    {
        retval = 4;
        goto done;
    }
#if defined( CODES )
    outcount += 1;
#endif

    while ( ! ( retval = LZWIn ( infile )))
    {
        if ( incode == END_OF_INPUT )
            break;
        if ( incode == NEW_DICTIONARY )
            goto priming;
        if ( incode >= freecode )
        {
            /* We have a code that's not in our dictionary! */
            /* This can happen only one way--see text */

            count = LZWLoadBuffer ( 1, oldcode );

            /* Make last char same as first. Can use either */
            /* inchar or the DecodeBuffer[count-1] */

            DecodeBuffer[ 0 ] = inchar;
        }
        else
            count = LZWLoadBuffer ( 0, incode );

        if ( count == 0 )
            return ( 2 ); /* had a memory problem */

        inchar = DecodeBuffer[ count - 1 ];
        while ( count )
        {
            if ( fputc ( DecodeBuffer[--count], outfile) == EOF )
            {
                retval = 4;
                goto done;
            }
#if defined( CODES )
            outcount += 1;
#endif
        }

        /* now, update the dictionary */
        if ( freecode < MAX_CODE )
        {
            dictionary[ freecode ].parent = oldcode;
            dictionary[ freecode ].c = inchar;
            freecode += 1;

#if defined( CODES )
            {
                unsigned cnt;
                printf( "    just added code %5u: \"",
                                freecode - 1 );
                cnt = LZWLoadBuffer ( 0, freecode - 1 );
                while ( cnt )
                    if ( DecodeBuffer[ --cnt ] < 32 )
                        printf ( "'0x%X'", DecodeBuffer[ cnt ] );
                    else
                        fputc ( DecodeBuffer[ cnt ], stdout );
                printf( "\"\n" );
            }
#endif
        }
        oldcode = incode;
    }

 done:
    fclose ( infile );
    fclose ( outfile );
    return ( retval );
}

#if defined( DRIVER )
int main ( int argc, char *argv[] )
{
    FILE * infile, *outfile;
    int retval;

    if ( argc != 3 )
    {
        fprintf ( stderr, "Usage: lzwunc infile outfile\n" );
        return ( EXIT_FAILURE );
    }

    infile = fopen ( argv[ 1 ], "r+b" );
    if ( infile == NULL )
    {
        fprintf ( stderr, "can't open %s for input\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    outfile = fopen ( argv[ 2 ], "w+b" );
    if ( outfile == NULL )
    {
        fprintf( stderr, "can't open %s for output\n", argv[2] );
        return ( EXIT_FAILURE );
    }

    if ( retval = LZWDecode( infile, outfile ))
    {
        printf( "uncompression failed: " );
        if ( retval == 1 )
            printf ( "invalid signature bytes\n" );
        else
        if ( retval == 2 )
            printf ( "malloc failed\n" );
        else
        if ( retval == 3 )
            printf ( "read error in data\n" );
        else
        if ( retval == 4 )
            printf ( "write error on output\n" );
        else
            printf ( "\n" );
        return ( retval );
    }
    else
    {
        printf ( "%s was expanded into %s\n",
                                       argv[ 1 ], argv[ 2 ] );
        return ( 0 );
    }
}
#endif
