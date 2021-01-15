/*--- braces.c ---------------------------- Listing 2-8 ---------
 * Checks that braces, brackets, and parentheses are properly
 * paired in a C program. If they're not, an error message
 * is printed to stderr saying at what line the unmatched
 * opening item is found. Uses a stack.
 *
 * Usage: braces filename.ext
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stacks.h"

int main ( int argc, char *argv[] )
{
    FILE *fin;                  /* file we'll be reading from */
    char buffer[128];           /* read file into this buffer */

    int line_count;             /* current line count */
    struct StkElement *stk_el;  /* scratch stack element */
    Stack *stk;                 /* the stack we will use */
    char ch;                    /* character we're examining */
    int i;                      /* for loop count */

    if ( argc != 2 )
    {
        fprintf ( stderr, "Usage: braces filename.ext\n" );
        exit ( EXIT_FAILURE );
    }

    fin = fopen ( argv[1], "rt" );
    if ( fin == NULL )
    {
        fprintf ( stderr, "Cannot open/find %s\n", argv[1] );
        exit ( EXIT_FAILURE );
    }

    /* Create and initialize the stack */

    stk = CreateStack ( 40 );   /* create a stack of 40 items */
    if ( stk == NULL )
    {
        fprintf ( stderr, "Insufficient Memory\n" );
        exit ( EXIT_FAILURE );
    }

    /* Create the scratch stack element */

    stk_el = (struct StkElement *)
                malloc ( sizeof ( struct StkElement ));
    if ( stk_el == NULL )
    {
        fprintf ( stderr, "Insufficient memory\n" );
        exit ( EXIT_FAILURE );
    }

    line_count = 0;

    while ( ! feof ( fin ))
    {
        /* read a line of a C program */
        if ( fgets ( buffer, 127, fin ) == NULL )
            break;

        line_count += 1;

        /* get rid of the trailing carriage return */
        buffer [ strlen ( buffer ) - 1 ] = '\0';

        /* scan and process braces, brackets, and parentheses */
        for ( i = 0; buffer[i] != '\0'; i++ )
        {
            switch ( ch = buffer[i] )
            {
            case '(':
            case '[':
            case '{':
                stk_el->opener  = ch;
                stk_el->line_no = line_count;
                if ( ! PushElement ( stk, stk_el ))
                {
                    fprintf ( stderr, "Out of stack space\n" );
                    exit ( EXIT_FAILURE );
                }
                break;
            case ')':
            case ']':
            case '}':
                if ( ! PopElement ( stk, stk_el ))
                    fprintf ( stderr, "Stray %c at line %d\n",
                                ch,  line_count );
                else
                if (( ch == ')'&& stk_el->opener != '(' ) ||
                    ( ch == ']'&& stk_el->opener != '[' ) ||
                    ( ch == '}'&& stk_el->opener != '{' ))
                    fprintf ( stderr,
                      "%c at line %d not matched by %c at line %d\n",
                       ch, line_count,
                       stk_el->opener, stk_el->line_no );
                break;
            default:
                continue;
            }
        }
    }

    /*  We are at the end of file. Are there unmatched items? */

    if ( ViewElement ( stk, 0 ) != NULL )
        while ( PopElement ( stk, stk_el ) != 0 )
            fprintf ( stderr, "%c from line %d unmatched\n",
                        stk_el->opener, stk_el->line_no );

    fprintf ( stderr, "Error checking complete\n" );

    fclose ( fin );
    return ( EXIT_SUCCESS );
}
