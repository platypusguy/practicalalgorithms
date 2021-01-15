/*--- MINIGREP.C -------------------------- Listing 4-5 --------
 *  Find substrings represented by regular expressions
 *  in some other string or text
 *
 *  #define DRIVER to have a command-line version compiled.
 *------------------------------------------------------------*/

#define DRIVER 1    /* Compile the main driver */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/*----------------------------------------------------
 * If DEBUG is defined, D(printf("hello");) expands
 * to printf("hello"). If DEBUG isn't defined, D(...)
 * is expanded to an empty string, effectively
 * removing the printf() statement from input
 *--------------------------------------------------*/
#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

/*--- Metacharacters in the input:  ---*/
#define BOL     '^'     /* start-of-line anchor                */
#define EOL     '$'     /* end-of-line anchor                  */
#define ANY     '.'     /* matches any character               */
#define CCL     '['     /* start a character class             */
#define CCLEND  ']'     /* end a character class               */
#define NCCL    '^'     /* negates character class if 1st char */
#define CLOSURE '*'     /* Kleene closure (matches 0 or more)  */
#define PCLOSE  '+'     /* Positive closure (1 or more)        */
#define OPT     '?'     /* Optional closure (0 or 1)           */

/*--- These are the tokens representing metacharacters -------*/

#define    MT_BOL      ( 0x80 | '^' )
#define    MT_EOL      ( 0x80 | '$' )
#define    MT_ANY      ( 0x80 | '.' )
#define    MT_CCL      ( 0x80 | '[' )
#define    MT_OPT      ( 0x80 | '?' )
#define    MT_CLOSE    ( 0x80 | '*' )
#define    MT_PCLOSE   ( 0x80 | '+' )

/*--- pattern strings are unsigned char ---*/
typedef unsigned char pattern;

/* maximum number of pattern elements. Remember that
 * character classes require 17 pattern elements.
 */
#define MAXPAT  128

/*--- need this many bytes for character-class bitmap ---*/
#define MAPSIZE 16

/*  Advance a pointer into the pattern template
 *  to the next pattern element, this is a +1 for
 *  all pattern elements but MT_CCL, where you
 *  to skip past both the MT_CCL character and the
 *  bitmap that follows that character
 */

#define ADVANCE(pat) (pat += (*pat == MT_CCL) ? (MAPSIZE+1) : 1)

    /* Bitmap functions. Set bit b in the map and */
    /* test bit b to see if it was set previously */

#define SETBIT(b,map) ( \
         (map)[((b) & 0x7f) >>3] |= (1<< ((b) & 0x07)) )
#define TSTBIT(b,map) ( \
         (map)[((b) & 0x7f) >>3] &  (1<< ((b) & 0x07)) )

#define ISHEXDIGIT(x) isxdigit(x)

#define ISOCTDIGIT(x) ('0'<=(x) && (x)<='7')


/*--- Return values from PatternError() and MakePattern() -----*/
#define E_NONE       0
#define E_ILLEGAL    1
#define E_NOMEM      2
#define E_PAT        3

static int Error = E_NONE;     /*--- error flag, like errno ---*/

/*--- Declare the functions we wiil create and use ------------*/
static  pattern * DoCCL      ( pattern *, unsigned char *);
static  int       DoEscapeSeq( char ** );
static  int       HexToBinary( int );
static  int       MatchOne   ( char **, pattern *, char * );
static  pattern * MakePattern( char * );
extern  char    * MatchString( char *,  pattern *, int );
static  int       OctToBinary( int );
static  char    * PatternCmp ( char *,  pattern *, char * );
extern  int       PatternErr ( void );


/*--- returns current error status ---*/
int PatternErr ( void )
{
    return ( Error );
}

/*---------------------------------------------------------------
 *  Make a pattern template from the regular-expression
 *  string pointed to by exp.
 *  Stop when '\0' or '\n' is found in exp.
 *  Return:  a pointer to the pattern template on success, NULL
 *           on failure (in which case, PatternErr()
 *           will return one of the following values:
 *
 *                  E_ILLEGAL       Illegal input pattern.
 *                  E_NOMEM         out of memory
 *                  E_PAT           pattern too long.
 *-------------------------------------------------------------*/
static pattern *MakePattern ( char *exp )
{

    pattern *pat;  /* pattern template being assembled    */
    pattern *cur;  /* pointer to current pattern element  */
    pattern *prev; /* pointer to previous pattern element */

    pat   = NULL;
    Error = E_ILLEGAL;
    if( ! *exp || *exp == '\n' )
        return ( pat );

    if( *exp == CLOSURE || *exp == PCLOSE || *exp == OPT )
        return ( pat );

    /* get pattern buffer  */
    Error = E_NOMEM;
    if (( pat = (pattern *) malloc ( MAXPAT )) == NULL )
        return ( pat );

    /* zero the buffer if debugging */
    D( memset ( pat, 0, MAXPAT); )

    prev  = cur = pat;
    Error = E_PAT;

    while ( *exp  &&  *exp != '\n' )
    {
        if ( cur >= &pat[MAXPAT-1] )
        {
            free ( pat );
            return ( NULL );
        }

        switch ( *exp )
        {
            case ANY:
                *cur = MT_ANY;
                prev = cur++;
                ++exp;
                break;

            case BOL:
                *cur = ( cur == pat ) ? MT_BOL : *exp;
                prev = cur++;
                ++exp;
                break;

            case EOL:
                *cur = ( !exp[1] || exp[1]=='\n' ) ?
                            MT_EOL : *exp;
                prev = cur++;
                ++exp;
                break;

            case CCL:
                if ((( cur - pat ) + MAPSIZE ) >= MAXPAT )
                {
                    /* need more room for bitmap */

                    free ( pat );
                    return ( NULL );
                }

                prev   = cur;
                *cur++ = MT_CCL;
                exp    = DoCCL ( cur, exp );
                cur   += MAPSIZE ;
                break;

            case OPT:
            case CLOSURE:
            case PCLOSE:
                switch ( *prev )
                {
                    case MT_BOL:
                    case MT_EOL:
                    case MT_OPT:
                    case MT_PCLOSE:
                    case MT_CLOSE:
                        free ( pat );
                        return ( NULL );
                }

                memmove ( prev+1, prev, cur-prev );
                *prev = ( *exp == OPT ) ? MT_OPT :
                        ( *exp == PCLOSE ) ? MT_PCLOSE :
                        MT_CLOSE;
                ++cur;
                ++exp;
                break;

            default:
                prev   = cur;
                *cur++ = DoEscapeSeq ( &exp );
                break;
        }
    }

    *cur  = '\0';
    Error = E_NONE;

    return ( pat );
}

/*---------------------------------------------------------------
 *  Set bits in the map corresponding to characters
 *  specified in the src character class.
 *-------------------------------------------------------------*/

static pattern *DoCCL ( pattern *map, pattern *src )
{
    int first, last, negative;
    pattern *start = src;

    ++src;                           /* skip past the [       */
    if( negative = ( *src == NCCL )) /* check for negative CCl*/
        ++src;
    start = src;              /* start of characters in class */
    memset ( map, 0, MAPSIZE );     /* bitmap initially empty */

    while ( *src && *src != CCLEND )
    {
        if ( *src != '-' )
        {
            /* Use temp variable to avoid macro side effects */

            first = DoEscapeSeq ( &src );
            SETBIT( first, map );
        }
        else
        if ( src == start  )
        {
            SETBIT( '-', map );      /* literal dash at     */
            ++src;                   /* start or end.       */
        }
        else
        {
            ++src;          /* skip to end-of-sequence char */
            if ( *src < src[-2] )
            {
                first = *src;
                last  = src[-2];
            }
            else
            {
                first = src[-2];
                last  = *src;
            }

            while ( ++first <= last )
                SETBIT( first, map );
            src++;
        }
    }

    if ( *src == CCLEND )
        ++src;                  /* Skip CCLEND */

    if ( negative )
        for ( first = MAPSIZE; --first >= 0 ; )
            *map++ ^= ~0;      /* invert all bits */

    return ( src );
}

/*---------------------------------------------------------------
 *  Uses PatternCmp() to look for a match of pat anywhere in
 *  str using a brute-force search. str is a character string
 *  while pat is a pattern template made by MakePattern().
 *  Returns:
 *      o  NULL if no match was found.
 *      o  A pointer the last character satisfying the match
 *           if ret_endp is true.
 *      o  A pointer to the beginning of the matched string
 *           if ret_endp is false.
 *-------------------------------------------------------------*/

char *MatchString ( char *str, pattern *pat, int ret_endp )
{
    char  *start;
    char  *end = NULL;

    /*----------------------------------------------------------
    * This test lets you do MatchString(str, MakePattern(...),?);
    * without grave consequences if MakePattern() fails.
    *----------------------------------------------------------*/

    if ( !pat )
        return NULL;

    if ( !*str )
    {
        if (( *pat == MT_EOL ) ||
            ( *pat == MT_BOL &&
                ( !pat[1] || pat[1] == MT_EOL )))
                    end = str;
    }
    else
    {
       /*-------------------------------------------------
        * Do a brute-force search for substring, comparing
        * a pattern against the input string.
        *-----------------------------------------------*/
        start = str;
        while ( *str )
        {
            if( !( end = PatternCmp ( str, pat, start )))
                str++;
            else                /* successful match */
            {
                if ( !ret_endp )
                    end = str ;
                break;
            }
        }
    }
    return ( end );
}

/*-------------------------------------------------------------
* Like strcmp, but compares str against pat. Each element of
* str is compared with the template until either a mismatch is
* found or the end of the template is reached. In the former
* case, a 0 is returned; in the latter, a pointer into str
* (pointing to the last character in the matched pattern)
* is returned. strstart points at the first character in the
* string, which might not be the same thing as line if the
* search started in the middle of the string.
-------------------------------------------------------------*/
static char *PatternCmp ( char *str, pattern *pat, char *start )
{
    char *bocl,    /* beginning of closure string.         */
         *end;     /* return value: end-of-string pointer. */

    if ( !pat )            /* make sure pattern is valid   */
        return ( NULL );

    while ( *pat )
    {
        if ( *pat == MT_OPT )
        {
           /*----------------------------------------------------
            *  Zero or one matches. It doesn't matter if MatchOne
            *  fails---it will advance str past the character on
            *  success. Always advance the pattern past both
            *  the MT_OPT and the operand.
            *--------------------------------------------------*/

            MatchOne ( &str, ++pat, start );
            ADVANCE ( pat );
        }
        else
        if ( !( *pat == MT_CLOSE || *pat == MT_PCLOSE ))
        {
           /*----------------------------------------------------
            *  Do a simple match. Note that MatchOne() fails
            *  if there's still something in pat when we are
            *  at end of string.
            *--------------------------------------------------*/

            if ( !MatchOne ( &str, pat, start ))
                return NULL;

            ADVANCE ( pat );
        }
        else         /* process a Kleene or positive closure */
        {
            if ( *pat++ == MT_PCLOSE )   /* one match req'd */
                if ( !MatchOne ( &str, pat, start ))
                        return NULL;

            /* Match as many as possible, zero is OK */

            bocl = str;
            while ( *str  && MatchOne ( &str, pat, start ))
                ;

           /*----------------------------------------------------
            *  str now points to the character that made
            *  made us fail. Try to process the rest of the
            *  string. If the character following the closure
            *  could have been in the closure (as in the pattern
            *  "[a-z]*t") the final 't' will be sucked up in the
            *  while loop. So, if the match fails, back up a
            *  notch and try to match the rest of the string
            *  again, repeating this process recursively until
            *  we get back to the beginning of the closure. The
            *  recursion goes, at most, one level deep.
            *--------------------------------------------------*/

            if ( *ADVANCE ( pat ))
            {
                for ( ; bocl <= str; --str )
                    if ( end = PatternCmp( str, pat, start ))
                            break;
                return ( end );
            }
            break;
        }
    }

   /*------------------------------------------------------------
    *  MatchOne() advances str to point at the next
    *  character to be matched. So str points at the
    *  character following the last character matched
    *  when you reach the end of the template. The
    *  exceptions are templates containing only a
    *  BOLN or EOLN token. In these cases, MatchOne doesn't
    *  advance. Since we must return a pointer to the last
    *  matched character, decrement str to make it point at
    *  the end of the matched string, making sure that the
    *  decrement hasn't gone past the beginning of the string.
    *
    *  Note that $ is a position, not a character, but in the
    *  case of a pattern ^$, a pointer to the end of line
    *  character is returned. In ^xyz$, a pointer to the z
    *  is returned.
    *
    *  The --str is done outside the return statement because
    *  max() is often a macro that has side effects.
    *----------------------------------------------------------*/

    --str;
    return ( max ( start, str ));
}

/*-------------------------------------------------------------
*  Match one pattern element, pointed at by pat, against the
*  character at **strp. Return 0 on a failure, 1 on success.
*  *strp is advanced to skip over the matched character on a
*  successful match. Closure is handled one level up by
*  PatternCmp().
*
*  "start" points at the character at the left edge of the
*  line. This might not be the same thing as *strp if the
*  search is starting in the middle of the string. Note,
*  an end-of-line anchor matches '\n' or '\0'.
*------------------------------------------------------------*/
static int MatchOne ( char **strp, pattern *pat, char *start )
{

     /* amount to advance *strp, -1 == error  */
     int advance = -1;

     switch ( *pat )
     {
       case MT_BOL:                 /* First char in string? */
         if ( *strp == start )      /* Only one star here.   */
              advance = 0;
         break;

       case MT_ANY:              /* . = anything but newline */
         if ( **strp != '\n' )
              advance = 1;
         break;

       case MT_EOL:
         if ( **strp == '\n'  ||  **strp == '\0' )
              advance = 0;
         break;

       case MT_CCL:
         if ( TSTBIT ( **strp, pat + 1 ))
              advance = 1;
         break;

       default:                    /* literal match */
         if ( **strp == *pat )
              advance = 1;
         break;
     }

     if ( advance > 0 )
          *strp += advance;

     return ( advance + 1 );
}

/*------------------------------------------------------------*/
static int HexToBinary ( int c )
{
     /* Convert the hex digit represented by 'c' to an int. 'c'
      * must be one of: 0123456789abcdefABCDEF
      */
     return (isdigit(c) ? (c)-'0': ((toupper(c))-'A')+10)  & 0xf;
}

static int OctToBinary ( int c )
{
     /* Convert the hex digit represented by 'c' to an int. 'c'
      * must be a digit in the range '0'-'7'.
      */
     return ((( c ) - '0' )  &  0x7 );
}

/*-------------------------------------------------------------
 *  Map escape sequences into their equivalent symbols.
 *  Return the equivalent ASCII character. *s is advanced
 *  past the escape sequence. If no escape sequence is
 *  present, the current character is returned and
 *  the string is advanced by one.
 *
 *  The following escape sequences are recognized:
 *
 *   \b     backspace
 *   \f     formfeed
 *   \n     newline
 *   \r     carriage return
 *   \s     space
 *   \t     tab
 *   \e     ASCII ESC character ('\033')
 *   \DDD   number formed of 1-3 octal digits
 *   \xDDD  number formed of 1-3 hex digits
 *   \^C    C = any letter. Control code
 *-----------------------------------------------------------*/
static int DoEscapeSeq ( char **s )
{
     int rval;

     if ( **s != '\\' )
          rval = * (( *s )++ );
     else
     {
          ++( *s );                    /* Skip the \ */
          switch ( toupper ( **s ))
          {
            case '\0':  rval = '\\';             break;
            case 'B':   rval = '\b' ;            break;
            case 'F':   rval = '\f' ;            break;
            case 'N':   rval = '\n' ;            break;
            case 'R':   rval = '\r' ;            break;
            case 'S':   rval = ' '  ;            break;
            case 'T':   rval = '\t' ;            break;
            case 'E':   rval = '\033';           break;

            case '^':
              rval = *++( *s ) ;
              rval = toupper ( rval ) - '@' ;
              break;

            case 'X':
              rval = 0;
              ++( *s );
              if ( ISHEXDIGIT ( **s ))
              {
                   rval  = HexToBinary ( *(*s)++ );
              }
              if ( ISHEXDIGIT(**s) )
              {
                   rval <<= 4;
                   rval  |= HexToBinary ( *(*s)++ );
              }
              if ( ISHEXDIGIT(**s) )
              {
                   rval <<= 4;
                   rval  |= HexToBinary ( *(*s)++ );
              }
              --( *s );
              break;

            default:
              if( !ISOCTDIGIT ( **s ))
                   rval = **s;
              else
              {
                   ++ ( *s );
                   rval = OctToBinary ( *(*s)++ );
                   if ( ISOCTDIGIT ( **s ))
                   {
                        rval <<= 3;
                        rval  |= OctToBinary ( *(*s)++ );
                   }
                   if ( ISOCTDIGIT ( **s ))
                   {
                        rval <<= 3;
                        rval  |= OctToBinary ( *(*s)++ );
                   }
                   --( *s );
              }
              break;
          }
          ++ ( *s );
     }
     return rval;
}

/*-------------------------------------------------------------
 * Driver is compiled so as to make this program a command-line
 * utility. In its absence, you have a grep engine that can be
 * called by other applications using the format below.
 *-----------------------------------------------------------*/

#ifdef DRIVER

#include <limits.h>      /* to access CHAR_BIT */

int main ( int argc, char **argv )
{
     static pattern *pat;
     static FILE    *inp;
     static char     inp_buf[ 1024 ];

     /*----------------------------------------------------------
      *  This implementation is so dependent on 8-bit bytes
      *  that they must be checked for. Since 8-bit bytes are
      *  almost universal, this should not hinder portability.
      *--------------------------------------------------------*/

     if ( CHAR_BIT != 8 )  /* ANSI-defined as bits in a char */
     {
        fprintf ( stderr, "Error: Requires 8-bit bytes\n" );
        exit ( EXIT_FAILURE );
     }

     if ( argc < 2 || argv[1][0] == '-' )
     {
          fprintf ( stderr,
                        "Usage: minigrep reg_exp filename\n" );
          fprintf ( stderr,
                        "Usage: minigrep reg_exp < filename\n" );
          exit ( EXIT_FAILURE );
     }

     if( !( pat = MakePattern ( argv[1] )))
     {
          fprintf ( stderr, "Can't make expression template\n" );
          exit ( EXIT_FAILURE );
     }

     if ( argc == 2 )
          inp = stdin;
     else
     if( !( inp = fopen ( argv[2], "r" )))
     {
          perror ( argv[2] );
          exit ( EXIT_FAILURE );
     }

     while ( fgets ( inp_buf, sizeof(inp_buf), inp ))
             if ( MatchString ( inp_buf, pat, 0 ))
                  fputs ( inp_buf, stdout );

     return ( EXIT_SUCCESS );
}
#endif
