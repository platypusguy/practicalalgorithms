/*--- metaphon.c -------------------------- Listing 4-8 ---------
*
* Usage: the calling function must pass three arguments:
*
*    char *word      - the word to be converted to a 'metaph'
*    char *result    - a MAXMETAPH+1 byte field for the result
*    int  flag       - a flag
*
* If flag is 1, then a code will be computed for word and
* stored in result. If flag is 0, then the function will compute
* a code for word and compare it with the code passed in
* result. It will return 0 for a match, else -1. The function
* will also return -1 if word is 0 bytes long.
*
* v. 1.1 Correction from the book version.
*        line 72 added below to correct non-initialization
*                of ntrans[0].
*--------------------------------------------------------------*/

#define DRIVER 1

#include <ctype.h>

#define MAXMETAPH  4

int Metaphone ( const char *, char *, int );

/* Character coding array */
static char codes[26] =  {
    1,16,4,16,9,2,4,16,9,2,0,2,2,2,1,4,0,2,4,4,1,0,0,0,8,0
 /* A  B C  D E F G  H I J K L M N O P Q R S T U V W X Y Z*/
    };

/*--- Macros to access character coding array -------------*/

#define ISVOWEL(x)  (codes[(x) - 'A'] & 1)      /* AEIOU */

    /* Following letters are not changed */
#define NOCHANGE(x) (codes[(x) - 'A'] & 2)      /* FJLMNR */

    /* These form diphthongs when preceding H */
#define AFFECTH(x) (codes[(x) - 'A'] & 4)       /* CGPST */

    /* These make C and G soft */
#define MAKESOFT(x) (codes[(x) - 'A'] & 8)      /* EIY */

    /* These prevent GH from becoming F */
#define NOGHTOF(x)  (codes[(x) - 'A'] & 16)     /* BDH */

int Metaphone ( const char *word, char *result, int flag )
{
    char *n, *n_start, *n_end; /* pointers to string */
    char *metaph, *metaph_end; /* pointers to metaph */
    char ntrans[32];           /* word with uppercase letters */
    char newm[8];              /* new metaph for comparison */
    int  KSflag;               /* state flag for X to KS */

    /*--------------------------------------------------------
     *  Copy word to internal buffer, dropping non-alphabetic
     *  characters and converting to uppercase.
     *-------------------------------------------------------*/

    for ( n = ntrans + 1, n_end = ntrans + 30;
         *word && n < n_end; word++ )
            if ( isalpha ( *word ))
                *n++ = toupper ( *word );

    if ( n == ntrans + 1 )   /* return if 0 bytes */
        return -1;
    n_end = n;               /* set n_end to end of string */

    ntrans[0] = 'Z';         /* ntrans[0] should be a neutral char */
    *n++ = 0; *n = 0;        /* pad with nulls */
    n = ntrans + 1;          /* assign pointer to start */

    /* if doing a comparison, redirect pointers */
    if ( !flag )
    {
        metaph = result;
        result = newm;
    }

   /*------------------------------------------------------------
    *  check for all prefixes:
    *            PN KN GN AE WR WH and X at start.
    *----------------------------------------------------------*/

    switch ( *n )
    {
        case 'P':
        case 'K':
        case 'G':
            if ( *( n + 1 ) == 'N')
                 *n++ = 0;
            break;

        case 'A':
            if ( *( n + 1 ) == 'E')
                 *n++ = 0;
            break;
        case 'W':
            if ( *( n + 1 ) == 'R' )
                 *n++ = 0;
            else
            if ( *(n + 1) == 'H')
            {
                *( n + 1 ) = *n;
                *n++ = 0;
            }
            break;
        case 'X':
            *n = 'S';
            break;
    }

   /*------------------------------------------------------------
    *  Now, loop step through string, stopping at end of string
    *  or when the computed metaph is MAXMETAPH characters long
    *----------------------------------------------------------*/

    KSflag = 0; /* state flag for KS translation */

    for ( metaph_end = result + MAXMETAPH, n_start = n;
          n <= n_end && result < metaph_end; n++ )
    {

        if ( KSflag )
        {
            KSflag = 0;
            *result++ = *n;
        }
        else
        {
            /* drop duplicates except for CC */
            if ( *( n - 1 ) == *n && *n != 'C' )
                continue;

            /* check for F J L M N R or first letter vowel */
            if ( NOCHANGE ( *n ) ||
                    ( n == n_start && ISVOWEL ( *n )))
                *result++ = *n;
            else
            switch ( *n )
            {
            case 'B':        /* check for -MB */
                if ( n < n_end || *( n - 1 ) != 'M' )
                    *result++ = *n;
                break;

            case 'C': /* C = X ("sh" sound) in CH and CIA */
                        /*   = S in CE CI and CY            */
                        /*     dropped in SCI SCE SCY       */
                        /* else K                           */
                if ( *( n - 1 ) != 'S' ||
                    !MAKESOFT ( *( n + 1 )))
                {
                    if ( *( n + 1 ) == 'I' && *( n + 2 ) == 'A' )
                        *result++ = 'X';
                    else
                    if ( MAKESOFT ( *( n + 1 )))
                        *result++ = 'S';
                    else
                    if ( *( n + 1 ) == 'H' )
                        *result++ = (( n == n_start &&
                                    !ISVOWEL ( *( n + 2 ))) ||
                                        *( n - 1 ) == 'S' ) ?
                                        (char)'K' : (char)'X';
                    else
                        *result++ = 'K';
                }
                break;

            case 'D':  /* J before DGE, DGI, DGY, else T */
                *result++ =
                    ( *( n + 1 ) == 'G' &&
                        MAKESOFT ( *( n + 2 ))) ?
                            (char)'J' : (char)'T';
                break;

            case 'G':   /* complicated, see table in text */
                if (( *( n + 1 ) != 'H' || ISVOWEL ( *( n + 2 )))
                    && (
                        *( n + 1 ) != 'N' ||
                        (
                            (n + 1) < n_end  &&
                            (
                                *( n + 2 ) != 'E' ||
                                *( n + 3 ) != 'D'
                            )
                        )
                    )
                    && (
                        *( n - 1 ) != 'D' ||
                        !MAKESOFT ( *( n + 1 ))
                        )
                )
                        *result++ =
                            ( MAKESOFT ( *( n  + 1 )) &&
                            *( n + 2 ) != 'G' ) ?
                                    (char)'J' : (char)'K';
                else
                if( *( n + 1 ) == 'H'   &&
                    !NOGHTOF( *( n - 3 )) &&
                    *( n - 4 ) != 'H')
                        *result++ = 'F';
                break;

            case 'H':   /* H if before a vowel and not after */
                        /* C, G, P, S, T */

                if ( !AFFECTH ( *( n - 1 )) &&
                    ( !ISVOWEL ( *( n - 1 )) ||
                    ISVOWEL ( *( n + 1 ))))
                            *result++ = 'H';
                break;

            case 'K':    /* K = K, except dropped after C */
                if ( *( n - 1 ) != 'C')
                    *result++ = 'K';
                break;

            case 'P':    /* PH = F, else P = P */
                *result++ = *( n +  1 ) == 'H'
                        ? (char)'F' : (char)'P';
                break;
            case 'Q':   /* Q = K (U after Q is already gone */
                *result++ = 'K';
                break;

            case 'S':   /* SH, SIO, SIA = X ("sh" sound) */
                *result++ = ( *( n + 1 ) == 'H' ||
                            ( *(n  + 1) == 'I' &&
                                ( *( n + 2 ) == 'O' ||
                                *( n + 2 ) == 'A')))  ?
                                    (char)'X' : (char)'S';
                break;

            case 'T':  /* TIO, TIA = X ("sh" sound) */
                        /* TH = 0, ("th" sound ) */
                if( *( n  + 1 ) == 'I' && ( *( n + 2 ) == 'O'
                    || *( n + 2 ) == 'A') )
                        *result++ = 'X';
                else
                if ( *( n + 1 ) == 'H' )
                    *result++ = '0';
                else
                if ( *( n + 1) != 'C' || *( n + 2 ) != 'H')
                    *result++ = 'T';
                break;

            case 'V':     /* V = F */
                *result++ = 'F';
                break;

            case 'W':     /* only exist if a vowel follows */
            case 'Y':
                    if ( ISVOWEL ( *( n + 1 )))
                        *result++ = *n;
                break;

            case 'X':     /* X = KS, except at start */
                if ( n == n_start )
                    *result++ = 'S';
                else
                {
                    *result++ = 'K'; /* insert K, then S */
                    KSflag = 1; /* this flag will cause S to be
                            inserted on next pass thru loop */
                }
                break;

            case 'Z':
                *result++ = 'S';
                break;
            }
        }

        /* compare new metaph with old */
        if ( !flag && *( result - 1 ) !=
                metaph[( result - newm ) - 1] )
            return -1;
    }

    /* If comparing, check that metaphs were equal length */
    if ( !flag && metaph[result - newm] )
        return -1;

    *result = 0; /* null-terminate return value */
    return 0;
}

#ifdef DRIVER   /* compile a driver to demo routine */

#include <stdio.h>
#include <stdlib.h>

main ( int argc, char *argv[] )
{
    char coded_word [MAXMETAPH+1];

    if ( argc != 2 )
    {
        fprintf ( stderr, "Usage: METAPHON word-to-be-coded\n" );
        return ( EXIT_FAILURE );
    }

    if ( Metaphone ( argv[1], coded_word, 1 ) == -1 )
    {
        fprintf ( stderr, "Invalid word/name to be coded\n" );
        return ( EXIT_FAILURE );
    }
    else
        printf ( "Metaphone for %s is %s\n",
                        argv[1], coded_word );

    return ( EXIT_SUCCESS );
}
#endif
