/*--- APPROX.C ----------------------------- Listing 4-6 -------
 *
 * Approximate string search
 *
 * Usage:
 *  approx pattern text-to-search degree-of-approximation
 *
 *  if DRIVER is #defined, a test driver is compiled
 *-------------------------------------------------------------*/

#define DRIVER 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* local, static data */

static char *Text, *Pattern;/* pointers to search strings      */
static int Textloc;         /* current search position in Text */
static int Plen;            /* length of Pattern               */
static int Degree;          /* max degree of allowed mismatch  */
static int *Ldiff, *Rdiff;  /* dynamic difference arrays       */
static int *Loff,  *Roff;   /* used to calculate start of match*/

void AppInit ( char *pattern, char *text, int degree )
{
    int i;

    /* save parameters */
    Text    = text;
    Pattern = pattern;
    Degree  = degree;

    /* initialize */

    Plen   = strlen ( pattern );
    Ldiff  = (int *) malloc ( sizeof(int) * ( Plen + 1 ) * 4 );
    Rdiff  = Ldiff + Plen + 1;
    Loff   = Rdiff + Plen + 1;
    Roff   = Loff +  Plen + 1;
    for ( i = 0; i <= Plen; i++ )
    {
        Rdiff[i] = i;   /* initial values for right column */
        Roff[i]  = 1;
    }

    Textloc = -1; /* current offset into Text */
}

void AppNext ( char **start, char **end, int *howclose )
{
    int *temp, a, b, c, i;

    *start = NULL;
    while ( *start == NULL )  /* start computing columns   */
    {
        if ( Text[++Textloc] == '\0' ) /* no more text     */
            break;

        temp     = Rdiff;  /* move right column to left    */
        Rdiff    = Ldiff;  /*  so that we can compute new  */
        Ldiff    = temp;   /*  right-hand column.          */
        Rdiff[0] = 0;      /* top (boundary) row           */

        temp     = Roff;    /* and swap offset arrays, too */
        Roff     = Loff;
        Loff     = temp;
        Roff[1]  = 0;

        for ( i = 0; i < Plen; i++ )  /* run thru pattern  */
        {
            /* compute a, b, & c as the 3 adjacent cells...*/
            if ( Pattern[i] == Text[Textloc] )
                a = Ldiff[i];
            else
                a = Ldiff[i] + 1;
            b = Ldiff[i+1] + 1;
            c = Rdiff[i] + 1;

            /* ... now pick minimum ... */
            if ( b < a )
                a = b;
            if ( c < a )
                a = c;

            /* ... and store */
            Rdiff[i+1] = a;
        }
       /*--------------------------------------------------------
        *  Now update offset array:
        *  the values in the offset arrays are added to the
        *  current location to determine the beginning of the
        *  mismatched substring. See text for details.
        *------------------------------------------------------*/

        if ( Plen > 1 )
            for ( i = 2; i <= Plen; i++ )
            {
                if ( Ldiff[i-1] < Rdiff[i] )
                     Roff[i] = Loff[i-1] - 1;
                else
                if ( Rdiff[i-1] < Rdiff[i] )
                     Roff[i] = Roff[i-1];
                else
                if ( Ldiff[i] < Rdiff[i] )
                    Roff[i] = Loff[i] - 1;
                else    /* Ldiff[i-1] == Rdiff[i] */
                    Roff[i] = Loff[i-1] - 1;
            }

		/* now, do we have an approximate match? */

        if ( Rdiff[Plen] <= Degree )  /* indeed so! */
        {
            *end      = Text + Textloc;
            *start    = *end + Roff[Plen];
			*howclose = Rdiff[Plen];
		}
    }

    if ( start == NULL ) /* all done: free dynamic arrays */
        free ( Ldiff );
}

#ifdef DRIVER

int main ( int argc, char *argv[] )
{
    char *begin, *end;
    int howclose;

    if ( argc != 4 )
    {
        fprintf ( stderr,
                    "Usage is: approx pattern text degree\n" );
        return ( EXIT_FAILURE );
    }

    AppInit ( argv[1], argv[2], atoi ( argv[3] ));
    AppNext ( &begin, &end, &howclose );

    while ( begin != NULL )
    {
        printf ( "Degree %d: %.*s\n",
                  howclose, end-begin + 1, begin );
        AppNext ( &begin, &end, &howclose );
    }

    return ( EXIT_SUCCESS );
}
#endif
