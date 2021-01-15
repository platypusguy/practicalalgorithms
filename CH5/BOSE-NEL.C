/*--- bose-nel.c -------------------------- Listing 5-19 --------
 * Generate swap pairs based on the Bose-Nelson
 * technique, using the algorithm described by T. N.
 * Hibbard in A Simple Sorting Algorithm, Journal of
 * the ACM 10:142-50, 1963. The code is a direct
 * translation of Hibbard's pseudocode into C, and
 * retains his variable names, line labels, and use
 * of goto statements.
 *
 * Has built-in driver
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

/* common variables */
int SwapPairs = 0; /* running total of number of swap pairs */
FILE *OutFile;	   /* write swap pairs to this file */

/* Bit access macros */
#define SetBit(x,b) ((x) |=  (1 << (b)))
#define ClrBit(x,b) ((x) &= ~(1 << (b)))
#define TstBit(x,b) ((x) &   (1 << (b)))

void BoseSort ( int N )
{
    unsigned int x, xj, y, yj, j, L;

    fprintf ( OutFile,
             "{ /* Bose-Nelson sort for %d elements */\n", N );

    /* L = ceil(log2(N-1)) */
    L = sizeof(int) * CHAR_BIT - 1;
    for ( ; ! TstBit(N - 1, L); L-- )
                ;
    L += 1;

    /* starting values */
    x = 0;
    y = 1;

  /* Top of loop - x and y are a swap pair */
  A:                        /*--- goto target ---*/
    fprintf ( OutFile, "    swap(%d, %d);\n", x, y );
    SwapPairs += 1;

    j = 0;

  C:                        /*--- goto target ---*/
    xj = TstBit(x, j);
    yj = TstBit(y, j);

    if ( xj == 0 && yj == 0 )
        goto zero;
    else if ( xj && yj )
        goto one;
    else if ( xj == 0 )
        goto first_two;
    else
        goto two;

  zero:                     /*--- goto target ---*/

    SetBit(x, j);
    SetBit(y, j);
    if ( y <= N - 1 )
        goto A;

  one:                      /*--- goto target ---*/
    ClrBit(y, j);
    goto A;

  two:                      /*--- goto target ---*/
    ClrBit(x, j);
    j += 1;
    goto C;

  first_two:                /*--- goto target ---*/
    ClrBit(x, j);
    ClrBit(y, j);
    if ( j == L )
        return;
    j += 1;
    if ( TstBit(y, j) )
        goto D;
    SetBit(x, j);
    SetBit(y, j);
    if ( y > N - 1 )
        goto first_two;
    if ( y < N - 1 )
        j = 0;

  D:                        /*--- goto target ---*/
    ClrBit(x, j);
    SetBit(y, j);
    goto A;
}

int main ( int argc, char *argv[] )
{
    int i, n;
    double optimal = 0.0;

    if ( argc != 3 || ( n = atoi ( argv[1] )) <= 1 )
    {
        fprintf ( stderr,
                  "Usage: bose-nel N outfile, where N > 1\n" );
        return ( EXIT_FAILURE );
	}

    if (( OutFile = fopen ( argv[2], "w" )) == NULL)
    {
        fprintf ( stderr, "Can't open %s\n", argv[2] );
        return ( EXIT_FAILURE );
	}

    BoseSort ( n );

    fprintf ( OutFile,
                " } /* There are %d swaps */\n", SwapPairs );
    fclose ( OutFile );

    printf ( "There were %d swaps generated.\n", SwapPairs );

	/*
	 * Report theoretical optimum.
	 * Start by computing log10(n!)
	 */
    for ( i = 1; i <= n; i++ )
        optimal += log10 ( i );

	/*
	 * Convert to ceil(log2(n!)) using
	 * fact that log10(2) = .30103
	 */
    optimal = ceil ( optimal / 0.30103 );

    printf ( "The best theoretical sort uses %.0f swaps.\n",
           (float) optimal );

    return ( EXIT_SUCCESS );
}
