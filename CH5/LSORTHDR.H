/*--- lsorthdr.h -------------------------- Listing 5-15 -------
 * General definitions for linked-list oriented sort routines
 *-------------------------------------------------------------*/

/*
 * We sort a linked list of these structures. The structure
 * could contain anything you'd like: you need only to define
 * appropriate comparison functions.
 */
typedef struct sNode {
    char *text;
    struct sNode *next;
} Node;

/*
 * A type for the comparison function: a symbol with type
 * CompFunc is a pointer to a function that takes two pointers
 * to void and returns int
 */
typedef int (*CompFunc) ( void *, void * );
