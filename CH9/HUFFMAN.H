/*--- huffman.h --------------------------- Listing 9-5 -------*
 * Header file for Huffman compression routines                *
 *-------------------------------------------------------------*/

/* Max number of unique symbols or codes */
#define MAXSYMBOLS 256

/* Total number of external and internal nodes */
#define MAXNODES   (MAXSYMBOLS*2-1)

/* Child directions */
#define LEFT  0
#define RIGHT 1

typedef unsigned long CountType;
typedef int LinkType;

/* Header byte structure */
struct _Header {
    LinkType index;
    LinkType child[2];
};

#define SIGNATURE "Huff1"
