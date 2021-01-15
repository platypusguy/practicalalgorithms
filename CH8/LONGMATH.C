/*--- longmath.c ----------------------- Listings 8-2 to 8-19 ---
 * Arbitrary-precision routines.
 * To change precision, redefine MAX_SIZE in longmath.h
 * To use these functions, link to a driver.
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "longmath.h"

/*--------------------------------------------------------------
 * Creates a TermData structure and initializes it to
 * a value of zero. Returns NULL on error.
 *------------------------------------------------------------*/
struct TermData * TermCreate ( void )
{
    struct TermData *t;

    t = malloc ( sizeof ( struct TermData ));
    if ( t == NULL )
        return ( NULL );

    t->term = (char *) malloc ( 2*MAX_SIZE + 1 );
    TermInit ( t );

    return ( t );
}

/*---------------------------------------------------------------
 * This function handles the signs and invokes the correct
 * arithmetic operation.
 *-------------------------------------------------------------*/
int ComputeResult ( struct TermData *t1, int operator,
              struct TermData *t2, struct TermData *sol )
{
    int cmp;

    TermInit ( sol );   /* just good practice */

    switch ( operator )
    {
        case '+':
            /*
             * addition really occurs only when the signs
             * are the same. If signs differ, the operation
             * is really subtraction. In that case, we call
             * subtraction routines. Before calling
             * NormalSubtract(), we make sure the terms are
             * in the right order. See the comments in the
             * subtraction section below for more info on this.
             */

            if ( t1->sign == t2->sign ) /* a + b or -a + -b */
            {
                if ( ! NormalAdd ( t1, t2, sol ))
                {
                    fprintf ( stderr,
                        "Overflow on addition\n" );
                    return ( 0 );
                }
                sol->sign = t1->sign;
            }
            else
            if ( t1->sign == -1 )   /* -a + b */
            {
                sol->sign = -1;

                cmp = NormAbsCmp ( t1, t2 );
                if ( cmp < 0 ) /* t2 larger than t1 */
                {
                    sol->sign = -sol->sign;
                    NormalSubtract ( t2, t1, sol );
                }
                else
                if ( cmp > 0 ) /* t1 larger than t2 */
                    NormalSubtract ( t1, t2, sol );
                else
                {              /*  t1 = t2, so sol = 0 */
                    TermInit ( sol );
                    return ( 1 );
                }
            }
            else                    /* a + -b */
            {
                sol->sign = +1;

                cmp = NormAbsCmp ( t1, t2 );
                if ( cmp < 0 ) /* t2 larger than t1 */
                {
                    sol->sign = -sol->sign;
                    NormalSubtract ( t2, t1, sol );
                }
                else
                if ( cmp > 0 ) /* t1 larger than t2 */
                    NormalSubtract ( t1, t2, sol );
                else
                {              /*  t1 = t2, so sol = 0 */
                    TermInit ( sol );
                    return ( 1 );
                }
            }
            break;

        case '-':
            /*
             * there are four possible cases for subtraction,
             * and each is treated differently:
             *  a - b       subtract a from b
             *  a - -b      add a and b
             * -a - b       add a and b, multiply by -1
             * -a - -b      subtract a from b, multiply by -1
             *
             * multiplying by -1 is accomplished simply
             * by flipping the sign of the result.
             *
             * Moreover, subtraction is set up so that we always
             * subtract the smaller term from the larger. If t2
             * is larger than t1, however, then we flip the
             * sign of the solution and reverse the terms. This
             * approach works because x - y = -1( y - x ). That
             * is, you can flip the terms in subtraction if you
             * flip the sign of the difference:
             *              3 - 7 = -1 * ( 7 - 3 )
             */

            if ( t1->sign == +1 )
            {
                if ( t2->sign == +1 )  /* a - b */
                {
                    sol->sign = +1;

                    cmp = NormAbsCmp ( t1, t2 );
                    if ( cmp < 0 ) /* t2 larger than t1 */
                    {
                        sol->sign = -sol->sign;
                        NormalSubtract ( t2, t1, sol );
                    }
                    else
                    if ( cmp > 0 ) /* t1 larger than t2 */
                        NormalSubtract ( t1, t2, sol );
                    else
                    {              /*  t1 = t2, so sol = 0 */
                        TermInit ( sol );
                        return ( 1 );
                    }
                }
                else
                {                      /* a - -b */
                    if ( ! NormalAdd ( t1, t2, sol ))
                    {
                        fprintf ( stderr,
                            "Overflow on addition\n" );
                        return ( 0 );
                    }
                    sol->sign = +1;
                }
            }
            else
            {                          /* -a - b */
                if ( t2->sign == +1 )
                {
                    if ( ! NormalAdd ( t1, t2, sol ))
                    {
                        fprintf ( stderr,
                            "Overflow on addition\n" );
                        return ( 0 );
                    }
                    sol->sign = -1;
                }
                else                   /* -a - -b */
                {
                    sol->sign = -1;

                    cmp = NormAbsCmp ( t1, t2 );
                    if ( cmp < 0 ) /* t2 larger than t1 */
                    {
                        sol->sign = -sol->sign;
                        NormalSubtract ( t2, t1, sol );
                    }
                    else
                    if ( cmp > 0 ) /* t1 larger than t2 */
                        NormalSubtract ( t1, t2, sol );
                    else
                    {              /*  t1 = t2, so sol = 0 */
                        TermInit ( sol );
                        return ( 1 );
                    }
                }
            }
            break;

        case '*':
            /*
             * multiplication sign issues are straightforward.
             * just multiply and set the sign, depending on
             * whether the terms have the same sign.
             */

            if ( ! NormalMultiply ( t1, t2, sol ))
            {
                fprintf ( stderr, "Overflow on mulitply\n" );
                return ( 0 );
            }
            if ( t1->sign == t2->sign )
                sol->sign = +1;
            else
                sol->sign = -1;
            break;


        case '/':
            /* likewise for division */

            if ( ! NormalDivide ( t1, t2, sol ))
                return ( 0 );

            if ( t1->sign == t2->sign )
                sol->sign = +1;
            else
                sol->sign = -1;
            break;

        default:
            fprintf ( stderr, "Unsupported operation %c\n",
                        operator );
            return ( 0 );
    }

    return ( 1 );
}

/*---------------------------------------------------------------
 * We start at the rightmost digit of the terms, but no farther
 * left than the decimal point. We work our way left, adding as
 * we go until we reach the leftmost digit, and then go one digit
 * farther, in case of any carry.
 *-------------------------------------------------------------*/
int NormalAdd ( struct TermData *t1, struct TermData *t2,
                  struct TermData *sum )
{
    int i, j, start, stop;

    start = DEC_LOC +
                max ( t1->places_after, t2->places_after );
    stop  = DEC_LOC - 1 -
                max ( t1->places_before, t2->places_before );

    for ( i = start; i >= stop; i-- )
    {
        sum->term[i] += ( t1->term[i] + t2->term[i] );
        if ( sum->term[i] > 9 )
        {
             sum->term[i] -= 10;
             sum->term[i-1] += 1;
        }
    }

    sum->places_after = start - DEC_LOC;

    for ( i = 0; i < DEC_LOC; i++ )
          if ( (sum->term)[i] != 0 )
            break;
    sum->places_before = DEC_LOC - i;

    /* make sure that the sum is within MAX_SIZE digits */

    i = sum->places_before + sum->places_after - MAX_SIZE;

    if ( i > 0 )    /* sum is larger than MAX_SIZE */
    {
        int carry = 0;

        if ( i > sum->places_after )
            return ( 0 );   /* high-order truncation will occur*/

        sum->places_after -= i;    /* adjust sum */

        j = DEC_LOC + sum->places_after;    /* do rounding */
        if ( (sum->term)[j] > 4 )
            carry = 1;

        if ( carry )
            while ( 1 )
            {
                j--;

                if ( j == 0 && (sum->term)[j] > 8 )
                    return ( 0 );

                (sum->term)[j] += carry;

                if ( (sum->term)[j] > 9 )
                    (sum->term)[j] -= 10;
                else
                    break;
            }

        while ( i-- )       /* do low-order truncation */
        {
            (sum->term)[DEC_LOC +
                    sum->places_after + i ] = 0;
        }
    }

    return ( 1 );
}

/*--------------------------------------------------------------
 * NormAbsCmp() works similar to strcmp(). The return value is
 * < 0, = 0, > 0 depending on whether the *absolute value* of
 * the first term is less than, equal to, or greater than the
 * second term, respectively.
 *------------------------------------------------------------*/
int NormAbsCmp ( struct TermData * t1, struct TermData * t2 )
{
    int memcmp_result;

    /* first check the digits before the decimal point */

    if ( t1->places_before > t2->places_before )
        return ( 1 );

    if ( t2->places_before > t1->places_before )
        return ( -1 );

    /*
     * same number of digits before decimal point,
     * so compare character by character
     */

    memcmp_result =
         memcmp ( t1->term + DEC_LOC - t1->places_before,
                  t2->term + DEC_LOC - t2->places_before,
                  t1->places_before +
                    max ( t1->places_after, t2->places_after ));

    if ( memcmp_result > 0 )
        return ( 1 );
    else
    if ( memcmp_result < 0 )
        return ( -1 );
    else
        return ( 0 );
}

/*--------------------------------------------------------------
 * Division is accomplished by turning the term data into an
 * array of characters rather than bytes holding the numeric
 * value of each digit--the latter format is used in the other
 * arithmetic operations. However, in division, it becomes a
 * cumbersome format. (See text.)
 *------------------------------------------------------------*/
int NormalDivide  ( struct TermData * dvend_arg,
                    struct TermData * dvsor_arg,
                    struct TermData * quot )
{
#define ASCII_VAL ( '0' ) /* the difference between 0 and '0' */

    unsigned int
         leading_zeros = 0,
         divend_len,
         divsor_len,
         idivend_len;

    int  strcmp_return,
         strncmp_return;

    char *answer,
         *dividend,
         *divisor,
         *interim,
         *new_interim,
         *product;

    unsigned int
         quo_guess,

         next_dvend_digit = 0,

         answer_idx = 0,

         dvsor_len,
         idvend_len;

    int  dvend_order, /* order of magnitude of divisor and   */
         dvsor_order; /* dividend. Used for locating decimal */
                      /* point in quotient.                  */

    char *pc;   /* generic variables */
    int  i, j;


    dividend    = calloc ( 1, 2*MAX_SIZE + 1 );
    divisor     = calloc ( 1, MAX_SIZE + 1 );
    interim     = calloc ( 1, 2*MAX_SIZE + 1 );
    new_interim = calloc ( 1, 2*MAX_SIZE + 1 );
    product     = calloc ( 2, 2*MAX_SIZE + 1 );
    answer      = calloc ( 2, MAX_SIZE + 1 );

    if ( dividend == NULL || divisor == NULL  ||
         interim  == NULL || new_interim == NULL ||
         product  == NULL || answer == NULL )
    {
        printf ( "Error allocating memory in division\n" );
        return  ( 0 );
    }

    /* load dividend with the digits as character values */
    i = 0;
    for ( pc  = dvend_arg->term +
                DEC_LOC - dvend_arg->places_before;
          pc <= dvend_arg->term +
                DEC_LOC + dvend_arg->places_after - 1;
          pc++, i++ )
            dividend[i] = *pc + ASCII_VAL;

    /* remove leading zeros */

    while ( *dividend == '0' )
        DivShiftArrayLeft ( dividend );


    /* likewise, for the divisor */
    i = 0;
    for ( pc  = dvsor_arg->term +
                DEC_LOC - dvsor_arg->places_before;
          pc <= dvsor_arg->term +
                DEC_LOC + dvsor_arg->places_after - 1;
          pc++, i++ )
            divisor[i] = *pc + ASCII_VAL;

    while ( *divisor == '0' )
        DivShiftSmallArrayLeft ( divisor );

    /* If divisor is zero, signal error and abort */

    if ( DivCheckZeroOnly ( divisor ))
    {
        printf ( "Error: Division by Zero\n" );
        return ( 0 );
    }

    /* If dividend is zero, set quotient to zero and exit */

    if ( DivCheckZeroOnly ( dividend ))
    {
        TermInit ( quot );
        return ( 1 );
    }

    divend_len = strlen ( dividend );
    divsor_len = strlen ( divisor );

    /*
     * If dividend is < divisor,
     * add zeros to end of dividend
     * an add leading zeros to quotient
     */

    while ( divend_len < divsor_len )
    {
        *( dividend + divend_len ) = '0';
        leading_zeros += 1;
        divend_len += 1;
    }

    if ( divend_len == divsor_len )
    {
        strcmp_return = strcmp ( dividend, divisor );
        if ( strcmp_return < 0 )    /* dividend is lesser */
        {
            *( dividend + divend_len ) = '0';
            leading_zeros += 1;
            divend_len += 1;
        }
        else
        if ( strcmp_return == 0 )    /* they're the same */
        {
            answer[answer_idx++] = '1';
            goto wrapup;
        };                   /* otherwise, divisor is lesser */
    }

    /* load the dividend into interim */

    strcpy ( interim, dividend );
    idivend_len = strlen ( interim );

    loop:                   /*--- main loop ---*/

    if ( divsor_len > 3 )
        dvsor_len = idvend_len = 4;
    else
    if ( divsor_len < idivend_len )
    {
        if ( strcmp ( divisor, interim ) > 0 )
        {
            dvsor_len = divsor_len;
            idvend_len = divsor_len + 1;
        }
        else
            dvsor_len = idvend_len = divsor_len;
    }
    else /* can only be the terms are equal length */
    {
        dvsor_len  = divsor_len;
        idvend_len = idivend_len;
    }

    if ( dvsor_len == idvend_len && dvsor_len > 1 )
        if ( DivAtoin ( divisor, dvsor_len ) >
             DivAtoin ( interim, dvsor_len ))
                dvsor_len -= 1;

    quo_guess = DivAtoin ( interim, idvend_len ) /
                DivAtoin ( divisor, dvsor_len );

    if ( quo_guess > 9 )
         quo_guess /= 10;

    try_quo_guess:      /*--- try_quo_guess goto ---*/

    DivQuickMult ( divisor, quo_guess, product );

    strncmp_return =
        strncmp ( product, interim, strlen ( product ));

    if ( strncmp_return > 0 )   /* if product > interim */
    {
        if ( quo_guess == 1 )
        {
            /*
             * a quo_guess of 1 can be a special case:
             * try 9 and bring down another digit
             */

            if ( DivSpecialCase ( divisor, interim ))
            {
                quo_guess = 9;

                /*
                 * did we already pad the dividend ?
                 * if so, then add a zero to interim dividend,
                 * if not, then bring down another digit.
                 */

                if ( leading_zeros )
                    interim[idvend_len++] = '0';
                else
                if ( next_dvend_digit < divend_len )
                    interim[idvend_len++] =
                        dividend[next_dvend_digit++];

                /* and try again */

                goto try_quo_guess;
            }
            else            /* not special case */
                DivShiftArrayRight ( product );
        }
        else

        /* quo_guess != 1, so check whether array needs shift */

        if ( strlen ( product ) < idivend_len )
            DivShiftArrayRight ( product );
        else
        {           /* guess was just too high so try again */
            quo_guess -= 1;
            goto try_quo_guess;
        }
    }

    /* load the correct digit */

    answer[answer_idx++] = quo_guess + ASCII_VAL;

    /*
     * if no next digit to bring down has been ascertained,
     * the next operation sets which digit to start bringing
     * down. Only done the first time through here.
     */

    if ( ! next_dvend_digit )
        next_dvend_digit = strlen ( product );

    /* new_interim = interim - product */

    DivQuickSub ( interim, product, new_interim );

    if (( DivCheckZeroOnly ( new_interim )
        && next_dvend_digit  >= divend_len )
        || answer_idx >= MAX_SIZE )        /* are we done? */
            goto wrapup;

    while ( *new_interim == '0' )
        DivShiftArrayLeft ( new_interim );

    memset ( interim, '\0', 2*MAX_SIZE + 1 );
    strcpy ( interim, new_interim );
    idivend_len = strlen ( interim );

    get_next_digit:     /*--- loop for get next digit ---*/

    if ( next_dvend_digit < divend_len )
        interim[idivend_len++] = dividend[next_dvend_digit++];
    else
    {                   /* if beyond EO dividend, bring down 0 */
        interim [ idivend_len++] = '0';
    }

    if ( idivend_len < divsor_len )    /* if interim < divisor */
    {
        answer[answer_idx++] = '0';
        if ( answer_idx >= MAX_SIZE )
            goto wrapup;
        goto get_next_digit;
    }
    else
    if ( idivend_len == divsor_len )    /* same length */
    {
        if ( strcmp ( divisor, interim ) > 0 )
        {                           /* but divisor is greater */
            answer[answer_idx++] = '0';
            if ( answer_idx >= MAX_SIZE )
                goto wrapup;
            goto get_next_digit;
        }
    }

    goto loop;

    wrapup:

    /*
     * Now take result from answer and place it in quot. We
     * compare the order of magnitudes of the dividend and the
     * divisor to determine where the decimal point goes.
     * The rule is:
     *
     *   Places  =      Order        -  Order       +  1
     *                      (dvend)         (divisor)
     *
     *      where strcmp ( dividend, divisor ) > 0
     *      otherwise, don't add 1.
     *
     * A positive number is the number of places left of
     * the decimal point. From this, we subtract any leading
     * zeros.
     */

    /* get the order for the dividend */

    if ( dvend_arg->places_before != 0 )
        dvend_order = dvend_arg->places_before;
    else
    {
        i = 0;
        pc = dvend_arg->term + DEC_LOC;
        while ( pc < dvend_arg->term + 2*MAX_SIZE )
        {
            if ( *pc != 0 )
                break;
            else
            {
                i  += 1;
                pc += 1;
            }
        }
        dvend_order = -i;
    }

    /* get the order for the divisor */

    if ( dvsor_arg->places_before != 0 )
        dvsor_order = dvsor_arg->places_before;
    else
    {
        i = 0;
        pc = dvsor_arg->term + DEC_LOC;
        while ( pc < dvsor_arg->term + 2*MAX_SIZE )
        {
            if ( *pc != 0 )
                break;
            else
            {
                i  += 1;
                pc += 1;
            }
        }
        dvsor_order = -i;
    }

    i = dvend_order - dvsor_order;

    if  ( strcmp ( dividend, divisor ) >= 0 )
        i += 1;

    j = DEC_LOC - i;

     /*
      * quot has already been intialized to zeros,
      * so we can start moving in the digits.
      */

    for ( i = 0; i < answer_idx && j < 2*MAX_SIZE; i++, j++ )
         (quot->term)[j] = answer[i] - ASCII_VAL;

    /* compute the number of places before and after */

    for ( i = 0; i < DEC_LOC; i++ )
          if ( (quot->term)[i] != 0 )
            break;

    quot->places_before = DEC_LOC - i;

    for ( i = 2*MAX_SIZE - 1; i >= DEC_LOC; i-- )
          if ( (quot->term)[i] != 0 )
            break;

    quot->places_after = i - DEC_LOC + 1;

    /* free the terms we created on the heap */

    free ( dividend );
    free ( divisor );
    free ( interim );
    free ( new_interim );
    free ( product );
    free ( answer );

    return ( 1 );
}

/*--------------------------------------------------------------
 * Performs atoi for length number of characters. Called by
 * division to establish the stubs of the divisor and dividend
 * we will use to try guessing the next quotient digit.
 *------------------------------------------------------------*/
int DivAtoin ( const char *string, int length )
{           /* usual tests for sign and white space omitted */
    int i, n;
    n = 0;

    for ( i = 0;
          i < length && ( string[i] >= '0' && string[i] <= '9' );
          i++ )
            n = 10 * n + string[i] - '0';

    return ( n );
}

/*--------------------------------------------------------------
 * This function checks a division term for a zero value.
 * Called only by division operation.
 *------------------------------------------------------------*/
int DivCheckZeroOnly ( const char *array )
{
    while ( *array )
    {
        if ( *array != '0' )
            return ( 0 );
        array += 1;
    }

    return ( 1 );
}

/*--------------------------------------------------------------
 * This function multiplies an array of digits as characters
 * by a single-digit integer. Called by division only.
 *------------------------------------------------------------*/
void DivQuickMult ( const char *long_term, int digit,
                    char *result )
{
    int from, to; /* array subscripts */
    int new_carry, old_carry, hold;

    new_carry = old_carry = hold = to = 0;

    memset ( result, '\0', 2*MAX_SIZE + 1 );
    for ( from = strlen ( long_term ) - 1;
          from >= 0; from--, to++ )
    {
        hold = ( long_term[from] - ASCII_VAL ) * digit;
        new_carry = hold / 10;
        result[to] = hold % 10 + old_carry;
        if ( result[to] > 9 )
        {
            new_carry += 1;
            result[to] -= 10;
        }
        result[to] += ASCII_VAL;
        old_carry = new_carry;
    }

    if ( old_carry )        /* if any left over */
        result[to] = old_carry + ASCII_VAL;

    strrev ( result );
}

/*--------------------------------------------------------------
 * Subtracts one array of chars (the subtrahend) from another
 * array (the minuend), generating a difference. Called only
 * by division.
 *------------------------------------------------------------*/
void DivQuickSub ( char *minuend, char *subtrahend,
                   char *diff )
{
    int sub, to;    /* indices to various arrays */

    sub = to = strlen ( subtrahend ) - 1; /* start at right   */
    diff[to + 1] = '\0';       /* after setting end of string */

    while ( sub >= 0 )
    {
        if ( minuend[sub] < subtrahend[sub] )
        {
            minuend[sub] += 10;
            subtrahend[sub - 1] += 1;
        }
        diff[to--] = minuend[sub] - subtrahend[sub] + ASCII_VAL;
        sub -= 1;
    }
}

/*--------------------------------------------------------------
 * Shifts an array of chars left by one character, truncating
 * the leftmost char. Called by division only when the leading
 * character is a '0'. The small version works on MAX_SIZE
 * arrays, the regular version on 2*MAX_SIZE.
 *------------------------------------------------------------*/
void DivShiftArrayLeft ( char *array )
{
    char buffer [ 2*MAX_SIZE + 1 ];
    memset ( buffer, '\0', 2*MAX_SIZE + 1);
    strcpy ( buffer, array );
    strcpy ( array, buffer + 1 );
}

void DivShiftSmallArrayLeft ( char *array )
{
    char buffer [ MAX_SIZE + 1 ];
    memset ( buffer, '\0', MAX_SIZE + 1);
    strcpy ( buffer, array );
    strcpy ( array, buffer + 1 );
}

/*--------------------------------------------------------------
 * Shifts an array of characters right one character and
 * prepends a zero. Called only by division.
 *------------------------------------------------------------*/
void DivShiftArrayRight ( char *array )
{
    memmove ( array + 1, array, strlen ( array ));
    array[0] = '0';
}

/*--------------------------------------------------------------
 * If the division guesses a quotient digit that is too high,
 * we normally would decrement the guess by 1 and try again.
 * However, if the guessed digit is a 1, we have to be careful,
 * because the decrement should give us a 9, not a 0. This
 * function test whether the correct digit is, in fact a 9, or
 * whether we have misapprehended the dividend on our guess.
 * Called only by division.
 *------------------------------------------------------------*/
int DivSpecialCase ( const char *divisor,
                     const char *curr_dividend )
{
    char test_result [2*MAX_SIZE + 1];

    DivQuickMult ( divisor, 9, test_result );

    if ( strcmp ( curr_dividend, test_result ) > 0 )
        return ( 1 );
    else
        return ( 0 );
}

#undef ASCII_VAL

/*--------------------------------------------------------------
 *  Multiplication of normalized terms.
 *  In the expression c = a * b, a is called the multiplicand,
 *  b the multiplier, and c the product.
 *------------------------------------------------------------*/
int NormalMultiply ( struct TermData * t1,
             struct TermData * t2, struct TermData * prod )
{
    /*
     * Each digit of the multiplier will require the generation
     * of an intermediate result, which is added to previous
     * intermediate results, to produce the product. Hence,
     * we'll make the multiplier the shorter of the two terms.
     */

    char *mcand,     /* the multiplicand */
         *mier,      /* the multiplier   */
         *temp;      /* temporary hold area for product */

    int  mcand_curr, /* where we are in the muliplicand */
         mier_curr,  /* where we are in the multiplier */
         temp_curr,  /* where we are in the temp product */
         temp_here;

    int  mcand_len,  /* number of digits in multiplicand */
         mier_len;   /* number of digits in multiplier */

    int  carry;      /* the carry digit when adding the
                        intermediate results */
    int  i, j, from, to;

    static int table [100] =
    {
     /*         0*0    0*1    0*2    0*3    0*4  */
     /* 0 */ 0x0000,0x0000,0x0000,0x0000,0x0000,
     /*         0*5    0*6    0*7    0*8    0*9  */
             0x0000,0x0000,0x0000,0x0000,0x0000,
     /*         1*0    1*1    1*2    1*3    1*4  */
     /* 1 */ 0x0000,0x0001,0x0002,0x0003,0x0004,
     /*         1*5    1*6    1*7    1*8    1*9  */
             0x0005,0x0006,0x0007,0x0008,0x0009,
     /*         2*0    2*1    2*2    2*3    2*4  */
     /* 2 */ 0x0000,0x0002,0x0004,0x0006,0x0008,
     /*         2*5    2*6    2*7    2*8    2*9  */
             0x0100,0x0102,0x0104,0x0106,0x0108,
     /*         3*0    3*1    3*2    3*3    3*4  */
     /* 3 */ 0x0000,0x0003,0x0006,0x0009,0x0102,
     /*         3*5    3*6    3*7    3*8    3*9  */
             0x0105,0x0108,0x0201,0x0204,0x0207,
     /*         4*0    4*1    4*2    4*3    4*4  */
     /* 4 */ 0x0000,0x0004,0x0008,0x0102,0x0106,
     /*         4*5    4*6    4*7    4*8    4*9  */
             0x0200,0x0204,0x0208,0x0302,0x0306,
     /*         5*0    5*1    5*2    5*3    5*4  */
     /* 5 */ 0x0000,0x0005,0x0100,0x0105,0x0200,
     /*         5*5    5*6    5*7    5*8    5*9  */
             0x0205,0x0300,0x0305,0x0400,0x0405,
     /*         6*0    6*1    6*2    6*3    6*4  */
     /* 6 */ 0x0000,0x0006,0x0102,0x0108,0x0204,
     /*         6*5    6*6    6*7    6*8    6*9  */
             0x0300,0x0306,0x0402,0x0408,0x0504,
     /*         7*0    7*1    7*2    7*3    7*4  */
     /* 7 */ 0x0000,0x0007,0x0104,0x0201,0x0208,
     /*         7*5    7*6    7*7    7*8    7*9  */
             0x0305,0x0402,0x0409,0x0506,0x0603,
     /*         8*0    8*1    8*2    8*3    8*4  */
     /* 8 */ 0x0000,0x0008,0x0106,0x0204,0x0302,
     /*         8*5    8*6    8*7    8*8    8*9  */
             0x0400,0x0408,0x0506,0x0604,0x0702,
     /*         9*0    9*1    9*2    9*3    9*4  */
     /* 9 */ 0x0000,0x0009,0x0108,0x0207,0x0306,
     /*         9*5    9*6    9*7    9*8    9*9  */
             0x0405,0x0504,0x0603,0x0702,0x0801
    };

    static int mults_of_ten [10] =
        { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };

    mcand_len = mier_len = 0;

    /*
     * products can overflow a term, since the number of
     * digits left of the decimal pt in the product is the
     * sum of the digits left of the decimal pt in the
     * multiplier and multiplicand - 1. Likewise, the number
     * of digits right of the decimal point is the sum of
     * these digits in the multiplier and multiplicand.
     *
     * Hence, our internal representation of the product,
     * temp, has 2*MAX_SIZE digits on either side of the
     * decimal point. We will round and truncate as necessary
     * when we load the digits into prod.
     */

    temp = calloc ( 1, 4*MAX_SIZE + 1 );
    if ( temp == NULL )
    {
        printf ( "Out of memory in multiplication.\n" );
        return ( 0 );
    }

    /*
     * the following test avoids guaranteed high-order
     * truncation and saves having to do the multiplication
     * only to discover the overflow at end.
     */

    if ( t1->places_before + t2->places_before - 1 >
         MAX_SIZE )
    {
        free ( temp );
        return ( 0 );
    }

    if (( t1->places_before + t1->places_after ) >
    ( t2->places_before + t2->places_after ))
    {
        mcand = t1->term;
        mier  = t2->term;

        mcand_len = t1->places_before + t1->places_after;
        mier_len  = t2->places_before + t2->places_after;

        mcand_curr = DEC_LOC + t1->places_after - 1;
        mier_curr  = DEC_LOC + t2->places_after - 1;
    }
    else
    {
        mcand = t2->term;
        mier  = t1->term;

        mcand_len = t2->places_before + t2->places_after;
        mier_len  = t1->places_before + t1->places_after;

        mcand_curr = DEC_LOC + t2->places_after - 1;
        mier_curr  = DEC_LOC + t1->places_after - 1;
    }

    /*
     * The number of digits after the decimal points in a
     * product is the sum of the number of decimal digits
     * in  each term: 12.6 * 1.2 = 15.12.
     * Hence, we start putting digits into prod, using the
     * following formulation:
     */

    temp_curr = 2*DEC_LOC +
                t1->places_after + t2->places_after - 1;

    carry = 0;
    while ( mier_len > 0 )  /* the multiplication loop */
    {                       /* for each digit of multiplier */
        int a, b, val;

        i = mcand_len;
        j = mcand_curr;
        temp_here = temp_curr;

        while ( i >= 0 )     /* process the whole multiplicand */
        {
            if ( mier [ mier_curr ] == 0 )
                break;

            a = mier [ mier_curr ];
            b = mcand [ j ];
            a += mults_of_ten [ b ];

            val = table [ a ];
            temp[ temp_here ] += carry + val & 0x00FF;
            carry = val >> 8;
            if ( temp[temp_here] > 9 )
            {
                carry += temp[temp_here] / 10;
                temp[ temp_here ] %= 10;
            }
            j--;         /* move up the multiplicand */
            i--;         /* one less iteration to do */
            temp_here--; /* move one product digit to the left */
        }
        mier_curr--;    /* move up the multiplier */
        mier_len--;     /* one less multiplier digit */
        temp_curr--;    /* move up the solution by one digit */
    }

    if ( carry > 0 )
        temp[temp_curr] = carry;

    for ( i = 0; i < 2*DEC_LOC; i++ )
          if ( temp[i] != 0 )
            break;

    /* did we overflow anyway? */

    if (( 2*DEC_LOC - i ) > MAX_SIZE )
    {
        free ( temp );
        return ( 0 );
    }
    else
        prod->places_before = 2*DEC_LOC - i;

    /* copy the digits before the dec pt from temp to prod */

    from = 2*DEC_LOC - 1;
    to   = DEC_LOC - 1;

    i = prod->places_before;
    while ( i-- )
        (prod->term)[to--] = temp[from--];

    /*
     * now examine the digits after the decimal point
     * and perform rounding and truncation as necessary
     */

    for ( i = 4*MAX_SIZE - 1; i >= 2*DEC_LOC; i-- )
          if ( temp[i] != 0 )
            break;

    prod->places_after = i - 2*DEC_LOC + 1;

    /* j = maximum places after */

    j = MAX_SIZE - prod->places_before;

    /* do we have to round and truncate? */

    if ( j < prod->places_after )
    {
        prod->places_after = j;
        if ( temp[2*DEC_LOC + j] > 4 )  /* we round up if > 4 */
            carry = 1;
        else
            carry = 0;

        /* copy the digits over */

        from = 2*DEC_LOC + j - 1;
        to   = DEC_LOC + j - 1;

        while ( j-- )
            (prod->term)[to--] = temp[from--];

        /* now do the rounding */

        if ( carry )
        {
            i = DEC_LOC + prod->places_after - 1;
            while ( 1 )
            {
                /*
                 * we now add carry (the rounding). If
                 * the current digit is 9, the carry will
                 * generate a 10, meaning that we have to
                 * carry to the next digit left. If this
                 * occurs at the leftmost digit, it can
                 * cause an overflow, so this possibility
                 * is checked for first.
                 */

                if ( i == 0 &&              /* overflow */
                     (prod->term)[i] > 8 )
                {
                    free ( temp );
                    return ( 0 );
                }

                (prod->term)[i] += carry;

                if ( (prod->term)[i] > 9 )
                {
                    (prod->term)[i] -= 10;
                    i--;
                }
                else
                    break;
            }
        }
    }
    else    /* no truncation so just copy the digits */
    {
        j = prod->places_after;

        from = 2*DEC_LOC + j - 1;
        to   = DEC_LOC + j - 1;

        while ( j-- )
            (prod->term)[to--] = temp[from--];
    }

    free ( temp );
    return ( 1 );
}

/*---------------------------------------------------------------
 * subtraction: We first determine where to start the process
 * of subtraction. It is the rightmost digit of the two terms,
 * that is to the right of the decimal point, else it is
 * at the first digit left of the decimal point. We then
 * proceed from that digit (the least significant digit) and
 * move to the left until we reach the leftmost digit of the
 * two terms. Because of the possibility of a carry, we go one
 * digit more to the left. This duplicates manual subtraction.
 *-------------------------------------------------------------*/
int NormalSubtract ( struct TermData * t1, struct TermData * t2,
                        struct TermData * diff )
{
    /*
     * The result of a subtraction is called a difference,
     * hence we named the variable containing the answer diff.
     */

    int carry, i, j;
    int max_after, max_before;
    char *p1, *p2, *pd;

    /*
     * we'll copy the terms to scratch since we'll be altering
     * them with any carries and borrows that we do.
     */

    char scratch1 [2*MAX_SIZE + 1],
         scratch2 [2*MAX_SIZE + 2];

    memcpy ( scratch1, t1->term, 2*MAX_SIZE + 1 );
    memcpy ( scratch2, t2->term, 2*MAX_SIZE + 1 );

    /* where to start subtracting? */

    max_before = max( t1->places_before, t2->places_before );
    max_after  = max( t1->places_after,  t2->places_after );

    j =  max_before + max_after;

    /*
     * at worst, the rightmost digit is the last digit left of
     * the decimal point. So, start no further left than there.
     */

    p1 = scratch1   + DEC_LOC + max_after - 1;
    p2 = scratch2   + DEC_LOC + max_after - 1;
    pd = diff->term + DEC_LOC + max_after - 1;

    while ( j >= 0 )
    {
        /*
         * if there is a carry, borrow 10 and
         * add 1 to the next higher digit in t2
         */

        if ( *p2 > *p1 )
        {
            *p1  += 10;
            *( p2 - 1 ) += 1;
        }

        *pd = *p1 - *p2;  /* the actual subtraction */

        pd--; p1--; p2--; /* move to the next higher digit */
        j--;
    }

    for ( j = 0, pd = diff->term;
          pd < diff->term + DEC_LOC; j++, pd++ )
            if ( *pd != 0 )
                break;

    diff->places_before = DEC_LOC - j;

    /*
     * to get the number of places after, start at max_after,
     * the maximum number of places after, and move left
     * until the first nonzero digit right of the decimal pt.
     */

    pd = diff->term + DEC_LOC + max_after - 1;
    while ( pd >= diff->term + DEC_LOC )
        if ( *pd == 0 )
        {
            max_after -= 1;
            pd -= 1;
        }
        else
            break;

    diff->places_after  = max_after;

    /*
     * There can be too many digits in the result: subtract
     * MAX_SIZE digits right of the decimal pt from MAX_SIZE
     * digits left of the decimal point. If this occurs, we
     * round and truncate.
     */

    i = diff->places_before + diff->places_after - MAX_SIZE;

    if ( i > 0 )    /* overflow */
    {
        diff->places_after = MAX_SIZE - diff->places_before;
        i = DEC_LOC + diff->places_after;
        if ( (diff->term)[i] > 4 )  /* round up if > 4 */
            carry = 1;
        else
            carry = 0;

        j = i;
        i -= 1;

        /* add carry (the rounding) */

        while ( carry )
        {
            if ( i == 0 && (diff->term)[i] > 8 )
            {
                printf ( "Overflow on subtraction\n" );
                return ( 0 );
            }

            (diff->term)[i] += carry;

            if ( (diff->term)[i] > 9 )
            {
                (diff->term)[i] -= 10;
                carry = 1;
                i--;
            }
            else
                carry = 0;
        }

        /* now zero out the digits we have truncated */

        while ( j < 2*MAX_SIZE )
            (diff->term)[j++] = 0;
    }

    return ( 1 );
}

/*--------------------------------------------------------------
 * Converts an null-terminated ASCII number in either normal or
 * scientific notation to a term.
 *
 * If the first character is a sign or a decimal point, we
 * process it and replace it with a 0, so as to enable
 * processing of a string of digits. However, to maintain the
 * integrity of the ASCII string, we save the original first
 * character in first_char and restore it before returning.
 * This is of use only when the calculator is reading input
 * from a file.
 *
 * Returns NORMAL, SCIENTIFIC depending on format of string,
 *  or returns 0 on error.
 *------------------------------------------------------------*/
int AsciiToTerm ( char *buffer, struct TermData *t )
{
    char * dec_pt,
         * p;
    int    i, exponent, len, notation;
    char * new_term;
    char   first_char;
    char * exp;         /* where the exponent flag is */

    new_term = t->term;   /* point new_term to where it'll go */

    /* is it scientific notation? */

    exp = strpbrk ( buffer, "eE" );
    notation = ( exp == NULL ? NORMAL : SCIENTIFIC );

    first_char = *buffer;

    if ( isdigit ( *buffer ))       /* get the sign */
        t->sign = +1;
    else
    {
        if ( *buffer == '-' )
            t->sign = -1;
        else
        if ( *buffer == '+' )
            t->sign = +1;
        else
        if ( *buffer != '.' )
        {
            t->sign = 0;    /* flag error */
            return ( 0 );
        }

        /* replace any leading sign by a zero */

        if ( *buffer != '.' )
            *buffer = '0';
    }

    dec_pt = strchr ( buffer, '.' );
    len = strlen ( buffer );

    /*
     * load the digits after the decimal point. The first
     * digit goes at term[DEC_LOC], subsequent digits
     * go to the right up to MAX_SIZE digits.
     */

    if ( dec_pt != NULL )  /* Only if there's a decimal point */
    {
        p = dec_pt + 1;
        for ( i = DEC_LOC; *p && i != 2*MAX_SIZE; i++, p++ )
        {
            if ( ! isdigit ( *p ))  /* check that it's a digit */
            {
                if ( notation == NORMAL )
                {
                    t->sign = 0;    /* if not, show an error */
                    break;
                }
                else   /* if it's scientific notation... */
                {
                    if ( *p == 'e' || *p == 'E' )
                        break;
                    else
                    {
                        t->sign = 0;    /* show an error */
                        break;
                    }
                }
            }
            else
                new_term[i] = *p - '0';
        }
    }

    if ( notation == SCIENTIFIC )
    {
        p = exp + 1;
        exponent = atoi ( p );
    }

    if ( t->sign == 0 )     /* any error so far ? */
        return ( 0 );

    /*
     * load the digits before the decimal point. You load at
     * the first place right of buffer[DEC_LOC] and add digits
     * to the left up to MAX_SIZE digits.
     */

    if ( dec_pt == NULL )
    {
        if ( notation == NORMAL )
            p = buffer + len - 1;
        else
            p = exp - 1;
    }
    else
        p = dec_pt - 1;


    for ( i = DEC_LOC - 1; p >= buffer && i >= 0; i--, p-- )
    {
        if ( ! isdigit ( *p ))  /* check that it's a digit */
        {
            t->sign = 0;        /* if not, indicate an error */
            break;
        }
        else
        {
            new_term[i] = *p - '0';
            if ( p == buffer ) /* this test for pointer wrap- */
                break;          /* around on Intel segments   */
        }
    }

    /*
     * if it's scientific notation, shift the term right or left
     * depending on the exponent. If the exponent is > 0, shift
     * left exponent number of places; if it's < 0, shift right.
     */

    if ( notation == SCIENTIFIC )
    {
        if ( exponent > 0 )
        {
            while ( exponent-- )    /* shift left */
            {
                if ( *new_term > 0 )
                {
                    printf ( "Error: %s too large\n", buffer );
                    return ( 0 );
                }

                memmove ( new_term, new_term + 1,
                                            2*MAX_SIZE - 1 );
                new_term[2*MAX_SIZE - 1] = 0;
            }
        }
        else
        if ( exponent < 0 )         /* shift right */
        {
            int warning = 0;

            while ( exponent++ )
            {
                if ( new_term[2*MAX_SIZE - 1] > 0 )
                    warning += 1;

                memmove ( new_term + 1, new_term,
                                            2*MAX_SIZE - 1 );
                *new_term = 0;
            }

            if ( warning )
                printf ( "Low order truncation of % digits.\n",
                            warning );
        }
    }       /* note: if exponent = 0, no shift occurs */

    /*
     * find out how many places before decimal point:
     * start at new_term[0] and move left until you encounter
     * the first non-zero digit or the decimal point.
     * Minimum places_before = 1.
     */

    for ( p = new_term;
          *p == '\0' &&  p < new_term + DEC_LOC; p++ )
              ;       /* just loop to the first non-zero digit */

    t->places_before = ( new_term + DEC_LOC ) - p;

    /*
     * find out how many places after decimal point:
     * if there was a decimal point, then start at right end
     * of new_term and go left until you encounter the first
     * non-zero digit or the decimal point. If there was no
     * decimal point, then places_after = 0.
     */

    if ( dec_pt == NULL )
        t->places_after = 0;
    else
    {
        for ( p = new_term + 2*MAX_SIZE;
              *p == '\0' && p >= new_term + DEC_LOC;
              p-- )
                ;   /* just loop to the first non-zero digit */

        t->places_after = ( p - ( new_term + DEC_LOC ) + 1 );
    }

    *buffer = first_char;

    return ( notation );
}

/*--------------------------------------------------------------
 * Converts a normalized term into an ASCII string
 *------------------------------------------------------------*/
void TermToAscii ( struct TermData * t, char *ascii,
                   int notation )
{
    char *first,        /* first printing digit */
         *last,         /* last printing digit */
         *output;       /* where the ascii string is built */

    first = t->term;
    output = ascii;

    /* skip leading zeros */

    while ( *first == '\0' && first < t->term + 2*MAX_SIZE )
        first++;

    /* if at end, print answer of 0 and exit */

    if ( first == t->term + 2*MAX_SIZE )
    {
        *output++ = '0';
        *output = '\0';
        return;
    }
    else    /* if past decimal point, start at dec. pt. */
    {
        if ( first >= t->term + DEC_LOC )
             first  = t->term + DEC_LOC;
    }

    /* find last non-zero digit up to decimal point */

    last = t->term + 2*MAX_SIZE;

    while ( last > first &&
            last >= t->term + DEC_LOC &&
           *last == '\0' )
                last--;

    /* before beginning printing, check the sign */

    if ( t->sign == -1 )
        *output++ = '-';

    /* if we start at the decimal point, print leading 0 */
    if ( first == t->term + DEC_LOC )
        *output++ = '0';

    while ( first <= last )
    {
        if ( first == t->term + DEC_LOC )
            *output++ = '.';

        *output++ = *first + '0';
        first += 1;
    }

    *output = '\0';

   if ( notation == SCIENTIFIC )
        AsciiToScientific ( ascii );

}

/*--------------------------------------------------------------
 * Takes a string in normal notation and converts it to
 * scientific notation of the form:
 *                  [-](0-9).(0-9)*e[+|-](0-9)*
 * The final string overlays the original string. This could be
 * dangerous since the new form is conceivably longer than the
 * original form. However, we know the input string is twice
 * MAX_SIZE, so there should always be enough room to fit. This
 * routine would have to modify this aspect if it were to be
 * used as a generic format conversion function.
 *-------------------------------------------------------------*/
int AsciiToScientific ( char *ascii )
{
    char * buffer,
         * dec_pt,      /* location of the decimal pt, if any */
         * first_digit; /* where the first non-zero digit is */

    char str_exp[8];    /* will hold exponent string */


    int i, j, ascii_len, exponent;

    i = j = exponent = 0;

    ascii_len = strlen ( ascii );

    buffer = malloc ( ascii_len + 6 );
    if ( ascii_len < 1 || buffer == NULL )
        return ( 0 );

    /* process the sign */

    if ( ! isdigit ( *ascii ))
    {
        if ( *ascii == '-' )
            buffer[i++] = ascii[j++];
        else
        if ( *ascii != '+' )
        {
            printf (
                "Invalid number for scientific format\n" );
            return ( 0 );
        }
    }

    /* now process the digits. First check for decimal point. */

    dec_pt = strchr ( ascii, '.' );

    /* skip to the first non-zero digit */

    while ( ascii[j] == '.' || ascii[j] == '0' )
    {
        j +=1;
        if ( ascii[j] == '\0' )    /* end of string */
        {
            printf (
                "Invalid number for scientific format\n" );
            return ( 0 );
        }
    }

    first_digit = &ascii[j];

    buffer[i++] = ascii[j++];
    buffer[i++] = '.';
    if ( ! ascii[j] )       /* e.g. 6 = 6.0e1 */
        buffer[i++] = '0';
    else
        while ( ascii[j] )
            if ( ascii[j] == '.' )
                j += 1;
            else
                buffer[i++] = ascii[j++];

    /* do we have trailing zeros? Trim 1.600e4 to 1.6e4 */

    j = i - 1;
    while ( j > 1 && buffer[j - 1] != '.' )
    {
        if ( buffer[j--] == '0' )
            i -= 1;
        else
            break;
    }

    /* compute the exponent */

    if ( dec_pt )
    {
        if ( first_digit > dec_pt ) /* e.g., 0.0065  */
            exponent = ( first_digit - dec_pt ) * -1;
        else                        /* e.g., 650.2   */
            exponent = dec_pt - first_digit - 1;
    }
    else  /* no decimal point */
    {
        exponent = ascii_len - 1;
        if ( ! isdigit ( *ascii ))  /* was there a + or - ? */
            exponent -= 1;
    }

    /* output the exponent */

    buffer[i++] = 'e';

    memset ( str_exp, '\0', 8 );
    sprintf ( str_exp, "%d", exponent );
    strcpy ( buffer + i, str_exp );

    /* overlay the original string */

    strcpy ( ascii, buffer );
    free ( buffer );
    return ( 1 );
}

/*--------------------------------------------------------------
 * Initialize a term to zero.
 *------------------------------------------------------------*/
void TermInit ( struct TermData *t )
{
    t->places_before = 0;
    t->places_after  = 0;
    t->sign          = 1;
    memset ( t->term, '\0', 2*MAX_SIZE + 1 );
}

/*--------------------------------------------------------------
 * Copy a term from the second argument to the first.
 * Returns a pointer to the copied string.
 *------------------------------------------------------------*/
struct TermData * TermCopy ( struct TermData * dest,
                             struct TermData * src )
{
    dest->sign          = src->sign;
    dest->places_before = src->places_before;
    dest->places_after  = src->places_after;

    memcpy ( dest->term, src->term, 2*MAX_SIZE + 1 );

    return ( dest );
}

/*--------------------------------------------------------------
 * For compilers that do not have this non-ANSI function.
 * strrev() accepts a pointer to a string and reverses the bytes
 * in the string. It overlays the original string with the
 * reversed string. The function always returns a pointer to the
 * string, regardless of whether an error occurred.
 *-------------------------------------------------------------*/
char *strrev ( char *str )
{
    int i, j, len;
    char * pc;

    len = strlen ( str );
    pc  = malloc ( len + 1 );

    if ( pc == NULL )
        return ( str );

    memcpy ( pc, str, len + 1 );

    for ( i = len - 1, j = 0; i >= 0; i--, j++ )
            str[j] = pc[i];

    str[j] = '\0';

    free ( pc );
    return ( str );
}

