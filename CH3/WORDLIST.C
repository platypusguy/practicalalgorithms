/*--- wordlist.c -------------------------- Listing 3-4 -------
 *  Lists all words in a text file by storing them in a hash
 *  table. Must be linked to the linked-list primitives of
 *  Chapter 2 and to ElfHash() of Listing 3-3.
 *-------------------------------------------------------------*/

   /* uncomment the following line to print all words
    * with their associated hash value.
    */

/* #define LIST_HASH  1 */

   /* uncomment the following line to print the unique words
    * and a count of their frequency.
    */

/* #define LIST_WORDS 1 */

   /* comment out the following line if you do not want an
    * an analysis of the hashing function and hash table load.
    */

#define LIST_TABLE_STATS 1

   /* uncomment the following line if you want a listing
    * of all words > 10 letters. Such words are often typing
    * or scanning errors in the text or odd constructs.
    */

#define LIST_LONG_WORDS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "llgen.h"          /* Header for generic linked lists */
#include "llapp.h"          /* Header for appl.'s linked lists */

extern unsigned long ElfHash ( char *);

/*--- the hash table portion ---*/

#define TABLE_SIZE  1999    /* Number of slots; a prime number */

Link   *Table;              /* Our table is an array of Links */

#if 0
int CreateTable ( Link **t )
{
    *t = calloc ( TABLE_SIZE, sizeof ( Link ));
    return ( *t == NULL ? 0 : 1 );
}
#endif

/*--------------------------------------------------------------
 * We use calloc() to allocate the table. However, on some
 * compilers the bit pattern used by calloc() for initialization
 * is not the same as NULL (which is what we want). So we check
 * for this condition and, if it occurs, intialize the table by
 * hand to NULLs.
 *-------------------------------------------------------------*/
int CreateTable ( Link **t )
{
    int i;

    *t = (Link *) calloc ( TABLE_SIZE, sizeof ( Link ));
    if ( *t == NULL )
        return ( 0 );

    if ( **t != NULL )      /* is the calloc'd memory == NULL? */
    {
        for ( i = 0; i < TABLE_SIZE; i++, t++ )
            **t = NULL;
    }

    return ( 1 );
}


/*--------------------------------------------------------------
 * Function to convert a string to upper case. Exists in most
 * PC C libraries but missing from many UNIX C libraries.
 *-------------------------------------------------------------*/
char *strupr ( char *str )
{
    char *s = str;

    while ( *s )
    {
        *s = toupper ( *s );
        s += 1;
    }

    return ( str );
}

/* === main line === */

int main ( int argc, char *argv[] )
{
    char     word[64];        /* the raw word from the file */
    char     *pw;             /* pointer to the word */
    int      c, i, j;
    int      chains,          /* counts how many chains     */
             chain_table[33]; /* table of chain lengths     */
                              /*    for our report.         */
    int      add_status;      /* return value from table add*/

    unsigned hash_key;

    struct List *L1,          /* list for hash table entries*/
                *long_wd;     /* list of long words         */

    struct NodeData1 nd;      /* the node we add each time  */
    struct Node      n;       /* used for scratch purposes  */

    FILE     *fin;            /* the input file             */

    if ( argc < 2 )
    {
        fprintf ( stderr, "Error! Usage: wordlist filename\n" );
        exit ( EXIT_FAILURE );
    }

    if ( argc > 2 )
        fprintf
            ( stderr, "Warning: Usage: wordlist filename\n" );

    fin = fopen ( argv[1], "rt" );
    if ( fin == NULL )
    {
        fprintf ( stderr, "Could not find/open %s\n", argv[1] );
        exit ( EXIT_FAILURE );
    }

    /*--- create the table ---*/

    if ( ! CreateTable ( &Table ))
    {
        fprintf ( stderr, "Error! Could not create table\n" );
        exit ( EXIT_FAILURE );
    }

    /*--- set up linked-list data structures ---*/

    L1 = CreateLList ( CreateData1,      /* in llapp.c */
                       DeleteData1,      /*     "      */
                       DuplicatedNode1,  /*     "      */
                       NodeDataCmp1 );   /*     "      */

    long_wd = CreateLList ( CreateData2, /* in llapp.c */
                       DeleteData2,      /*     "      */
                       DuplicatedNode2,  /*     "      */
                       NodeDataCmp2 );   /*     "      */

    if ( L1 == NULL || long_wd == NULL )
    {
        fprintf ( stderr, "Error creating linked list\n" );
        exit ( EXIT_FAILURE );
    }

    /*--- begin processing file ---*/

    c = ' ';

    while ( ! feof ( fin ))
    {
        /*--- skip white space ---*/

        while ( c != EOF && isspace ( c ))
            c = fgetc ( fin );

        /*--- pick up the word ---*/
        i = 0;
        while ( c!= EOF && !isspace ( c ))
        {
            word[i++] = c;
            c = fgetc ( fin );
        }

        if  ( c == EOF )
            break;

        word[i] = '\0';

        /*--- strip off trailing punctuation ---*/

        while ( i >= 0 && ispunct ( word[--i] ))
            word[i] = '\0';

        pw = strupr ( word );

        /*--- get the hash value ---*/

        hash_key = (unsigned int) ElfHash ( pw );
        hash_key %= TABLE_SIZE;

#ifdef LIST_HASH
        printf ( "%15s  %3d\n", pw, hash_key );
#endif

        /*--- insert into table ---*/

        L1->LHead = Table[hash_key];

        nd.word =  pw;              /* the string we're adding */
        nd.u    =  1;               /* adding one occurrence   */

        add_status =  AddNodeAscend ( L1, &nd );
        if ( add_status == 0 )      /* an error occurred */
            printf ( "Warning! Error while allocating node.\n" );

        Table[hash_key] = L1->LHead;

        /*--- handle long words ---*/

        /* if a word is longer than ten chars, put it in the
         * long word list for subsequent display.
         */

        if ( strlen ( pw ) > 10 )
            AddNodeAscend ( long_wd, &nd );

        /* if a word is longer than 20 chars, it's likely to
         * be a typo or other error; so delete it. This pro-
         * cessing is included primarily to exercise these
         * functions. It should be removed for real text
         * processing.
         */

        if ( strlen ( pw ) > 20 )
        {
            Link pl;

            pl =  FindNodeAscend ( L1, &nd );
            if ( pl == NULL )
                printf ( "processing error!\n" );
            else
                DeleteNode ( L1, pl );
        }
    }

    /*--- now dump the table ---*/

    for ( j = 0; j < 33; j++ )
          chain_table[j] = 0;

    for ( i = 0; i < TABLE_SIZE; i++ )
    {
        Link pcurr;     /* Node we're examining */

        pcurr = Table[i];             /* set to start of list */
        if ( pcurr == NULL )          /* skip empty slots */
            continue;
        else
        {
            int chain_len;

            for ( chain_len = 0; ; pcurr = pcurr->next )
            {
                memcpy ( &n, pcurr, sizeof ( struct Node ));

#ifdef LIST_WORDS
              /* Print each word and the count of occurrences */
                printf ( "%-20s  %3u\n",
                         ( (pND1) n.pdata)->word,
                         ( (pND1) n.pdata)->u );
#endif
                chain_len += 1;

                if ( pcurr->next == NULL )
                    break;
            }

            if ( chain_len > 32 )
                 chain_len = 32;

            chain_table[chain_len] += 1;
        }
    }

#ifdef LIST_LONG_WORDS
    if ( long_wd->LCount < 1 )
        printf ( "No long words!\n" );
    else
    {                       /* step thru the list and print it*/
        Link pcurr;

        for ( pcurr =  long_wd->LHead;
              pcurr != NULL;
              pcurr =  GotoNext ( long_wd, pcurr))
                printf ( "%-20s\n",
                            ((pND2)( pcurr->pdata ))->word );
    }
#endif

#ifdef LIST_TABLE_STATS

    chains = 0;
    for ( i = 32; i > 0; i-- )
    {
        if ( chain_table[i] == 0 )
            continue;
        else
        {
            printf ( "%3d chains of length %2d\n",
                      chain_table[i], i );
            chains += chain_table[i];
        }
    }

    if ( chains != 0 )
    {
        printf ( "\n%d Nodes in %u chains\n\n",
                 L1->LCount, chains );

        printf ( "Size of hash table   = %u\n",
                    (unsigned) TABLE_SIZE );

        printf ( "Average chain length = %f\n",
                  L1->LCount / (double)chains );

        printf ( "Slot Occupancy       = %f \n",
                 ( (double)chains ) / TABLE_SIZE );

        printf ( "Load Factor          = %f \n",
                 ( (double)L1->LCount ) / TABLE_SIZE );
    }
    else
        printf ( "Error! No chains found.\n" );
#endif

    return ( EXIT_SUCCESS );
}
