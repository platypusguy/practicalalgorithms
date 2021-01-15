/*--- lilcal.c ---------------------------- Listing 7-1 --------
 * lilcal works similarly to the UNIX cal program.
 * If you type lilcal with no arguments, it prints the calendar
 * for the current month; otherwise, provide it with two
 * arguments: the month and year. Hence, lilcal 9 56 will print
 * the calendar for September 1956. October 1582 is hardwired as
 * a special case.
 *
 * UNIX cal allows you to print the calendar for a whole year.
 * Since this ability is cosmetic rather than algorithmic, it
 * is not included here.
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "datelib.h"

void PrintCalendar ( int, int );
void PrintOct1582 ( void );

int main ( int argc, char *argv[] )
{
    int start_yy, start_mm;

    time_t timer;
    struct tm *tms;

    if ( argc != 1 && argc != 3 )
    {
        fprintf ( stderr, "lilcal requires 0 or 2 arguments\n" );
        return ( EXIT_FAILURE );
    }

    if ( argc == 1 )    /* current month */
    {
        timer = time ( NULL );
        tms   = localtime ( &timer );
        tms->tm_mon += 1;

        start_mm = tms->tm_mon;
        start_yy = tms->tm_year;
        start_yy += 1900;
    }
    else
    if ( argc == 3 )
    {
        start_mm = atoi ( argv[1] );
        start_yy = atoi ( argv[2] );
    }

    /*
     * due to the calendar change, October 1582 is a unique
     * month and must be provided for specially.
     */

    if ( start_yy == 1582 && start_mm == 10 )
        PrintOct1582();
    else
        PrintCalendar ( start_yy, start_mm );

    return ( EXIT_SUCCESS );
}

void PrintCalendar ( int cal_yy, int cal_mm )
{
    char      buffer [40];
    struct tm date;
    int       day_of_week;
    int       days_in_month;
    int       days_to_print[42];
    int       i, j, rows;

    if ( LoadDateStruct ( &date, cal_yy, cal_mm, 1 ) == -1 )
    {
        fprintf ( stderr, "Illegal month: %02d-%02d\n",
                    cal_yy, cal_mm );
        return;
    }

    /* get the day of the week for the 1st of month */

    day_of_week = DayOfWeek ( &date );

    /* find out how many days in the month */

    switch ( cal_mm )
    {
        case  1:
        case  3:
        case  5:
        case  7:
        case  8:
        case 10:
        case 12:
            days_in_month = 31;
            break;

        case  4:
        case  6:
        case  9:
        case 11:
            days_in_month = 30;
            break;

        case  2:
            days_in_month = 28;
            if ( IsYearLeap ( cal_yy ) == YES )
                days_in_month += 1;
            break;
    }

    /* load up the days to print */

    for ( i = 0; i < 42; i++ )
        days_to_print[i] = 0;

    for ( i = day_of_week, j = 1; j <= days_in_month; i++, j++ )
        days_to_print[i] = j;

    /* now print the calendar */

    TimeSprintf ( buffer, 40, "    %B %Y", &date );
    printf ( "%s\n", buffer );
    printf ( "  S  M Tu  W Th  F  S\n" );

    for ( rows = 0; rows < 6; rows++ )  /* print up to 6 rows */
    {                                   /* of 7 days.         */
        for ( i = 0; i < 7; i++ )
            if ( days_to_print[(rows * 7) + i] == 0 )
                printf ( "   " );
            else
                printf ( " %2d",
                            days_to_print[(rows * 7) + i] );
        printf ( "\n" );
    }
}

void PrintOct1582 ( void )
{
    printf ( "    October 1582\n" );
    printf ( "  S  M Tu  W Th  F  S\n" );
    printf ( "     1  2  3  4 15 16\n" );
    printf ( " 17 18 19 20 21 22 23\n" );
    printf ( " 24 25 26 27 28 29 30\n" );
    printf ( " 31\n" );
}
