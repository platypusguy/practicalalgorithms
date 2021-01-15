/*--- llgen.h ----------------------------- Listing 2-2 ---------
 *  Declarations for generic doubly linked lists.
 *  Used in conjunction with llgen.c (Listing 2-3).
 *-------------------------------------------------------------*/
#ifndef LLGEN_H       /* make sure it's included only once */
#define LLGEN_H    1


struct Node {
    struct Node     *prev;  /* link to previous node */
    struct Node     *next;  /* link to next node */
    void            *pdata; /* generic pointer to data */
};

typedef struct Node *Link;

/* a linked list data structure */
struct List {
    Link            LHead;
    Link            LTail;
    unsigned int    LCount;
    void * ( * LCreateData )     ( void * );
    int    ( * LDeleteData )     ( void * );
    int    ( * LDuplicatedNode ) ( Link, Link );
    int    ( * LNodeDataCmp )    ( void *, void * );
};

/* The four functions specific to an individual linked list are:

   LCreateData:  is passed a pointer to an application-defined
                 object and is expected to return a pointer to
                 whatever is to be stored in the linked list.

   LDeleteData:  is passed a pointer to the object an application
                 has stored in a linked list. LDeleteData must
                 destroy the object.

   LDuplicatedNode: is passed two pointers. The first pointer is
                    to a node that you would like to add to a
                    linked list and the second is to a node that
                    is already in the list but is a duplicate of
                    the first pointer.
                    LDuplicatedNode returns:
                            0 -> do nothing to list
                            1 -> destroy duplicate
                            2 -> add duplicate to list

   LNodeDataCmp: is passed pointers to two application data
                 objects and must compare them, returning a
                 number that is < 0, zero, or > 0, depending on
                 the relationship between the first and second
                 objects.
*/

/*--- generic linked-list primitives ---*/
int  AddNodeAscend  ( struct List *, void * );
int  AddNodeAtHead  ( struct List *, void * );
struct List * CreateLList (
                void * ( * ) ( void * ),       /* create data */
                int    ( * ) ( void * ),       /* delete data */
                int    ( * ) ( Link, Link ),   /* duplicate   */
                int    ( * )  ( void *, void * )); /* compare */
Link CreateNode     ( struct List * , void * );
int  DeleteNode     ( struct List *, Link );
Link FindNode       ( struct List *, void * );
Link FindNodeAscend ( struct List *, void * );
Link GotoNext       ( struct List *, Link );
Link GotoPrev       ( struct List *, Link );
#endif
