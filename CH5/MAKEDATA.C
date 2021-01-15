/*--- makedata.c -------------------------- Listing 5-21 --------
 * Generate data for sort tests
 *
 * For usage, see main()
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void main ( int argc, char *argv[] )
{
    int i, from, to, step;

    if ( argc != 3 || strchr ( "fFrRbBuU?", *argv[1] ) == NULL)
    {
        printf ( "usage: makedata [f|r|b|?] n > outfile\n"
                 "   f = forward-ordered\n"
                 "   r = reverse-ordered\n"
                 "   u = up and down-ordererd\n"
                 "   b = binary (mixed ones and zeros)\n"
                 "   ? = random-ordered\n" );

        return;
    }

    if ( *argv[1] == '?' )
    {
        to = atoi ( argv[2] );

        for ( i = 0; i != to; i++ )
            printf ( "%05d\n", abs ( rand() ));

        return;
    }

    if ( tolower ( *argv[1] ) == 'b' )
    {
        to = atoi ( argv[2] );

        for ( i = 0; i != to; i++ )
            printf ( "%05d\n", abs(rand()) % 2 );

        return;
    }

    if ( tolower ( *argv[1] ) == 'u' )
    {
        to = atoi ( argv[2] ) / 2;
        for ( i = 0; i < to; i++ )
            printf ( "%05d\n", i );
        for ( i = to; i >= 0; i-- )
            printf ( "%05d\n", i );

        return;
    }

    if ( tolower ( *argv[1]) == 'f' )
        step = 1;
    else
        step = -1;

    i = atoi ( argv[2] );

    if ( step == 1 )
    {
        from = 0;
        to = i;
    }
    else
    {
        from = i;
        to = 0;
    }

    for ( i = from; i != to; i += step )
        printf ( "%05d\n", i );
}
