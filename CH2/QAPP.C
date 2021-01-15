/*--- qapp.c ------------------------------ Listing 2-10 --------
 *  Application-specific functions for queue examples.
 *  Replace these routines with your own.
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>         /* for free() */
#include <string.h>         /* for strcmp() and strdup() */

#include "llgen.h"
#include "qapp.h"

/*=== linked-list functions for queue ===*/

/*
 * our nodes come from the free list,
 * so this function is never called.
 */
void * CreateData1 ( void * data )
{
    return ( NULL );
}

int DeleteData1 ( void * data )
{
    /*
     * In this case, NodeData1 consists of a pointer and an int.
     * The integer will be returned to memory when the node
     * is freed. However, the string must be freed manually.
     */
     free ( ((pND1) data)->text );
     return ( 1 );
}
/*---------------------------------------------------------------
 * This function determines what to do when inserting a node
 * into a list if an existing node with the same data is found
 * in the list. In this case, since we are counting words, if a
 * duplicate word is found, we simply increment the counter.
 *
 * Note this function should return one of the following values:
 *      0       an error occurred
 *      1       delete the duplicate node
 *      2       insert the duplicate node
 * Any other processing on the duplicate should be done in this
 * function.
 *-------------------------------------------------------------*/

int DuplicatedNode1 ( Link new_node, Link list_node )
{
    return 2;
}

/* compare only the priority of the queue data */
int NodeDataCmp1 ( void *first, void *second )
{
    return (  ( ((pND2) first)->priority -
                ((pND2) second)->priority ));
}

/*=== Now the functions for the list of free nodes ===*/

/* data is a priority level (int) and text (string) */
void * CreateData2 ( void * data )
{
    struct NodeData1 * new_data;

    /*--- allocate our data structure ---*/

    new_data = malloc ( sizeof ( struct NodeData1 ));
    if ( new_data == NULL )
        return ( NULL );

    /*--- move the values into the data structure ---*/

    /*
     * we assign a priority of 0
     * and allocate a string of TEXT_SIZE + 1
     */

    new_data->priority =  0;
    new_data->text     =  (char *) malloc ( TEXT_SIZE + 1);

    if ( new_data->text == NULL )   /* error copying string */
    {
        free ( new_data );
        return ( NULL );
    }
    else
        return ( new_data ); /* return a complete structure */
}

int DeleteData2 ( void * data )
{
    /*
     * In this case, NodeData2 consists of a pointer.
     * The string must be freed manually.
     */

     free ( ((pND2) data)->text );
     return ( 1 );
}

/* this list inserts duplicated nodes */
int DuplicatedNode2 ( Link new_node, Link list_node )
{
    return 2;
}

/* this function is never called */
int NodeDataCmp2 ( void *first, void *second )
{
    return ( 0 );
}

/* function to copy our data */

int DataCopy ( void * dest, void * src )
{
    pND2 s, d;
    s = src;
    d = dest;

    if ( dest == NULL || src == NULL )
        return ( 0 );


    printf ( "About to copy %d - %s \n",
                s->priority, s->text );

    d->priority = s->priority;
    strncpy ( d->text, s->text, TEXT_SIZE );

    return ( 1 );
}
