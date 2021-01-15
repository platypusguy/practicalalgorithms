/*--- lzwcomp.c --------------------------- Listing 9-9 --------------
 * Compress an input file, using the LZW encoding technique
 *
 * Entry point:     int LZWEncode (FILE *infile, FILE *outfile)
 *
 *                  Will compress infile into outfile. Both
 *                  files should already be open in binary mode.
 *                  They will be closed by LZWEncode().
 *
 *                  LZWEncode() returns:
 *                      0: success
 *                      1: error while reading in
 *                      2: malloc error
 *                      4: error while writing out
 *
 * Switches:        DRIVER    - compiles a test driver
 *                  HASHSTATS - reports on hashing efficiency
 *                  HASHHITS  - show table of hash hits
 *                  SHOWSTATS - provides compression statistics
 *-------------------------------------------------------------*/

#define DRIVER
#define SHOWSTATS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lzw.h"

CountType LZWTotBytesIn; /* no. data bytes in input file */
CountType LZWTotBytesOut; /* no. data bytes in output file */
CountType LZWCurrBytesIn; /* no. data bytes in current frame */
CountType LZWCurrBytesOut; /* no. data bytes in current frame */
unsigned  LZWBestRatio; /* best ratio we've had */
unsigned  LZWLastRatio; /* last ratio we saw */

#if defined( HASHHITS ) && !defined( HASHSTATS )
#  define HASHSTATS
#endif

#if defined( HASHSTATS )
#  define SHOWHASH(x) x
CountType HashCount;
CountType HashCollide;
IndexType FreeMax;
#  if defined( HASHHITS )
unsigned long HashHits[ DICTIONARY_SIZE ];
#  endif
#else
#  define SHOWHASH(x)
#endif

/* structure for the data dictionary */
struct Dictionary {
    unsigned char c;
    CodeType code;
    CodeType parent;
};


/*--- the data storage and output routines ---*/

#if BITS == 12

struct Dictionary dictionary[ DICTIONARY_SIZE ];

/* These are the 12-bit routines */

static int havenibble;
static int olddata;
static void LZWOutInit ( FILE *outfile )
{
    char sig[] = SIGNATURE;
    fwrite ( sig, strlen ( sig ) + 1, 1, outfile );
    havenibble = 0;
    LZWLastRatio = LZWBestRatio = 100;
    SHOWHASH( HashCount = HashCollide = 0; )
#  if defined( HASHHITS )
    SHOWHASH( memset ( HashHits, 0,
              DICTIONARY_SIZE * sizeof ( unsigned long )); )
#  endif
    SHOWHASH( FreeMax = 0; )
}

static int LZWOutFlush ( FILE *outfile )
{
    if ( havenibble )
    {
        if ( fputc ( olddata, outfile ) == EOF )
            return ( 4 );
        LZWTotBytesOut += 1;
        LZWCurrBytesOut += 1;
    }
    return ( 0 );
}

static int LZWOut ( FILE *outfile, CodeType code )
{
    if ( havenibble )
    {
        olddata |= ( code >> 8 ) & 0xF;
        if ( fputc ( olddata, outfile ) == EOF )
            return ( 4 );
        if ( fputc ( code, outfile ) == EOF )
            return ( 4 );
        LZWTotBytesOut += 2;
        LZWCurrBytesOut += 2;

        havenibble = 0;
    }
    else
    {
        olddata = ( code >> 4 ) & 0xFF;
        if ( fputc ( olddata, outfile ) == EOF )
            return ( 4 );
        LZWTotBytesOut += 1;
        LZWCurrBytesOut += 1;
        olddata = ( code << 4 ) & 0xF0;
        havenibble = 1;
    }
    return ( 0 );
}
#elif BITS == 16
struct Dictionary *dictionary;

/* These are the 16-bit routines */
static void LZWOutInit( FILE *outfile )
{
    SHOWHASH( CountType i; )
    char sig[] = SIGNATURE;

    fwrite ( sig, strlen ( sig ) + 1, 1, outfile );
    SHOWHASH( HashCount = HashCollide = 0; )
#  if defined( HASHHITS )
    SHOWHASH( for ( i = 0; i < DICTIONARY_SIZE; i++ )
                  HashHits[ i ] = 0; )
#  endif

    dictionary = malloc ( DICTIONARY_SIZE *
                    sizeof( struct Dictionary ));
}

static int LZWOutFlush ( FILE *outfile )
{
    return ( 0 );
}

static int LZWOut ( FILE *outfile, CodeType code )
{
    if ( fwrite ( &code, sizeof( CodeType ), 1, outfile ) != 1 )
        return ( 4 );
    LZWTotBytesOut += 2;
    LZWCurrBytesOut += 2;
    return ( 0 );
}
#else
#  error Undefined value for BITS!
#endif

/* our hashing lookup function */
static IndexType LZWFind ( CodeType currcode, int in )
{
    IndexType ndx;
    int step = 11, pastzero = 0;

#if BITS == 12
    ndx = ( currcode << 7 ) ^ in;
#elif BITS == 16
    ndx = ( ( IndexType ) currcode << 8 ) | in;
#endif
    ndx = ndx % DICTIONARY_SIZE;
    SHOWHASH( HashCount++; )
#if defined( HASHHITS )
    SHOWHASH( HashHits[ ndx ] ++; )
#endif
    for ( ;; )
    {
        if ( dictionary[ ndx ].code == UNUSED_CODE )
            break;
        if ( dictionary[ ndx ].parent == currcode &&
             dictionary[ ndx ].c == in )
            break;
        ndx += step;
        if ( ndx >= DICTIONARY_SIZE)
        {
            ndx -= DICTIONARY_SIZE;
            pastzero += 1;

            /*
             * Next is a safety check. If the step
             * value and the dictionary size are
             * relatively prime, there should never
             * be a problem. However, let's not loop
             * too many times.
             */

            if ( pastzero > 5 )
                step = 1;
        }
        SHOWHASH( HashCollide++; )

    }
    return ( ndx );
}

int LZWEncode ( FILE *infile, FILE *outfile )
{
    int retval = 0, in;
    CountType i;
    IndexType freecode = STARTING_CODE;
    CodeType currcode;
    IndexType idx;

    LZWTotBytesIn = 0;
    LZWTotBytesOut = 0;
    LZWCurrBytesIn = 0;
    LZWCurrBytesOut = 0;
    LZWOutInit( outfile );
#if BITS == 16
    if ( ! dictionary )
        return ( 2 );
#endif
    for ( i = 0; i < DICTIONARY_SIZE; i++ )
        dictionary[ i ].code = UNUSED_CODE;

    if (( currcode = fgetc ( infile )) == (CodeType) EOF )
        currcode = END_OF_INPUT;
    else
    {
        LZWTotBytesIn += 1;
        LZWCurrBytesIn += 1;
        currcode &= 0xFF; /* make sure we don't sign extend */
        while (( in = fgetc ( infile )) != EOF )
        {
            LZWTotBytesIn += 1;
            LZWCurrBytesIn += 1;
            idx = LZWFind ( currcode, in );
            if ( dictionary[ idx ].code == UNUSED_CODE )
            {
                /* not a match */
                retval = LZWOut ( outfile, currcode );

                /* now, update the dictionary */
                if ( freecode < MAX_CODE )
                {
                    dictionary[ idx ].c = in;
                    dictionary[ idx ].code = freecode++;
                    dictionary[ idx ].parent = currcode;
                }
                currcode = in;

                /* Had a miss; check compression efficiency */
                if ( LZWCurrBytesIn >= 10000 )
                {
                    unsigned ratio;
                    ratio = ( LZWCurrBytesOut * 100 ) /
                             LZWCurrBytesIn;
#if defined( HASHSTATS )
                    printf( "Input: %lu, "
                            "Output: %lu, "
                            "Overall Ratio; %lu%%, ",
                             LZWTotBytesIn,
                             LZWTotBytesOut,
                             ( LZWTotBytesOut * 100 ) /
                             LZWTotBytesIn );
                    printf( "This frame: %u%%\n", ratio );
#endif
                    LZWCurrBytesIn = 0;
                    LZWCurrBytesOut = 0;
                    if ( ratio > LZWBestRatio )
                    {
                        if ( ratio > 50 &&
                             ( ratio > 90 ||
                             ratio > LZWLastRatio + 10 ))
                        {
#if defined( HASHSTATS )
                            printf ( "Dictionary reset\n" );
#endif
                            LZWOut ( outfile, NEW_DICTIONARY );
                            for ( i = 0;
                                  i < DICTIONARY_SIZE; i++ )
                                dictionary[ i ].code =
                                    UNUSED_CODE;
                            SHOWHASH( if (freecode > FreeMax)
                                FreeMax = freecode; )
                            freecode = STARTING_CODE;
                        }
                    }
                    else
                        LZWBestRatio = ratio;
                    LZWLastRatio = ratio;
                }
            }
            else /* we match so far--keep going */
                currcode = dictionary[ idx ].code;

            if ( retval ) /* make sure no problems so far */
                break;
        }
    }

    retval = LZWOut ( outfile, currcode );
    if ( ! retval )
        retval = LZWOut ( outfile, END_OF_INPUT );
    if ( ! retval )
        retval = LZWOutFlush ( outfile );

#if defined( SHOWSTATS )
    if ( ! retval )
    {
        printf ( "The input file contained %lu bytes\n",
                   LZWTotBytesIn );
        printf ( "The output file contained %lu data bytes\n",
                   LZWTotBytesOut );
        if ( LZWTotBytesIn != 0 )
            printf ( "Output file is %lu%% "
                     "the size of input file\n",
                     ( LZWTotBytesOut * 100 ) / LZWTotBytesIn );
    }
#endif
#if defined( HASHSTATS )
    {
        float ratio;
        if ( freecode > FreeMax )
             FreeMax = freecode;
        printf( "The highest code used was %u\n", FreeMax );
        printf( "There were %ld hashes and %ld collisions,\n",
            HashCount, HashCollide );
        ratio = ( float ) HashCollide / ( float ) HashCount;
        printf( "for a ratio of %f\n", ratio );
#  if defined( HASHHITS )
        for ( i = 0; i < min( DICTIONARY_SIZE, 4096 ); i += 8 )
        {
            int j;
            printf( "%6ld: ", i );
            for ( j = 0; j < 8; j++ )
            {
                if ( i + j >= DICTIONARY_SIZE )
                    break;
                printf( "%10ld  ", HashHits[ i + j ] );
            }
            printf( "\n" );
        }
#  endif
    }
#endif

    if ( ferror ( infile ))
        retval = 1;
    if ( ferror ( outfile ))
        retval = 4;
    fclose ( infile );
    fclose ( outfile );
    return ( retval );
}

/*--------------------------------------------------------------
 * The following driver for the previous routines is active
 * if DRIVER is defined.
 *-------------------------------------------------------------*/
#if defined( DRIVER )
int main( int argc, char *argv[] )
{
    FILE * infile, *outfile;
    int retval;

    if ( argc != 3 )
    {
        printf( "Usage: lzwcomp infile outfile\n" );
        return ( EXIT_FAILURE );
    }

    infile = fopen( argv[ 1 ], "r+b" );
    if ( infile == NULL )
    {
        fprintf( stderr, "can't open %s for input\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    outfile = fopen( argv[ 2 ], "w+b" );
    if ( outfile == NULL )
    {
        fprintf( stderr, "can't open %s for output\n", argv[2] );
        return ( EXIT_FAILURE );
    }
    setvbuf ( outfile, NULL, _IOFBF, 8192 );

    retval = LZWEncode ( infile, outfile );

    if ( retval )
    {
        printf( "compression failed: " );
        if ( retval == 1 )
            printf ( "input error\n" );
        else
        if ( retval == 2 )
            printf ( "malloc error\n" );
        else
        if ( retval == 4 )
            printf ( "output error\n" );
        else
            printf ( "\n" );
        return  ( retval );
    }
    else
    {
        printf( "%s was compressed into %s\n",
                                       argv[ 1 ], argv[ 2 ] );
        return ( EXIT_SUCCESS );
    }
}
#endif
