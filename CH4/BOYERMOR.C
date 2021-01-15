/*-- BOYERMOR.C --------------------------- Listing 4-3 --------
 * Boyer-Moore string search routine
 *
 *   Preprocessor switches: if #defined:
 *
 *      DEBUG will cause the search routine to dump its tables
 *            at various times--this is useful when trying to
 *            understand how MatchJump is generated
 *
 *      DRIVER will cause a test driver to be compiled
 *
 *------------------------------------------------------------*/

#define DRIVER  1
/* #define DEBUG   1 */

#if defined(DEBUG)
#define SHOWCHAR for (uT=1; uT<= PatLen; uT++)  \
                     printf(" %c ", String[uT-1])
#define SHOWJUMP for (uT=1;uT<= PatLen;uT++)    \
                    printf("%2d ", MatchJump[uT])
#define SHOWA    printf("  uA = %u  ", uA)
#define SHOWB    printf("  uB = %u", uB)
#define SHOWBACK for (uT=1;uT<= PatLen;uT++)    \
                    printf("%2d ", BackUp[uT])
#define NL       printf("\n")

unsigned uT;
#else
#define SHOWCHAR
#define SHOWJUMP
#define SHOWA
#define SHOWB
#define SHOWBACK
#define NL
#endif

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define AlphabetSize (UCHAR_MAX + 1)    /* For portability */

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
 
char *BoyerMoore ( const char * String, /* search for this */
                   const char * Text,   /* ...in this text */
                   size_t TextLen )     /* ...up to here.  */
{
             /* array of character mismatch offsets */
    unsigned CharJump[AlphabetSize];

             /* array of offsets for partial matches */
    unsigned *MatchJump;

             /* temporary array for MatchJump calculation */
    unsigned *BackUp;
    size_t   PatLen;
    unsigned u, uText, uPat, uA, uB;


    /* Set up and initialize arrays */
    PatLen = strlen ( String );
    MatchJump = (unsigned *)
         malloc ( 2 * sizeof ( unsigned ) * ( PatLen + 1 ));
    BackUp = MatchJump + PatLen + 1;

    /* Heuristic #1 -- simple char mismatch jumps ... */

    memset ( CharJump, 0, AlphabetSize * sizeof(unsigned) );
    for ( u = 0 ; u < PatLen; u++ )
        CharJump[((unsigned char) String[u])]
                     = PatLen - u - 1;

    /* Heuristic #2 -- offsets from partial matches ... */
    for ( u = 1; u <= PatLen; u++ )
        MatchJump[u] = 2 * PatLen - u;
                                /* largest possible jump */
    SHOWCHAR; NL;
    SHOWJUMP; NL;

    u  = PatLen;
    uA = PatLen + 1;
    while ( u > 0 )
    {
        BackUp[u] = uA;
        while ( uA <= PatLen &&
                String[u - 1] != String[uA - 1] )
        {
            if ( MatchJump[uA] > PatLen - u )
                 MatchJump[uA] = PatLen - u;
            uA = BackUp[uA];
        }
        u--;
        uA--;
    }

    SHOWJUMP; SHOWA; SHOWBACK; NL;

    for ( u = 1; u <= uA; u++ )
        if ( MatchJump[u] > PatLen + uA - u )
             MatchJump[u] = PatLen + uA - u;

    uB = BackUp[uA];
    SHOWJUMP; SHOWB; NL;

    while ( uA <= PatLen )
    {
        while ( uA <= uB )
        {
            if ( MatchJump[uA] > uB - uA + PatLen )
                 MatchJump[uA] = uB - uA + PatLen;
            uA++;
        }
        uB = BackUp[uB];
    }
    SHOWJUMP; NL;

    /* now search */
    uPat =  PatLen;         /* tracks position in Pattern */
    uText = PatLen - 1;     /* tracks position in Text */
    while ( uText < TextLen && uPat != 0 )
    {
        if ( Text[uText] == String[uPat - 1])  /* match? */
        {
            uText--;    /* back up to next */
            uPat--;
        }
        else  /* a mismatch - slide pattern forward */
        {
            uA = CharJump[((unsigned char) Text[uText])];
            uB = MatchJump[uPat];
            uText += max(uA, uB);  /* select larger jump */
            uPat = PatLen;
        }
    }

    /* return our findings */
    if ( uPat == 0 )
        return ( (char *) ( Text + ( uText + 1 )));  /* a match */
    else
        return ( NULL );                  /* no match */
}

/*---------------------------------------------------------------
 * The main driver, activated by #defining DRIVER.
 * Will print all occurrences of a match in the first
 * 10,000 characters of the target file.
 *-------------------------------------------------------------*/

#ifdef DRIVER

#define MAX_TEXT_SIZE 10000u

int main ( int argc, char *argv[] )
{
    char *SearchFor, *Filename;

    FILE *Fin;                      /* File to search */
    char *Buffer;                   /* Buffer from file */

    char *start, *p;
    int i;
    size_t   TextSize;
    unsigned count;

    if ( argc != 3 )
    {
        puts ( "Usage is: boyermor search-string filename\n" );
        return ( EXIT_FAILURE );
    }
    else
    {
        SearchFor = argv[1];
        Filename  = argv[2];
    }

    if (( Fin = fopen ( Filename, "r" )) == NULL )
    {
        fprintf ( stderr, "Can't open %s\n", Filename );
        return ( EXIT_FAILURE );
    }

    /* allocate search buffer and fill it with target file */
    Buffer = (char*) malloc ( MAX_TEXT_SIZE + 1 );
    if ( Buffer == NULL )
    {
        puts ( "Error! Could not allocate buffer space\n" );
        return ( EXIT_FAILURE );
    }

    TextSize = fread ( Buffer, 1, MAX_TEXT_SIZE, Fin );
    fclose ( Fin );

    p = Buffer;
    count = 0;
    while ( count < TextSize )
    {
        if ( *p == '\n' )
             *p = '\0';
        p++;
        count++;
    }

    /* now search repeatedly */

    start = BoyerMoore ( SearchFor, Buffer, TextSize );
    if ( start == NULL )              /* no match found */
        printf ( "\n%s Not Found.\n", SearchFor );
    else                             /* match found */
        while ( start != NULL )
        {
            for ( p = start; ; p-- )  /* find start of line */
            {
                if ( *p == '\0' )
                {
                    p++;
                    break;
                }
                else
                if ( p == Buffer )
                    break;
            }                         /* print the match */
            printf( "Found:\n%s\n", p );
            for ( i = start - p; i > 0; i-- )
                fputc ( ' ', stdout );
            printf ( "%s\n\n", SearchFor );
            start =                   /* continue the search */
                BoyerMoore ( SearchFor, start + 1,
                            TextSize - ( start - Buffer ) - 1 );
        }
    return ( EXIT_SUCCESS );
}
#endif

