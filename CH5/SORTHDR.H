/*--- sorthdr.h --------------------------- Listing 5-3 -------
 * General definitions for array-oriented sort routines
 *-----------------------------------------------------------*/

/*-------------------------------------------------------
 * We sort an array of pointers to this structure. The
 * structure could contain anything you'd like: you only
 * need to define appropriate comparison functions.
 *-----------------------------------------------------*/

typedef struct sElement {
    char *text;
}
    Element;

/*------------------------------------------------------
 * A type for the comparison function: a symbol with
 * type CompFunc is a pointer to a function that takes
 * two pointers to void and returns an int.
 *----------------------------------------------------*/

typedef int (*CompFunc) ( void *, void * );

