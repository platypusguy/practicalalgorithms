/*--------------------------------------------------------------
 * Prototypes for datelib.c
 *-------------------------------------------------------------*/

long    DateDiff ( struct tm *, struct tm * );
long    DateToDayNumber ( struct tm * );
int     DayNumberToDate ( long int, struct tm * );
int     DayOfWeek ( struct tm * );
int     DayOfYear ( struct tm * );
int     IsDateValid ( struct tm * );
int     IsTimeValid ( struct tm * );
int     IsYearLeap ( int );
int     LoadDateStruct ( struct tm *, int, int, int );
int     LoadTimeStruct ( struct tm *, int, int, int );
double  TimeDiff ( struct tm *, struct tm * );
int     TimeSprintf ( char *, int, char *, struct tm * );

#ifndef YES
#define YES     1
#define NO      0
#endif
