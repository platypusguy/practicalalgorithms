/*--- SOUNDEX.C ---------------------------- Listing 4-7 -------
*   Will generate the soundex code for a name or word.
*
*   Usage: int Soundex ( char *name, char *soundex_code )
*
*   if DRIVER is #defined a driver routine is compiled.
*-------------------------------------------------------------*/

#include <ctype.h>
#include <string.h>

#define DRIVER 1

int Soundex ( const char *str, char *soundex_out )
{

#define SOUNDEX_ERR 0
#define SOUNDEX_OK  1

    static const char table[] =

      /* ABCDEFGHIJKLMNOPQRSTUVWXYZ */
        "01230120022455012623010202";

    int count = 0;                  /* Valid code letters done */
    char this_char, prev_char;      /* Current and previous    */
    char code [5];                  /* will hold Soundex code  */

    strcpy ( code, "    " );        /* Initialize string       */

    this_char = toupper ( *str );   /* The first character is  */
    if ( isalpha ( this_char ))     /* preserved albeit as     */
    {                               /* uppercase.              */
        code[0] = this_char;
        count = 1;
        str += 1;
    }
    else
        return ( SOUNDEX_ERR );

    prev_char = ' ';               /* previous char empty now  */

    while ( count < 4 && isalpha ( *str ))
    {
        char c;

        this_char = toupper ( *str );
        c = table[this_char - 'A'];

       /*--------------------------------------------------------
        *  Remove 0's and repeated sequences from output
        *  and then insert the next character into the string.
        *------------------------------------------------------*/

        if ( c != prev_char && c != '0' )
        {
            code[count++] = c;
            prev_char = c;
        }
        str += 1;
    }

    /* Next, if it's neither '/0' nor a letter, it's an error */

    if ( *str != '\0' && ! ( isalpha( *str )))
        return ( SOUNDEX_ERR );
    else
    {
        strcpy ( soundex_out, code );
        return ( SOUNDEX_OK );
    }
}

#ifdef DRIVER         /* Driver to demo soundex results */

#include <stdio.h>
#include <stdlib.h>

int main ( int argc, char *argv[] )
{
    char code[5];

    if ( argc != 2 )
    {
        fprintf ( stderr,
                    "Error! Usage: soundex word-to-code\n" );
        return ( EXIT_FAILURE );
    }

    if ( Soundex ( argv[1], code) == 0 )
        printf ("%s is not a valid name or word\n", argv[1] );
    else
        printf ( "Soundex for %s is %s\n", argv[1], code );

    return ( EXIT_SUCCESS );
}
#endif
