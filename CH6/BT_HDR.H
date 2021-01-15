/*--- bt_hdr.h ---------------------------- Listing 6-3 ---------
 * B-tree header file, to be included by user code
 *
 *-------------------------------------------------------------*/

#if !defined(BT_HDR_H)
#define BT_HDR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Some defines
 */
#define MAX_PATH_AND_FILE 100  /* max length of full file name */
#define TREE_OK   (0)
#define TREE_FAIL (-1)

/*
 * Some general typedefs
 */
typedef unsigned BLKSIZE; /* Holds size of disk blocks */
typedef long DISKOFFSET;  /* File offset for fseek */

/*
 * Structure of index files:
 *
 * First block (offset 0L, size is determined by size_struct):
 *   Signature string: SIGNATURE SIG_NDX
 *
 *   Sizes:            Sizes size_struct;
 *
 *	 Root block:	   DISKOFFSET root_block;
 *	 Free block chain: DISKOFFSET free_block;
 *
 *
 * Data blocks
 *   Block control:  BlockControl blockdata;
 *                     If blockstate == sNode, then the
 *                     DISKOFFSETS in this block are pointers
 *                     to additional index blocks.
 *                     If blockstate == sLeaf, then DISKOFFSET
 *                     is a pointer to a data block in the
 *                     .dat file
 *
 *                   key/DISKOFFSET pairs
 *
 *                     The user-supplied function get_key_size()
 *                     is used to step through the block and the
 *                     user-supplied functions key2keycmp() and
 *                     key2reccmp() are used to compare keys.
 */

/*
 * Structure of data files:
 *
 * First block (offset 0L, size is determined by size_struct):
 *   Signature string: SIGNATURE SIG_DAT
 *
 *   Sizes:            Sizes size_struct;
 *
 *	 Root block:	   DISKOFFSET root_block;
 *	 Free block chain: DISKOFFSET free_block;
 *
 *
 * Data blocks
 *   Block control: BlockControl blockdata;
 *   Data items:    The data records are variable length and are
 *                  just packed in one after another. The user-
 *                  supplied function get_rec_size() is employed
 *                  to step through the records.
 */

/* Signature strings */
#define SIGNATURE "B-Tree File/V1.0/"

/* SIG_DATA and SIG_INDEX must be same length */
#define SIG_DATA  "Dat"
#define SIG_INDEX "Ndx"

typedef struct sSize {
	/* Block sizes and share/split breakpoints */
    BLKSIZE block_size; /* size of blocks */
    BLKSIZE split;      /* split blocks when this # bytes used */
    BLKSIZE merge;      /* merge blocks when this # bytes used */
    unsigned levels;    /* # of index levels (.ndx files only) */
} Sizes;

/* Control structures for disk blocks */
typedef enum eBlockState { sAvail, sNode, sLeaf } BlockState;
typedef struct sBlockControl {
    size_t     bfree;   /* Free data area begins here */
    BlockState blockstate;
    DISKOFFSET parent;  /* parent of this block */
    DISKOFFSET next;    /* used when block belongs */
                        /* ... to a free chain */
} BlockControl;

/* Control structures for I/O buffer blocks */
typedef enum eBufState { sFree, sClean, sDirty } BufState;
typedef struct sBufferList {
    BufState state;
    DISKOFFSET offset;
    char *buffer;
    struct sBufferList *next;
} BufferList;

/*
 * Master control structure for access to a pair of database
 * files. See code in bt_open.c for example initialization.
 */
#define NDX 0
#define DAT 1
typedef struct sBtree {
    /* Files, file names, and buffers */
    struct {
        char filename[MAX_PATH_AND_FILE];
        int modified;
        FILE *file;
        BufferList *bufferlist;
        Sizes sizes;
        DISKOFFSET root_block;
        DISKOFFSET free_block;
    } fdata[2];

    /*--- Data objects and procedures to manipulate them ---*/

    /* get size of a key */
    unsigned (*getkeysize)(void *);

    /* size of key in a record */
    unsigned (*getkeyNrecsize)(void *);

    /* get size of a record */
    unsigned (*getrecsize)(void *);

    /* compare key to key */
    int (*key2keycmp)(void *, void *);

    /* compare key to record */
    int (*key2reccmp)(void *, void *);

    /* copy a key from a record */
    void (*rec2keycpy)(void *, void *);

    /* Miscellaneous */
    int error_code;     /* Index into ErrorCode[] */
    int duplicatesOK;   /* Are duplicate keys ok? */

    /* used by insert & delete */
    DISKOFFSET CurrentDataBlock;
    void *SearchKey;
    void *FoundRec;
} Btree;
/* some defines to simplify the code */
#define GETKEYSIZE     (*(bt->getkeysize))
#define GETKEYNRECSIZE (*(bt->getkeyNrecsize))
#define GETRECSIZE     (*(bt->getrecsize))
#define KEY2KEYCMP     (*(bt->key2keycmp))
#define KEY2RECCMP     (*(bt->key2reccmp))
#define REC2KEYCPY     (*(bt->rec2keycpy))
#define FDATA(x,y)     (bt->fdata[x].y)
#define SIZES(x,y)     (bt->fdata[x].sizes.y)

/* error messages */
extern char *ErrorText[]; /* defined in bt_disk.c */

/* Prototypes */

/* action function used during tree walk */
typedef int (*DoFunc) (Btree *bt, void *rec);


/* bt_new.c */
int  bt_new(Btree *bt, void *maxkey, void *maxrec);

/* bt_disk.c */
char *bt_getblock4read(Btree *bt, int f, DISKOFFSET dof);
char *bt_getblock4write(Btree *bt, int f, DISKOFFSET dof);
BufferList *bt_getnew4write(Btree *bt, int f);
int  bt_flush(Btree *bt);
int  bt_bufinit(Btree *bt, int f);
void bt_bufrelease(Btree *bt, int f);
int  bt_releaseblock(Btree *bt, int f, DISKOFFSET dof);

/* bt_open.c */
int  bt_open(Btree *bt);
int  bt_close(Btree *bt);

/* bt_data.c */
int  bt_add(Btree *bt, void *rec, void *key);
int  bt_delete(Btree *bt, void *key);
int  bt_find(Btree *bt, void *rec, void *key);
int  bt_walk(Btree *bt, DoFunc df);
#endif
