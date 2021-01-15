/*--- amrtmain.c -------------------------- Listing 8-22 -------
 * Loan amortization calculator. Enter the values for principal,
 * interest rate, and loan payment on the command line, where
 * -$ = principal, -r = rate, -p =payment. For example, a loan
 * of $250,000 at 7.5% with payments of $2900 is entered as:
 *
 *    amrtmain -r0.075  -$250000 -p2900
 *
 * Careful: minimal checking is done for entry errors. Under
 * UNIX the $ command line switch must be escaped: -\$5000
 * The program output consists of a table that shows the
 * outstanding principal, the amount of interest, and the
 * principal paid for each month of the loan.
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "longmath.h"

void ShowUsage ( void );

int main ( int argc, char *argv[] )
{
    struct TermData *principal, *rate, *payment;
    struct TermData *interest, *temp, *temp2;

    char buffer [7*MAX_SIZE];   /* buffer for output */

    int i, month;

    month      = 0;
    principal  = TermCreate();
    rate       = TermCreate();
    payment    = TermCreate();
    interest   = TermCreate();
    temp       = TermCreate();
    temp2      = TermCreate();

    /* try out various values */

    if ( argc != 4 )
    {
        ShowUsage();
        return ( EXIT_FAILURE );
    }

    for ( i = 1; i < argc; i++ )
    {
        char *p;
        if ( *argv[i] != '-' )
        {
            ShowUsage();
            return ( EXIT_FAILURE );
        }
        else
        {
            p = &argv[i][1];
            switch ( *p )
            {
                case '$':
                    AsciiToTerm ( p + 1, principal );
                    break;
                case 'p':
                    AsciiToTerm ( p + 1, payment );
                    break;
                case 'r':
                    AsciiToTerm ( p + 1, rate );
                    break;
                default:
                    fprintf ( stderr,
                                "Invalid switch: -%c\n", *p );
                    ShowUsage();
                    return ( EXIT_FAILURE );
            }
        }
    }

    /*
     * divide rate by 12 to get monthly rate,
     * then copy back to rate.
     */

    AsciiToTerm ( "12", temp );
    if ( ! ComputeResult ( rate, '/', temp, temp2 ))
    {
        printf ( "error 1\n" );
        return ( EXIT_FAILURE );
    }
    TermCopy ( rate, temp2 );

    /* print principal and monthly rate */

    memset ( buffer, ' ', MAX_SIZE * 7 );
    TermToAscii ( principal, buffer, NORMAL );

    buffer [strlen ( buffer )] = ' ';
    TermToAscii ( rate, buffer + MAX_SIZE * 2 + 3, NORMAL );
    buffer[strlen ( buffer )] = ' ';
    TermToAscii ( payment, buffer + MAX_SIZE * 4 + 3, NORMAL );

    printf (
        "Original Principal     Monthly Rate        Payment\n" );
    printf ( "%s\n\n", buffer );

    printf ( "Mnth Principal Left      Interest Paid" );
    printf ( "       Principal Paid\n" );

    /* print payments while principal > 0 */

    while ( principal->sign > 0 )
    {

        /* interest = principal * rate */

        if ( ! ComputeResult ( principal, '*', rate, interest ))
        {
            printf ( "error 2\n" );
            return ( EXIT_FAILURE );
        }

        /* temp = payment - interest */

        if ( ! ComputeResult ( payment, '-', interest, temp ))
        {
            printf ( "error 3\n" );
            return ( EXIT_FAILURE );
        }

        if ( temp->sign < 0 )
        {
            printf (
                "Monthly interest is > than monthly payment"
                "\nLoan will never be paid off\n" );
            return ( EXIT_FAILURE );
        }

        /* print month (same as payment number) */

        if (( month % 12 ) == 0 )
            printf ( "\n" );
        printf ( "%4d ", ++month );

        /* print: principal  interest  temp */

        memset ( buffer, ' ', MAX_SIZE * 7 );
        TermToAscii ( principal, buffer, NORMAL );
        buffer[strlen ( buffer )] = ' ';
        TermToAscii ( interest, buffer + MAX_SIZE * 2, NORMAL );
        buffer[strlen ( buffer )] = ' ';
        TermToAscii ( temp, buffer + MAX_SIZE * 4, NORMAL );
        printf ( "%s\n", buffer );

        /* principal = principal - temp */

        if ( ! ComputeResult ( principal, '-', temp, temp2 ))
        {
            printf ( "error 4\n" );
            return ( EXIT_FAILURE );
        }
        TermCopy ( principal, temp2 );
    }

    return ( EXIT_SUCCESS );
}

void ShowUsage ( void )
{
    printf (

   "Loan amortization calculator. Enter the principal,\n"
   "interest rate, and loan payment on the command line,\n"
   "where -$ = principal, -r = rate, -p =payment. For example,\n"
   "a loan of $250,000 at 7.5%% with payments of $2900 enter:\n\n"
   "   amrtmain -r0.075  -$250000 -p2900\n\n"
   "Note: UNIX users should enter the principal as -\\$250000.\n" );
}
