/*--- llapp.h ----------------------------- Listing 2-4a -------
 *  Application-specific data for linked lists.
 *  Called by llgen.h from the application.
 *-------------------------------------------------------------*/
#ifndef LLAPP_H
#define LLAPP_H 1

#include <string.h>

/*
 *  Our nodes consist of a pointer to
 *  a word and a count of occurrences.
 */

struct NodeData1 {
    char *word;
    unsigned int u;
};

typedef struct NodeData1 * pND1;

extern void * CreateData1 ( void * );
extern int    DeleteData1 ( void * );
extern int    DuplicatedNode1 ( Link, Link );
extern int    NodeDataCmp1 ( void *, void * );


/*
 *  Our second list's nodes consist of a
 *  pointer to a word.
 */

struct NodeData2 {
    char *word;
};

typedef struct NodeData2 * pND2;

extern void * CreateData2 ( void * );
extern int    DeleteData2 ( void * );
extern int    DuplicatedNode2 ( Link, Link );
extern int    NodeDataCmp2 ( void *, void * );

#endif
