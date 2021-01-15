/*--------------------------------------------------------------
 * Library of date routines v. 1.1
 *
 * 2000-01-15 TimeSprintf() corrected. Bug in printing years
 *            with embedded zeros. See 'Y' option.
 *-------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>

#include "datelib.h"

// #define DRIVER 1

static const int Days_in_month [13] = {
    0,
    31, 28, 31,     /* Jan, Feb, Mar */
    30, 31, 30,     /* Apr, May, Jun */
    31, 31, 30,     /* Jul, Aug, Sep */
    31, 30, 31      /* Oct, Nov, Dec */
};

/* same as above but for a leap year */

static const int Days_in_month_leap [13] = {
    0,
    31, 29, 31,     /* Jan, Feb, Mar */
    30, 31, 30,     /* Apr, May, Jun */
    31, 31, 30,     /* Jul, Aug, Sep */
    31, 30, 31      /* Oct, Nov, Dec */
};

static const char *Day_name[7] = {
    "Sunday",   "Monday",
    "Tuesday",  "Wednesday",
    "Thursday", "Friday",
    "Saturday"
};

static const char *Day_name_abbrev[7] = {
    "Sun",  "Mon",
    "Tue",  "Wed",
    "Thu",  "Fri",
    "Sat"
};

static const char *Month_name[13] = {
    "",
    "January",  "February", "March",
    "April",    "May",      "June",
    "July",     "August",   "September",
    "October",  "November", "December"
};

static const char *Month_name_abbrev[13] = {
    "",
    "Jan",  "Feb",  "Mar",
    "Apr",  "May",  "Jun",
    "Jul",  "Aug",  "Sep",
    "Oct",  "Nov",  "Dec"
};


#ifndef YES
#define YES 1
#define NO  0
#endif


/*--------------------------------------------------------------
 * Accepts pointers to two date structures and returns the
 * difference in days between them as a long int. Negative if
 * the first date is earlier, positive if it is later. No error
 * return is possible, so programmers must make sure the dates
 * are valid.
 *-------------------------------------------------------------*/
long DateDiff ( struct tm *d1, struct tm *d2 )
{
    return ( DateToDayNumber ( d1 ) - DateToDayNumber ( d2 ));
}

/*--------------------------------------------------------------
 * Returns a long which contains the number of days since
 * Jan 1, AD 1; returns -1L on error. See text for observations
 * on the use of this function.
 *-------------------------------------------------------------*/
long DateToDayNumber ( struct tm * date )
{
    long day_num;
    int  mm, yy;

    if ( IsDateValid ( date ) == NO )
        return ( -1L );

    yy = date->tm_year;
    mm = date->tm_mon + 1;

    /* get all the regular days in preceding years */

    day_num =  ( yy - 1 ) * 365L;

    /* now get the leap years */

    day_num += ( yy - 1 ) / 4L;

    /*
     * now back out the century years that are not leap:
     * this would be all century years that are
     * not evenly divisible by 400: 1700, 1800, 1900, 2100...
     */

    day_num -= ( yy - 1 ) / 100L;
    day_num += ( yy - 1 ) / 400L;

    /*
     * before 1582 all century years were leap, so adjust for
     * this. If year is > 1582, then just add 12 days for years
     * 100, 200, 300, 500, 600, 700, 900, 1000, 1100, 1300, 1400
     * and 1500. Otherwise, calculate it.
     */

    if ( yy - 1 > 1582L )
        day_num += 12L;
    else
    {
        day_num += ( yy - 1 ) / 100L;
        day_num -= ( yy - 1 ) / 400L;
    }

    /* now, add the days elapsed in the year so far */

    if ( IsYearLeap ( date->tm_year ) == NO )
        while ( --mm )
            day_num += Days_in_month[ mm ];
    else
        while ( --mm )
            day_num += Days_in_month_leap[ mm ];

    /* add days in current month for the year being evaluated */

    day_num += date->tm_mday;

    /*
     * now adjust for the 10 days cut out of the calendar when
     * the change was made to the Gregorian calendar. This change
     * reflects the jump from October 4 to October 15, 1582, a
     * deletion of 10 days.
     */

    if ( day_num > 577737L )
        day_num -= 10L;

    return ( day_num );
}

/*--------------------------------------------------------------
 * Converts a day number into an actual date.
 *-------------------------------------------------------------*/
int DayNumberToDate ( long int day_num, struct tm *date )
{
    int     dd, mm, yy, i;
    long    days_left;

    if ( day_num > 577737L )
        day_num += 10L;

    yy = (int) ( day_num / 365 );
    days_left = day_num % 365L;

    /* prior to 1700, all years evenly divisible by 4 are leap */

    if ( yy < 1700 )
        days_left -= ( yy / 4 );
    else
    {
        days_left -= ( yy / 4 );    /* deduct leap years */
        days_left += ( yy / 100 );  /* add in century years */
        days_left -= ( yy / 400 );  /* deduct years / 400 */
        days_left -= 12;            /* deduct century years that
                                       were leap before 1700 */
    }

    /* make sure days left is > 0 */

    while ( days_left <= 0 )
    {
        if ( IsYearLeap ( yy ) == YES )
        {
            yy -= 1;
            days_left += 366;
        }
        else
        {
            yy -= 1;
            days_left += 365;
        }
    }

    /*
     * yy holds the number of elapsed years.
     * So, add 1 for the current year.
     */

    yy += 1;

    /*
     * now deduct the days in each month, starting
     * from January to find month and day of month.
     * adjust for leap year, of course.
     */

    dd = (int) days_left;
    mm = 0;

    if ( IsYearLeap ( yy ) == YES )
        for ( i = 1; i <= 12; i++ )
        {
            mm = i;
            if ( dd <= Days_in_month_leap[i] )
                break;
            else
                dd -= Days_in_month_leap[i];
        }
    else
        for ( i = 1; i <= 12; i++ )
        {
            mm = i;
            if ( dd <= Days_in_month[i] )
                break;
            else
                dd -= Days_in_month[i];
        }

    if ( LoadDateStruct ( date, yy, mm, dd ) == -1 )
        return ( -1 );
    else
        return ( 0 );
}

/*--------------------------------------------------------------
 * Returns an integer from 0-6 signifying the day of the week,
 * where 0 = Sunday and 6 = Saturday. Returns -1 on error.
 *-------------------------------------------------------------*/
int DayOfWeek ( struct tm *date )
{
    long day_num = DateToDayNumber ( date );

    if ( day_num < 0 )
        return ( -1 );

    return ( (int) (( day_num % 7 ) + 5 ) % 7 );
}

/*--------------------------------------------------------------
 * Returns an integer from 1-366 signifying the day of the year.
 * where 1 is Jan 1, 365 or 366 is Dec 31, and -1 is an error.
 *-------------------------------------------------------------*/
int DayOfYear ( struct tm *date )
{
    struct tm jan1;

    if ( LoadDateStruct ( &jan1, date->tm_year, 1, 1 ) == -1 )
        return ( -1 );

    return ((int) ( DateDiff ( date, &jan1 ) + 1 ));
}

/*--------------------------------------------------------------
 * Returns YES if date is valid, otherwise returns NO.
 *-------------------------------------------------------------*/
int IsDateValid ( struct tm *date )
{
    if ( date->tm_mon < 0 || date->tm_mon > 11 )   /* per ANSI */
        return ( NO );

    if ( date->tm_year < 1 )
        return ( NO );

    if ( IsYearLeap ( date->tm_year ) == NO )
        if ( date->tm_mday < 1 ||
             date->tm_mday > Days_in_month[ date->tm_mon + 1 ] )
            return ( NO );
        else
        if ( date->tm_mday < 1 ||
             date->tm_mday >
                        Days_in_month_leap[ date->tm_mon + 1] )
            return ( NO );

    return ( YES );
}

/*--------------------------------------------------------------
 * Returns YES if time is valid, otherwise returns NO.
 *-------------------------------------------------------------*/
int IsTimeValid ( struct tm * date )
{
    if ( date->tm_hour < 0 || date->tm_hour > 23 )
        return ( NO );

    if ( date->tm_min < 0 || date->tm_min > 59 )
        return ( NO );

    /*
     * Technically speaking, ANSI allows for minutes with
     * 61 or 62 seconds for leap seconds that are added
     * with little warning by astronomical societies to
     * reflect slight changes in the earth's rotation.
     * For reasons discussed in the text, however, we
     * observe only the 60-second minute.
     */

    if ( date->tm_sec < 0 || date->tm_sec > 59 )
        return ( NO );

    return ( YES );
}

/*--------------------------------------------------------------
 * Returns YES or NO depending on whether a year is leap or not
 *-------------------------------------------------------------*/
int IsYearLeap ( int year )
{
    if ( year % 4 != 0 )    /* if year not divisible by 4... */
        return ( NO );      /* it's not leap */

    if ( year < 1582 )      /* all years divisible by 4 were */
        return ( YES );     /* leap prior to 1582            */

    if ( year % 100 != 0 )  /* if year divisible by 4, */
        return ( YES );     /* but not by 100, it's leap */

    if ( year % 400 != 0 )  /* if year divisible by 100, */
        return ( NO );      /* but not by 400, it's not leap */
    else
        return ( YES );     /* if divisible by 400, it's leap */
}

/*--------------------------------------------------------------
 * Loads a date passed as three integers into a tm structure.
 * Returns 0 on success, -1 on error.
 *-------------------------------------------------------------*/
int LoadDateStruct ( struct tm *date, int yy, int mm, int dd )
{
    date->tm_year = yy;
    date->tm_mon  = mm - 1; /* for compatibility w/ ANSI C */
    date->tm_mday = dd;

    if ( IsDateValid ( date ) == NO )
        return ( -1 );
    else
        return ( 0 );
}

/*--------------------------------------------------------------
 * Loads time into an existing tm structure.
 * Returns 0 on success, -1 on error
 *-------------------------------------------------------------*/
int LoadTimeStruct ( struct tm *date, int hh, int mm, int ss )
{
    date->tm_hour = hh;
    date->tm_min  = mm;
    date->tm_sec  = ss;

    if ( IsTimeValid ( date ) == NO )
        return ( -1 );
    else
        return ( 0 );
}

/*--------------------------------------------------------------
 * TimeDiff() operates like difftime(), however it can accept
 * times before Jan 1, 1900. No error return.
 *-------------------------------------------------------------*/
double TimeDiff ( struct tm *t1, struct tm *t2 )
{
    long day_num;
    double d1, d2;

    day_num = DateToDayNumber ( t1 );
    d1 = day_num * 86400.0 + t1->tm_hour * 3600.0 +
         t1->tm_min * 60.0 + t1->tm_sec;

    day_num = DateToDayNumber ( t2 );
    d2 = day_num * 86400.0 + t2->tm_hour * 3600.0 +
         t2->tm_min * 60.0 + t2->tm_sec;

    return ( d1 - d2 );
}

/*--------------------------------------------------------------
 * Operates like strftime() except:
 *  a) it returns -1 on error;
 *  b) strftime() expects tm_year to be 2 digits and so always
 *     adds 1900 to the year, while TimeSprintf() believes the
 *     year to be the actual year: 94 is AD 94 and so does not
 *     add 1900.
 *-------------------------------------------------------------*/
int TimeSprintf ( char *string, int max_size,
                  char *format, struct tm *ptm )
{
    char *pf;
    int  i, j;

    int  yy;        /* scratch variable for year */
    int  mon;       /* our internal use of the month is 1-12 */
    char *buffer;   /* where the output string goes */

	int writing_year; /* fixes bug in 'Y' option. 2000-01-15 */

    mon = ptm->tm_mon + 1;

    buffer = malloc ( 1024 );   /* largest likely output string */
    if ( buffer == NULL )
        return ( -1 );

    i = 0;    /* where we are in the output string */


    for ( pf = format; *pf != '\0' && i < 1024; pf++ )
    {
        if ( *pf != '%' )
            buffer[i++] = *pf;
        else
            switch ( *(++pf) )
            {
                /*--- day of the week: Sunday, Monday...---*/

            case 'a':               /* 3-letter weekday */
                j = DayOfWeek ( ptm );
                strncpy ( buffer + i,
                          Day_name_abbrev[j], 3 );
                i += 3;
                break;

            case 'A':               /* full weekday */
                j = DayOfWeek ( ptm );
                strcpy ( buffer + i, Day_name[j] );
                while ( *(buffer + i) != '\0' )
                    i++;
                break;

            case 'w':               /* weekday as a digit */
                j = DayOfWeek ( ptm );
                buffer[i++] = j + '0';
                break;

                /*--- day of the month: 1, 2, 3...---*/

            case 'd':               /* day as a number */
                buffer[i++] = ptm->tm_mday / 10 + '0';
                buffer[i++] = ptm->tm_mday % 10 + '0';
                break;

            case 'm':               /* month as a number */
                buffer[i++] =  mon  / 10 + '0';
                buffer[i++] =  mon  % 10 + '0';
                break;

                /*--- month ---*/

            case 'b':               /* 3-letter month */
                strncpy ( buffer + i,
                          Month_name_abbrev[mon], 3 );
                i += 3;
                break;

            case 'B':               /* full month name */
                strcpy ( buffer + i, Month_name[mon] );
                while ( *(buffer + i) != '\0' )
                    i++;
                break;

                /*--- year ---*/

            case 'Y':               /* year with century: 1994 */
                yy = ptm->tm_year;
				writing_year = NO;

                if ( yy < 0 )
                {
                    buffer[i++] = '-';
                    yy = -yy;
                }

                if ( yy > 9999 )
                {
                    buffer[i++] = yy / 10000 + '0';
					writing_year = YES;
                    yy %= 10000;
                }

                if ( yy > 999 )
                {
                    buffer[i++] = yy / 1000 + '0';
					writing_year = YES;
                    yy %= 1000;
                }
				else
				if ( writing_year == YES )
					buffer[i++] = '0';

                if ( yy > 99 )
                {
                    buffer[i++] = yy / 100 + '0';
					writing_year = YES;
                    yy %= 100;
                }
				else
				if ( writing_year == YES )
					buffer[i++] = '0';

                buffer[i++] = yy / 10 + '0';
                buffer[i++] = yy % 10 + '0';
                break;

            case 'y':               /* year without century: 94 */
                yy = ptm->tm_year;

                if ( yy < 0 )
                {
                    buffer[i++] = '-';
                    yy = -yy;
                }

                if ( yy > 100 )
                    yy %= 100;

                buffer[i++] = yy / 10 + '0';
                buffer[i++] = yy % 10 + '0';
                break;

            case 'x':               /* locale-specific date */
            {
                char s[18];

                TimeSprintf ( s, 18, "%a, %b %d, %Y", ptm );
                strcpy ( buffer + i, s );
                i += strlen ( s );
            }
                break;

                /*--- time in numbers ---*/

            case 'I':               /* hour on 12-hr clock */
            {
                int hour;

                if ( ptm->tm_hour > 12 )
                    hour = ptm->tm_hour - 12;
                else
                    hour = ptm->tm_hour;

                buffer[i++] = hour / 10 + '0';
                buffer[i++] = hour % 10 + '0';
            }
                break;

            case 'H':               /* hour on 24-hr clock */
                buffer[i++] = ptm->tm_hour / 10 + '0';
                buffer[i++] = ptm->tm_hour % 10 + '0';
                break;

            case 'M':               /* minute as a number */
                buffer[i++] = ptm->tm_min / 10 + '0';
                buffer[i++] = ptm->tm_min % 10 + '0';
                break;

            case 'S':               /* seconds as a number */
                buffer[i++] = ptm->tm_sec / 10 + '0';
                buffer[i++] = ptm->tm_sec % 10 + '0';
                break;

            case 'X':               /* time string: hh:mm:ss */
            {
                char s[9];

                TimeSprintf ( s, 9, "%H:%M:%S", ptm );
                strcpy ( buffer + i, s );
                i += 8;
            }
                break;

                /*--- miscellaneous ---*/

            case 'c':               /* date and time */
            {
                char s[28];

                TimeSprintf ( s, 28, "%b %d %H:%M:%S %Y", ptm );
                strcpy ( buffer + i, s );
                i += strlen ( s );
            }
                break;

            case 'j':               /* day of the year: 1-366 */
            {
                int day_of_year;

                day_of_year = DayOfYear ( ptm );

                if ( day_of_year < 10 )
                    buffer[i++] = day_of_year + '0';
                else
                {
                    if ( day_of_year > 99 )
                    {
                        buffer[i++] = day_of_year / 100 + '0';
                        day_of_year %= 100;
                    }

                    buffer[i++] = day_of_year / 10 + '0';
                    buffer[i++] = day_of_year % 10 + '0';
                }
            }
                break;

            case 'p':               /* AM or PM */
                if ( ptm->tm_hour < 12 )
                    buffer[i++] = 'A';
                else
                    buffer[i++] = 'P';
                buffer[i++] = 'M';
                break;

            case 'U':               /* Sunday week of year */
            case 'W':               /* Monday week of year */
            {
                int day_of_year,
                    day_of_week,
                    sunday_week;

                struct tm jan1;

                int jan1_dow;       /* jan 1 day of week */

                day_of_year = DayOfYear ( ptm );
                day_of_week = DayOfWeek ( ptm );

                LoadDateStruct ( &jan1, ptm->tm_year, 1, 1 );
                jan1_dow = DayOfWeek ( &jan1 );

                if ( jan1_dow != 0 )
                    day_of_year -= ( 7 - jan1_dow );

                if ( day_of_year < 1 )
                    sunday_week = 0;
                else
                {
                    sunday_week = 1;
                    sunday_week += ( day_of_year - 1 ) / 7;
                }

                if ( *pf == 'W' )   /* if Monday week */
                    if ( day_of_week < 1 )
                        sunday_week -= 1;

                if ( sunday_week > 9 )
                {
                    buffer[i++] = ( sunday_week / 10 ) + '0';
                    sunday_week %= 10;
                }

                buffer[i++] = sunday_week + '0';
            }
                break;

            case 'Z':               /* the time zone */
            {
                char *temp;

                temp = setlocale ( LC_TIME, NULL );
                if ( temp == NULL )
                    buffer[i++] = 'C';
                else
                {
                    strcpy ( buffer + i, temp );
                    i += strlen ( temp );
                }
            }
                break;

            case '%':               /* the percent sign */
                buffer[i++] = '%';
                break;

            default:
                strcpy ( buffer + i, "Error in TimeSprintf() " );
                i += 23;
            }
    }
    buffer[i] = '\0';

    strncpy ( string, buffer, max_size );
    free ( buffer );

    if ( i <= max_size )
        return ( i );
    else
        return ( 0 );
}

#ifdef DRIVER

int script_date_to_tms ( char *, struct tm * );
int script_time_to_tms ( char *, struct tm * );

int main ( int argc, char *argv[] )
{
    FILE *infile;

    char *p, str[40], str2[40];
    int  action, i;
    long day_num;

    time_t timer;
    struct tm *tms,
              *tms2;

    struct tm tm1, tm2;

    tms = &tm1;
    tms2= &tm2;

    if ( argc < 2 )     /* if no args, just print the date */
    {
        timer = time ( NULL );
        tms   = localtime ( &timer );

        TimeSprintf ( str, 40,
                "%m-%d-%Y at %H:%M:%S", tms );
        printf ( "Today is %s\n", str );

        return ( EXIT_SUCCESS );
    }

    if ( argc == 3 )
    {
        long start, end, test, count;
        start = atol ( argv[1] );
        end   = atol ( argv[2] );

        if ( start < 1 || ( start + end ) < 1 )
        {
            printf ( "Usage: datelib start-day-num "
                     "how-many-dates\n" );
            return ( 0 );
        }

        count = 0L;
        day_num = start;

        while ( 1 )
        {
            DayNumberToDate ( day_num, tms );
            test = DateToDayNumber ( tms );
            if ( test != day_num )
            {
                count += 1;
                printf ( "%ld is %d-%d-%d but becomes %ld\n",
                            day_num,
                            tms->tm_mon, tms->tm_mday,
                            tms->tm_year,
                            test );
            }

            if ( start < end )
            {
                if ( day_num < end )
                    day_num += 1;
                else
                    break;
            }
            else
            if ( start > end )
            {
                if ( day_num > end )
                    day_num -= 1;
                else
                    break;
            }
        }

        printf ( "\n%01ld error(s) occurred\n", count );

        return ( 0 );
    }

    /* only valid arg is a test script; so read and play it */

    infile =  fopen ( argv[1], "rt" );
    if ( infile == NULL )
    {
        printf ( "Cannot open %s\n", argv[1] );
        return ( EXIT_FAILURE );
    }

    while ( ! feof ( infile ))
    {
        if ( fgets ( str, 40, infile ) == NULL )
            break;

        action = *str;
        p = str + 1;

        switch ( action )
        {
            case ';':   /* comment line */
                        continue;

            case 'a':   /* exercise TimeSprintf() */
            case 'A':
                        i = script_date_to_tms ( p, tms );
                        if ( i < 0 )
                            continue;
                        p++;
                        while ( *p != ' ' && *p != '\n' )
                            p++;
                        i = script_time_to_tms ( p, tms );
                        if ( i < 0 )
                            continue;

                        tms2->tm_year = tms->tm_year - 1900;
                        tms2->tm_mon  = tms->tm_mon;
                        tms2->tm_mday = tms->tm_mday;
                        tms2->tm_hour = tms->tm_hour;
                        tms2->tm_min  = tms->tm_min;
                        tms2->tm_sec  = tms->tm_sec;
                        mktime ( tms2 );

                        printf      ( "Format               TimeSprintf             strftime\n\n" );

                        TimeSprintf ( str,  40, "%a %A", tms );
                        strftime    ( str2, 40, "%a %A", tms2 );
                        printf      ( "%%a %%A      %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%b %B", tms );
                        strftime    ( str2, 40, "%b %B", tms2 );
                        printf      ( "%%b %%B      %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%d", tms );
                        strftime    ( str2, 40, "%d", tms2 );
                        printf      ( "%%d         %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%y %Y", tms );
                        strftime    ( str2, 40, "%y %Y", tms2 );
                        printf      ( "%%y %%Y      %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%H/%I:%M:%S %p", tms );
                        strftime    ( str2, 40, "%H/%I:%M:%S %p", tms2 );
                        printf      ( "%%H/I%%M%%S%%p %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%U %W", tms );
                        strftime    ( str2, 40, "%U %W", tms2 );
                        printf      ( "%%U %%W      %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%j %m %%", tms );
                        strftime    ( str2, 40, "%j %m %%", tms2 );
                        printf      ( "%%j %%m %%    %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%x %X", tms );
                        strftime    ( str2, 40, "%x %X", tms2 );
                        printf      ( "%%X %%x       %20s        %20s\n",
                                         str, str2 );

                        TimeSprintf ( str,  40, "%c %Z", tms );
                        strftime    ( str2, 40, "%c %Z", tms2 );
                        printf      ( "%%c %%Z       %20s        %20s\n",
                                         str, str2 );

                        break;

            case 'f':   /* difference between dates */
            case 'F':
                        i = script_date_to_tms ( p, tms );
                        if ( i < 0 )
                            continue;
                        p++;
                        while ( *p != ' ' && *p != '\n' )
                            p++;
                        i = script_date_to_tms ( p, tms2 );
                        day_num = DateDiff ( tms, tms2 );
                        TimeSprintf ( str, 40, "%m-%d-%Y", tms );
                        printf ( "%s is %ld days from ",
                                 str, day_num );
                        TimeSprintf ( str, 40, "%m-%d-%Y", tms2);
                        printf ( "%s\n", str );
                        break;

            case 'n':   /* convert day number to date */
            case 'N':
                        while ( *p != '\n' && ! isdigit ( *p ))
                            p++;

                        day_num = atol ( p );
                        DayNumberToDate ( day_num, tms );
                        TimeSprintf ( str, 40, "%m-%d-%Y", tms );
                        printf ( "%ld is %s\n", day_num, str );
                        break;

            case 't':   /* print today's date */
            case 'T':
                        timer = time ( NULL );
                        tms   = localtime ( &timer );

                        TimeSprintf ( str, 40,
                                "%m-%d-%Y at %H:%M:%S", tms );
                        printf ( "Today is %s, ", str );

                        tms->tm_year += 1900;
                        LoadDateStruct ( tms2, 1, 1, 1 );
                        LoadTimeStruct ( tms2, 0, 0, 0 );
                        printf ( "%g secs elapsed in the modern era\n",
                                 TimeDiff ( tms, tms2 ));
                        break;

            case 'u':   /* Sunday/Monday week count */
            case 'U':
                        i = script_date_to_tms ( p, tms );
                        if ( i < 0 )
                            continue;

                        tms2->tm_year = tms->tm_year - 1900;
                        tms2->tm_mon  = tms->tm_mon;
                        tms2->tm_mday = tms->tm_mday;
                        tms2->tm_hour = 9;
                        tms2->tm_min  = 10;
                        tms2->tm_sec  = 11;
                        mktime ( tms2 );

                        printf      ( "Format               TimeSprintf                   strftime\n\n" );

                        TimeSprintf ( str,  40, "%U %W", tms );
                        strftime    ( str2, 40, "%U %W", tms2 );
                        printf      ( "%%U %%W      %20s        %20s\n",
                                         str, str2 );
                        break;

            case 'w':   /* day of week */
            case 'W':
                        i = script_date_to_tms ( p, tms );
                        if ( i < 0 )
                            continue;
                        i = DayOfWeek ( tms );
                        day_num = DateToDayNumber ( tms );
                        printf ( "%d-%d-%d is %s day num: %ld\n",
                                    tms->tm_mon + 1, tms->tm_mday,
                                    tms->tm_year,
                                    Day_name[i], day_num );
                        break;

            default:
                        printf ( "Error: %s\n", str );
                        break;
        }

    }
    return ( EXIT_SUCCESS );
}

/*--------------------------------------------------------------
 * reads a script date in mm-dd-yyyy order and loads it into
 * a tm structure pointed to by tms.
 *-------------------------------------------------------------*/
int script_date_to_tms ( char *p, struct tm *tms )
{
    int yy, mm, dd;
    char *orig_p = p;

    while ( *p != '\n' && ! isdigit ( *p ))
        p++;

    mm = atoi ( p );
    while ( *p != '\n' && isdigit ( *p ))
        p++;

    dd = atoi ( ++p );
    while ( *p != '\n' && isdigit ( *p ))
        p++;

    yy = atoi ( ++p );

    if ( LoadDateStruct ( tms, yy, mm, dd ) == -1 )
    {
        printf ( "Error: %s\n", orig_p );
        return ( -1 );
    }

    return ( 0 );
}

int script_time_to_tms ( char *p, struct tm *tms )
{
    int hh, mm, ss;

    while ( *p != '\n' && ! isdigit ( *p ))
        p++;

    hh = atoi ( p );
    while ( *p != '\n' && isdigit ( *p ))
        p++;

    mm = atoi ( ++p );
    while ( *p != '\n' && isdigit ( *p ))
        p++;

    ss = atoi ( ++p );

    if ( hh < 0 || hh > 23 )
        return ( -1 );
    else
        tms->tm_hour = hh;

    if ( mm < 0 || hh > 59 )
        return ( -1 );
    else
        tms->tm_min = mm;

    if ( ss < 0 || ss > 59 )
        return ( -1 );
    else
        tms->tm_sec = ss;

    return ( 0 );
}
#endif
