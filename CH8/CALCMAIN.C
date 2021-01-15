/*--- calcmain.c -------------------------- Listing 8-1 --------
 * Calculator main line for longmath routines.
 *
 * If the program is invoked without any arguments, it responds
 * as an interactive command-line calculator, with all necessary
 * prompts on the screen. Otherwise:
 *
 * If one argument is supplied, it is taken to be a filename,
 * for a calculator script that may contain the following:
 * terms in the prescribed form,
 * the arithemetic operators: +, -, *, /
 * the directive 'C' which clears and resets the calculator to 0
 *
 * In the absence of a clear command, the result of the previous
 * operation is carried over into the first term.
 *
 * If a second file is specified, all terms and operations and
 * their results are written to it; otherwise, the output file
 * is longmath.out
 *------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "longmath.h"

int main ( int argc, char *argv[] )
{
    struct TermData *term1,     /* two terms and the solution */
                    *term2,
                    *solution;

    int operator;
    int format  = NORMAL;       /* default to NORMAL notation */

    char buffer [2*MAX_SIZE];   /* buffer for user input */

    /*--- allocate and initialize the terms ---*/

    term1    = TermCreate();
    term2    = TermCreate();
    solution = TermCreate();

    if ( term1 == NULL || term2 == NULL || solution == NULL )
    {
        fprintf ( stderr, "Cannot allocate memory\n" );
        return ( EXIT_FAILURE );
    }

    /*--- the interactive calculator ---*/

    if ( argc < 2 )     /* no input file specified, hence  */
    {                   /*  it must be interactive.        */

        printf ( "Arbitrary-Precision Math. Press Q to Quit\n" );

        /* get the first term */

        printf ( "Type first term and press <Enter>\n" );
        gets ( buffer );
        if ( *buffer == 'q' || *buffer == 'Q' )
            return ( EXIT_SUCCESS );
        else
        {
            format = AsciiToTerm ( buffer, term1 );
            if ( ! format )
            {
                printf ( "Invalid term; aborting!\n" );
                exit ( EXIT_FAILURE );
            }
        }

        /* get the second term */

        printf ( "Type second term and press <Enter>\n" );
        gets ( buffer );
        if ( *buffer == 'q' || *buffer == 'Q' )
            return ( EXIT_SUCCESS );
        else
        {
            format = AsciiToTerm ( buffer, term2 );
            if ( ! format )
            {
                printf ( "Invalid term; aborting!\n" );
                exit ( EXIT_FAILURE );
            }
        }

        /* get the desired operation */

        printf (
          "Type operation ( + - * / ) and press <Enter>\n" );

        operator = getchar();
        if ( operator == 'q' || operator == 'Q' )
            return ( EXIT_SUCCESS );

        if ( operator != '+' && operator != '-' &&
             operator != '*' && operator != '/' )
        {
            printf ( "Invalid operator; aborting!\n" );
            exit ( EXIT_FAILURE );
        }

        /*--- compute the result --- */

        if ( ! ComputeResult (
                    term1, operator, term2, solution ))
         return ( EXIT_FAILURE );

        /*--- print the answer ---*/

        TermToAscii ( solution, buffer, format );

        printf ( "%s\n", buffer );

        return ( EXIT_SUCCESS );
    }
    else    /* calculator operations in a file */
    {
        FILE *fin, * fout;            /* file handles */
        char fin_name[64],            /* file names */
             fout_name[64];

        strcpy ( fin_name, argv[1] );
        if ( argc > 2 )
            strcpy ( fout_name, argv[2] );
        else
            strcpy ( fout_name, "longmath.out" );

        fin = fopen ( fin_name, "rt" );
        if ( fin == NULL )
        {
            fprintf ( stderr, "Error opening: %s\n", fin_name );
            return ( EXIT_FAILURE );
        }

        fout = fopen ( fout_name, "wt" );
        if ( fout == NULL )
        {
            fprintf ( stderr, "Error opening: %s\n", fout_name );
            return ( EXIT_FAILURE );
        }

        while ( ! feof ( fin ))
        {
            /* get the first term */

            format =  GetFileTerm ( term1, fin, fout );
            if ( ! format )
                break;

            /* get the operator */

            if ( ! GetFileOperator ( &operator, fin, fout ))
                break;

            continuing:  /* we'll jump to here if further ops */

            /* get next term */

            format =  GetFileTerm ( term2, fin, fout );
            if ( ! format )
                break;

            /*
             * if an error occurs in computing, abort *unless*
             * the next operation is a 'clear,' which starts
             * a new calculation.
             */

            if ( ! ComputeResult (
                        term1, operator, term2, solution ))
            {
                fprintf ( stderr, "Error in Computing.\n" );
                fprintf ( fout, "Error in Computing.\n" );

                if ( ! GetFileOperator ( &operator, fin, fout ))
                {
                    fprintf ( fout, "Aborting...\n" );
                    return ( EXIT_FAILURE );
                }
                else
                if ( operator != 'C')
                {
                    fprintf ( fout, "Aborting...\n" );
                    return ( EXIT_FAILURE );
                }
                else    /* next operation was 'clear' */
                {
                    TermInit ( term1 );
                    TermInit ( term2 );
                    TermInit ( solution );
                    continue;
                }
            }
            else
            {
                /* if no error, print the solution */

                TermToAscii ( solution, buffer, format );
                fprintf ( fout, "=\n%s\n", buffer );

                /* get the next operation */

                if ( ! GetFileOperator ( &operator, fin, fout ))
                    break;

                if ( operator == 'C' )  /* clear = reinit */
                {
                    TermInit ( term1 );
                    TermInit ( term2 );
                    TermInit ( solution );
                    continue;
                }
                else                     /* keep going */
                {
                    TermCopy ( term1, solution );
                    TermInit ( term2 );
                    TermInit ( solution );
                    goto continuing;
                }
            }
        }
        fclose ( fin );
        fclose ( fout );
    }
    return ( EXIT_SUCCESS );
}

/*--------------------------------------------------------------
 * Get an operator from the input file and check it for errors;
 * write a copy of it to outfile. Returns 0 on error, else 1.
 *------------------------------------------------------------*/
int GetFileOperator ( int *operator,
                      FILE * infile, FILE *outfile )
{
    char buffer [ MAX_SIZE + 4 ];
    char *p;

    if ( fgets ( buffer, MAX_SIZE + 4, infile ) == NULL )
        return ( 0 );

    /* replace the CR/LF with a null; check length */

    p = strchr ( buffer, '\n' );
    if ( p == NULL )
    {
        printf ( "Error: invalid operator :\n%s\n", buffer );
        fprintf ( outfile,
                  "Error: invalid operator :\n%s\n", buffer );
        return ( 0 );
    }
    else
        *p = '\0';

    *operator = *buffer;

    if ( *( buffer + 1 ) != '\0' ||
            ( *operator != '+' &&
              *operator != '-' &&
              *operator != '*' &&
              *operator != '/' &&
              *operator != 'c' &&
              *operator != 'C' ))
    {
        printf ( "Error: Invalid operator %c\n", *operator );
        fprintf ( outfile,
                  "Error: invalid operator :\n%s\n", buffer );
        return ( 0 );
    }

    if ( *operator == 'c' )
        *operator = 'C';

    fprintf ( outfile, "%c\n", *operator );

    return ( 1 );
}

/*--------------------------------------------------------------
 * Get a term from the input file and check it for errors;
 * write a copy of it to outfile. Returns 0 on error, else
 * returns SCIENTIFIC or NORMAL, indicating the number format.
 *------------------------------------------------------------*/
int GetFileTerm ( struct TermData * t,
                  FILE * infile, FILE *outfile )
{
    char buffer [ MAX_SIZE + 4 ];
    char *p;
    int  format = NORMAL;

    if ( fgets ( buffer, MAX_SIZE + 4, infile ) == NULL )
        return ( 0 );   /* EOF */

    /* replace the CR/LF with a null; check length */

    p = strchr ( buffer, '\n' );
    if ( p == NULL )
    {
        printf ( "Error: term too long:\n%s\n", buffer );
        fprintf ( outfile,
                  "Error: term too long:\n%s\n", buffer );
        return ( 0 );
    }
    else
        *p = '\0';

    /* convert to term and check length and errors */

    format = AsciiToTerm ( buffer, t );
    if ( t->sign == 0 ||
         ( t->places_before + t->places_after > MAX_SIZE ))
    {
        printf ( "Error: term too long or invalid:\n%s\n",
                   buffer );
        fprintf ( outfile,
                  "Error: term too long or invalid:\n%s\n",
                   buffer );
        return ( 0 );
    }

    fprintf ( outfile, "%s\n", buffer );
    return ( format );
}
