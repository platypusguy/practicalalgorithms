/*--- huffdec.c --------------------------- Listing 9-4 ---------
 * Purpose:         Uncompress a Huffman-compressed file
 *
 * Entry point:     int HuffDecode(FILE *infile, FILE *outfile)
 *
 *                  Will uncompress infile into outfile. Both
 *                  files should already be open in binary mode.
 *                  They will be closed by HuffDecode().
 *
 *                  HuffDecode() returns:
 *                      0: success
 *                      1: invalid signature byte
 *                      2: invalid header bytes
 *                      3: invalid data bytes
 *
 *
 * Switches:        DRIVER - compiles a test driver
 *                  DUMP - dump the tree after loading
 *-------------------------------------------------------------*/
#define DRIVER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "huffman.h"

/* a simple tree */
struct _HuffTree {
    LinkType child[2];
} HuffTree[MAXNODES];

CountType HuffBytesIn;  /* number of bytes to decode */
LinkType HuffRoot;      /* the tree's root */
LinkType HuffCount;     /* the number of internal nodes */

#ifdef DUMP
static void HuffDump ( LinkType n )
{
    LinkType i;

    printf ( "root is %d\n", HuffRoot );
    printf ( "No.  Sym  Left  Right\n" );
    for ( i = 0; i <= n; i++ )
    {
        printf( "%3d. ", i );
        if ( i < MAXSYMBOLS )
        {
            if ( isprint ( i ))
                printf( " %c ", i );
            else
                printf ( "x%.2X", i );
        }
        else
            printf ( "n/a" );

        printf ( " %5d  %5d",
                HuffTree[i].child[LEFT],
                HuffTree[i].child[RIGHT] );

        printf ( "\n" );
    }
    printf ( "\n" );
}
#define DUMPTREE(x) HuffDump(x)
#else
#define DUMPTREE(x)
#endif

/* the actual decompression routine */
int HuffDecode ( FILE *infile, FILE *outfile )
{
    int retval = 0;
    LinkType i;
    char buffer[10];
    struct _Header header;
    CountType bytesout;
    int inbyte, bitno, mask, set;

    /* set all counts to zero and initialize symbols */
    memset ( HuffTree, 0, sizeof ( HuffTree ));

    /* check the signature */
    fgets ( buffer, strlen ( SIGNATURE ) + 2, infile );
    if ( strcmp ( buffer, SIGNATURE ))
    {
        retval = 1;
        goto done;
    }

    /* read in header bytes */

    fread ( &HuffRoot, sizeof ( HuffRoot), 1, infile );
    fread ( &HuffCount, sizeof ( HuffCount), 1, infile );
    fread ( &HuffBytesIn, sizeof ( HuffBytesIn), 1, infile );

    for ( i = 0;
            i < HuffCount + ( HuffRoot - MAXSYMBOLS + 1 ); i++)
    {
        fread ( &header, sizeof ( header ), 1, infile );
        HuffTree[header.index].child[0] = header.child[0];
        HuffTree[header.index].child[1] = header.child[1];
    }

    if ( ferror ( infile ))
    {
        retval = 2;
        goto done;
    }
    DUMPTREE ( HuffRoot );

    /* now, translate the data bytes */
    bitno = 8;
    for ( bytesout = 0; bytesout < HuffBytesIn; bytesout++ )
    {
        /* walk down the tree */
        i = HuffRoot;
        for ( ;; )
        {
            /* do we need a new input byte? */
            if ( bitno > 7 )
            {
                inbyte = fgetc ( infile );
                if ( inbyte == EOF )
                {
                    retval = 3;
                    goto done;
                }
                bitno = 0;
                mask = 0x80;
            }

            /* test the current bit */
            if ( inbyte & mask )
                set = 1;
            else
                set = 0;
            bitno++;
            mask >>= 1;

            /* walk down the tree */
            i = HuffTree[i].child[set];

            /* are we there yet? */
            if ( HuffTree[i].child[0] == -1 )
            {
                /* at the bottom -- write the character */
                fputc ( i, outfile );
                break;
            }
        }
    }

  done:
    fclose ( infile );
    fclose ( outfile );
    return ( retval );
}

#ifdef DRIVER
int main ( int argc, char *argv[] )
{
    FILE *infile, *outfile;
    int retval;

    if ( argc != 3 )
    {
        fprintf ( stderr, "Usage: huffdec infile outfile\n" );
        return ( EXIT_FAILURE );
    }

    infile = fopen ( argv[1], "r+b" );
    if ( infile == NULL )
    {
        fprintf ( stderr, "can't open %s for input\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    outfile = fopen ( argv[2], "w+b" );
    if ( outfile == NULL )
    {
        fprintf ( stderr, "can't open %s for output\n", argv[2]);
        return ( EXIT_FAILURE );
    }

    if ( retval = HuffDecode ( infile, outfile ))
    {
        printf ( "decompression failed: " );
        if ( retval == 1 )
            printf ( "invalid signature byte\n" );
        if ( retval == 2 )
            printf ( "read error in header\n" );
        if ( retval == 3 )
            printf ( "read error in data\n" );
        else
            printf( "\n" );
        return ( retval );
    }
    else
    {
        printf ( "%s was expanded into %s\n",
                                        argv[1], argv[2]);
        return ( 0 );
    }
}
#endif
