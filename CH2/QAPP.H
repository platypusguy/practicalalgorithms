/*--- qapp.h ------------------------------ Listing 2-9 --------
 *  Application-specific data items for linked-list queues.
 *-------------------------------------------------------------*/
#ifndef QAPP_H
#define QAPP_H 1

/*
 *  Our first list's nodes consist of a pointer to
 *  text and a priority level.
 */

#define TEXT_SIZE 60    /* the maximum size of our text string */

extern int DataCopy ( void *, void * );

/*
 * The data and functions for the queue
 */

struct NodeData1 {
    char *text;
    unsigned int priority;
};

typedef struct NodeData1 * pND1;

extern void * CreateData1 ( void * );
extern int    DeleteData1 ( void * );
extern int    DuplicatedNode1 ( Link, Link );
extern int    NodeDataCmp1 ( void *, void * );


/*
 *  The free list has identical nodes
 */

struct NodeData2 {
    char *text;
    unsigned int priority;
};

typedef struct NodeData2 * pND2;

extern void * CreateData2 ( void * );
extern int    DeleteData2 ( void * );
extern int    DuplicatedNode2 ( Link, Link );
extern int    NodeDataCmp2 ( void *, void * );

#endif
