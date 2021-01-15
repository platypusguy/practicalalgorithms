 /*--- mslzc.c -------------------------------------------------
 *  Compression program that mimics Microsoft's implementation
 *  of a sliding-window compresssion algorithm. This is the
 *  compression routine to use with the mslzunc.c program
 *  shown in Chapter 9. Alternatively, you can use Microsoft's
 *  compress.exe
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

/* output routines */
int CodeByte;
char CodeBuf[16];
int CodeCount;
int CodeStore;
static FILE *CodeFile;
static void OutInit(FILE *outfile)
{
    CodeCount = 0;
    CodeStore = 0;
    CodeByte = 0;
    CodeFile = outfile;
}

static void OutFlush(void)
{
    int i;

    if (CodeCount)
    {
        fputc(CodeByte, CodeFile);
        for (i = 0; i < CodeStore; i++)
            fputc(CodeBuf[i], CodeFile);
    }
    CodeCount = 0;
    CodeByte = 0;
    CodeStore = 0;
}

static void OutChar(int c)
{
    CodeByte |= 1 << CodeCount;
    CodeBuf[CodeStore++] = c;

    CodeCount++;
    if (CodeCount == 8)
        OutFlush();
}

static void OutCode(int pos, int len)
{
    int byte1, byte2;

    /* subtract three, allowing us to store strings up to
       18 bytes long */
    len -= 3;

    /* subtract 0x10 for reasons that are unclear. Microsoft
       does it this way. */
    pos -= 0x10;

    byte1 = pos & 0xFF;
    byte2 = ((pos & 0xF00) >> 4) | len;

    CodeBuf[CodeStore++] = byte1;
    CodeBuf[CodeStore++] = byte2;

    CodeCount++;
    if (CodeCount == 8)
        OutFlush();
}

#define MINMATCH 3
#define MAXMATCH 18
#define TESTWRAP(x) ((x) >= MAXMATCH ? ((x) - MAXMATCH) : (x))
static int mslzCompressFile1(FILE *infile, FILE *outfile)
{
    char testbuf[MAXMATCH];
    int c;
    int teststart, testcount, eof = 0;
    int curr_pos, search, length, bestlength, bestlocation;

    /* Init our window to spaces */
    memset ( Window, ' ', WINSIZE );

    /* and our circular testbuffer */
    teststart = 0;
    testcount = 0;

    /* and the output routines */
    OutInit(outfile);
    curr_pos = 0;

    /* run through input file */
    for(;;)
    {
        /* load up our working test buffer */
        while ((testcount < MAXMATCH) && !eof)
        {
            c = fgetc(infile);
            if (c == EOF)
            {
                eof = 1;
                break;
            }
            testbuf[TESTWRAP(teststart + testcount)] = c;
            testcount++;
        }

        if (testcount == 0)
        {
            /* we're done! */
            OutFlush();
            return EXIT_SUCCESS;
        }

        /* let's see what we can match */
        bestlength = 0;
        for(search = WRAPFIX(curr_pos + 1);
            search != curr_pos;
            search = WRAPFIX(search+1))
        {
            for (length = 0; length < testcount; length++)
            {
                if (WRAPFIX(search + length) == curr_pos)
                    break;
                if (testbuf[TESTWRAP(teststart+length)] !=
                    Window[WRAPFIX(search + length)])
                    break;
            }

            if (length >= 3 && length > bestlength)
            {
                bestlength = length;
                bestlocation = search;
            }

            if (bestlength == testcount)
                break; /* can't do better than this! */
        }

        if (bestlength)
        {
            OutCode(bestlocation, bestlength);
        }
        else
        {
            OutChar(testbuf[teststart]);
            bestlength = 1;
        }

        /* update sliding Window */
        for(; bestlength; bestlength--)
        {
            Window[curr_pos] = testbuf[teststart];
            curr_pos++;
            curr_pos = WRAPFIX(curr_pos);

            teststart++;
            testcount--;
            teststart = TESTWRAP(teststart);
        }
    }
}

int mslzCompressFile(char *in, char *outdir)
{
    int length, retval;
    COMPHEADER header = { MAGIC1, MAGIC2, 0x41, 0, 0L };
    FILE *infile, *outfile;
    char *outname, *s;

    /* set up file names */
    if (( infile = fopen ( in, "rb" )) == NULL)
    {
        fprintf( stderr, "Can't open %s for input\n", in );
        return ( EXIT_FAILURE );
    }

    /* construct target name */
    length = strlen(in) + 2;
    if (outdir)
        length += strlen(outdir) + 1;
    outname = malloc(length);
    if (!outname)
    {
        fprintf (stderr, "Out of memory\n");
        return ( EXIT_FAILURE );
    }
    if (outdir)
    {
        strcpy(outname, outdir);
        s = outname + strlen(outname) - 1;
        if (*s == '\\' || *s == ':') /* ok terminator */
            ;
        else
            strcat(s, "\\");
    }
    else
        outname[0] = 0;
    strcat(outname, in);

    /* now, change the last character */
    s = outname + strlen(outname) - 1;
    if (*s == '.')
    {
        s++;
        *s = 0;
        *(s + 1) = 0;
    }
    header.FileFix = *s;
    *s = '_';

    if (( outfile = fopen ( outname, "wb" )) == NULL )
    {
        fprintf ( stderr, "Can't open %s for output\n", outname);
        return ( EXIT_FAILURE );
    }

    fseek ( infile, 0L, SEEK_END );
    header.UncompSize = ftell ( infile );
    fseek ( infile, 0L, SEEK_SET );

    fwrite(&header, sizeof(header), 1, outfile);

    /* off we go! */
    retval = mslzCompressFile1(infile, outfile);

    fclose ( infile );
    fclose ( outfile );

    return retval;
}

#ifdef DRIVER
/*--------------------------------------------------------------
 * Driver to exercise previous compression routines. Accepts
 * one or two command line arguments: The name of the file to be
 * compressed and an optional directory for the output file. The
 * output file will have the same name as the input file, but the
 * terminal character of the file's name will be replaced with
 * an underscore.
 *-------------------------------------------------------------*/
int main ( int argc, char *argv[] )
{
    if ( argc < 2 || argc > 3 )
    {
        fprintf ( stderr, "Usage: mslzc "
                    "compressed-file [target-directory]\n" );
        return ( EXIT_FAILURE );
    }

    return mslzCompressFile(argv[1],
                            argc == 3 ? argv[2] : NULL);
}
#endif
