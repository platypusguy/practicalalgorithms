/*--- longmath.h -------------------------- Listing 8-20 -------
 *  Header file for arbitrary-precision arithmetic routines
 *  in longmath.c
 *-------------------------------------------------------------*/

#ifndef LONGMATH_H              /* avoid multiple inclusions */
#define LONGMATH_H  1

struct TermData {               /* the data for each term */
        char   *term;               /* the term */
        int     sign;               /* pos or neg; 0 = error */
        int     places_before;      /* before decimal point */
        int     places_after;       /* after decimal point */
};

#ifndef MAX_SIZE
#define MAX_SIZE    20          /* maximum length of a term. */
#endif                          /* Can be changed freely.    */

#define NORMAL      1           /* formats of terms */
#define SCIENTIFIC  2

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(c,d) ((c) > (d) ? (c) : (d))
#endif


int  AsciiToTerm    ( char *, struct TermData * );
int  AsciiToScientific ( char * );
int  ComputeResult  ( struct TermData *, int,
                        struct TermData *, struct TermData * );
int  DivAtoin       ( const char *, int );
int  DivCheckZeroOnly   ( const char * );
void DivQuickMult   ( const char *, int, char * );
void DivQuickSub    ( char *, char *, char * );
void DivShiftArrayLeft      ( char * );
void DivShiftSmallArrayLeft ( char * );
void DivShiftArrayRight     ( char * );
int  DivSpecialCase ( const char *, const char * );
int  GetFileOperator( int *, FILE *, FILE * );
int  GetFileTerm    ( struct TermData * , FILE *, FILE * );
int  NormalAdd      ( struct TermData *, struct TermData *,
                        struct TermData * );
int  NormAbsCmp     ( struct TermData *, struct TermData * );
int  NormalDivide   ( struct TermData *, struct TermData *,
                        struct TermData * );
int  NormalMultiply ( struct TermData *, struct TermData *,
                        struct TermData * );
int  NormalSubtract ( struct TermData *, struct TermData *,
                        struct TermData * );
char * strrev       ( char * ); /* for portability */
void TermInit       ( struct TermData * );
struct TermData *
     TermCopy       ( struct TermData *, struct TermData * );
struct TermData *
     TermCreate     ( void );
void TermToAscii    ( struct TermData *, char *, int );


#define DEC_LOC     MAX_SIZE    /* location of decimal point */

#endif
