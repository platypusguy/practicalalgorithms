/*--- qdriver.c --------------------------- Listing 2-11 --------
 * Reads in a data file consisting of lines of text of the form
 *                          X9Message
 * where X = A for add to queue, D = delete, P = print the queue
 *       9 = priority of the queued item
 *       Message = string to enqueue
 * Note: actions D and P have no priority or message.
 * As each action is performed, a status message is printed.
 *
 * Must be linked with object files from qapp.c and llgen.c
 *-------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "llgen.h"
#include "qapp.h"

int dequeue ( struct List *, struct List *, void * );
int enqueue ( struct List *, struct List *, void * );

#define QMAX 100    /* maximum number of elements in a queue */

main ( int argc, char *argv [] )
{
    char     record[64];      /* the raw word from the file */
    int      count;
    void *   temp;            /* temporary data area */

    struct  List *queue,
                 *free_list;  /* our two queues */

    FILE     *fin;            /* the input file */

    if ( argc != 2 )
    {
        fprintf ( stderr, "Error! Usage: qdriver filename\n" );
        exit ( EXIT_FAILURE );
    }

    fin = fopen ( argv[1], "rt" );
    if ( fin == NULL )
    {
        fprintf ( stderr, "Could not find/open %s\n", argv[1] );
        exit ( EXIT_FAILURE );
    }

    /*--- set up linked-list data structures for queues ---*/

    queue
       = CreateLList ( CreateData1,      /* in qapp.c */
                       DeleteData1,      /*     "     */
                       DuplicatedNode1,  /*     "     */
                       NodeDataCmp1 );   /*     "     */
    free_list
       = CreateLList ( CreateData2,      /* in qapp.c */
                       DeleteData2,      /*     "     */
                       DuplicatedNode2,  /*     "     */
                       NodeDataCmp2 );   /*     "     */

    if ( queue == NULL || free_list == NULL )
    {
        fprintf ( stderr, "Error creating queue\n" );
        exit ( EXIT_FAILURE );
    }

    /*--- allocate the free list ---*/

    for ( count = 0; count < QMAX; count++ )
    {
        if ( ! AddNodeAtHead ( free_list, record ))
        {
            fprintf
                ( stderr, "Could not create queue of %d\n",
                    QMAX );
            exit ( EXIT_FAILURE );
        }
    }

    /*--- begin processing file ---*/

    temp = CreateData2 ( NULL );
    if ( temp == NULL )
    {
        fprintf ( stderr, "Error creating temporary data area\n" );
        exit ( EXIT_FAILURE );
    }

    while ( fgets ( record, 64, fin ) != NULL )
    {
        if ( strlen ( record ) > 0 )
            record[strlen ( record ) - 1] = 0; /* strip CR/LF */

        if ( *record == 'A' )   /* add */
        {
            ((pND2)temp)->priority = *( record + 1 ) - '0';
            ((pND2)temp)->text = record + 2;

            if ( enqueue ( queue, free_list, temp ) == 0 )
            {
                printf ( "Error enqueueing %d %s\n",
                          ((pND2)temp)->priority,
                          ((pND2)temp)->text );
                exit ( EXIT_FAILURE );
            }
            else
            {
                printf ( "Enqueued %d %s\n",
                          ((pND2)temp)->priority,
                          ((pND2)temp)->text );

                if ( queue->LCount == 0 )
                    printf ( "Empty queue\n" );
                else
                {
                    Link curr;
                    printf ( "-------- List so far-------\n" );
                    for ( curr = queue->LHead;
                          curr != NULL;
                          curr = curr->next )
                            printf ( "%d %s\n",
                                ((pND2)(curr->pdata))->priority,
                                ((pND2)(curr->pdata))->text );
                }
            }
        }
        else
        if ( *record == 'D' )   /* delete */
        {
            if ( dequeue ( queue, free_list, temp ) == 0 )
            {
                printf ( "Error dequeueing %d %s\n",
                          ((pND2)temp)->priority,
                          ((pND2)temp)->text );
                return ( EXIT_FAILURE );
            }
            else
                printf ( "Dequeued %d %s\n",
                          ((pND2)temp)->priority,
                          ((pND2)temp)->text );
        }
        else
        if ( *record == 'P' )   /* print */
        {
            if ( queue->LCount == 0 )
                printf ( "Empty queue\n" );
            else
            {
                Link curr;
                printf ( "\n-------- List so far-------\n" );
                for ( curr = queue->LHead;
                      curr != NULL;
                      curr = curr->next )
                        printf ( "%d %s\n",
                            ((pND2)(curr->pdata))->priority,
                            ((pND2)(curr->pdata))->text );
            }
        }
        else
            fprintf ( stderr, "Data error: %s\n", record );
    }

    fclose ( fin );
    return ( EXIT_SUCCESS );
}
/*---------------------------------------------------------------
 * enqueue loads the data items in entry into the head node of
 * the free list, then adds that node to the queue based on
 * priority.
 *-------------------------------------------------------------*/

int enqueue ( struct List *lqueue, struct List *lfree,
                void *new_entry )
{
    Link curr, new_node;

    /* Are there any free nodes left? */

    if ( lfree->LCount == 0 )
    {
        fprintf ( stderr, "Exceeded maximum queue size\n" );
        return ( 0 );
    }

    /* load the data into the head of the free list */

    new_node = lfree->LHead;

    if ( DataCopy ( new_node->pdata, new_entry ) == 0 )
         return  ( 0 );

    /* adding to an empty list? */

    if ( lqueue->LCount == 0 )
    {
        lfree->LHead = lfree->LHead->next;

        new_node->prev = NULL;
        new_node->next = NULL;

        lqueue->LTail = new_node;
        lqueue->LHead = new_node;

        lqueue->LCount = 1;
        lfree->LCount -= 1;

        return ( 1 );
    }
    else
    /* Traverse the list to find the insertion position */

    for ( curr = lqueue->LHead; ; curr = curr->next )
    {
        if ( curr == NULL          /* at end of queue */
                ||               /* or at insertion point */
                NodeDataCmp1 ( new_entry, curr->pdata ) < 0 )
        {
            new_node = lfree->LHead;
            lfree->LHead = lfree->LHead->next;

            if ( curr == NULL )   /* if end of list */
            {
                new_node->prev = lqueue->LTail;
                new_node->next = NULL;
                new_node->prev->next = new_node;
                lqueue->LTail = new_node;
            }
            else
            {
                if ( curr->prev == NULL )   /* adding at head? */
                    lqueue->LHead = new_node;

                new_node->prev = curr->prev;
                new_node->next = curr;

                if ( curr->prev != NULL )
                    curr->prev->next = new_node;
                curr->prev = new_node;
            }

            lqueue->LCount += 1;

            /* update the free list */

            lfree->LCount -= 1;

            return ( 1 );
        }
        else
        {
            pND2 p1, p2;
            p1 = curr->pdata;
            p2 = new_entry;
            printf ( "searched at %d %s to insert %d %s\n",
                      p1->priority, p1->text,
                      p2->priority, p2->text );
        }
    }
}

/*---------------------------------------------------------------
 * dequeue takes a pointer that will be set to the data in the
 * node at the head of the queue. It then moves the node being
 * dequeued from the queue to the free list. Note that if you do
 * not use the dequeued data before next queue operation, the
 * data is lost, so copy it if you need to. Returns 0 on error.
 *-------------------------------------------------------------*/

int dequeue ( struct List *lqueue, struct List *lfree,
                void * our_data )
{
    Link dequeued_link;

    /* is there anything to dequeue? */

    if ( lqueue->LCount == 0 )
    {
        fprintf ( stderr, "Error dequeue from empty queue\n" );
        return ( 0 );
    }

    /* make a copy of the data being dequeued */

    if ( DataCopy ( our_data, lqueue->LHead->pdata ) == 0 )
        return ( 0 );

    /* remove the node from the queue */

    dequeued_link = lqueue->LHead;
    lqueue->LHead = lqueue->LHead->next;
    lqueue->LCount -= 1;

    /* add the node to the free list */

    dequeued_link->prev = NULL;
    dequeued_link->next = lfree->LHead;
    lfree->LHead = dequeued_link;
    lfree->LCount += 1;

    return ( 1 );
}

