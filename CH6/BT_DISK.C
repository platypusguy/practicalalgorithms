/*--- bt_disk.c --------------------------- Listing 6-7 ---------
 * Disk-access routines and array of error messages
 *
 *-------------------------------------------------------------*/

#include "bt_hdr.h"

#if defined(HELP1)
#define SHOW(x) x
#else
#define SHOW(x)
#endif

char *ErrorText[] = {
  /*  0 */ "Not an error",
  /*  1 */ "bt_new: Could not create new data file",
  /*  2 */ "bt_new: Bad block_size value",
  /*  3 */ "bt_new: Bad split or merge values",
  /*  4 */ "bt_new: Could not write to new data file",
  /*  5 */ "bt_disk: Couldn't allocate buffers",
  /*  6 */ "bt_open: Couldn't open data file",
  /*  7 */ "bt_disk: Couldn't read data file",
  /*  8 */ "bt_open: Signature string missing",
  /*  9 */ "bt_close: Couldn't close file",
  /* 10 */ "bt_disk: Couldn't read/write file",
  /* 11 */ "bt_add: Attempted to add duplicate key",
  /* 12 */ "bt_split: Attempted to split block w/ NULL parent",
  /* 13 */ "bt_split: Couldn't find child in parent",
  /* 14 */ "bt_data: Locate failed",
  /* 15 */ "bt_data: couldn't split a block--record too large",
  /* 16 */ "bt: out of memory",
  /* 17 */ "bt_fix_index: trying to split root index block!",
  /* 18 */ "bt_fix_index: couldn't split a block--key too large",
  /* 19 */ "bt_fix_index: lost oldblock--critical error",
  /* 20 */ "bt_delete_record: couldn't find target record",
  /* 21 */ "bt_replacekey: couldn't find target key",
  /* 22 */ "bt_find_sister: couldn't find target key"
};

static int bt_blwrite(Btree *bt, int f, BufferList *bl)
{
    int ret = TREE_OK;

    if (fseek(FDATA(f,file), bl->offset, SEEK_SET))
        ret = TREE_FAIL;
    if (!ret &&
        fwrite(bl->buffer,
        SIZES(f,block_size), 1, FDATA(f,file)) != 1)
          ret = TREE_FAIL;

    if (ret) {
        bt -> error_code = 10;
        SHOW(printf("failing in " __FILE__
                    " at line %d\n", __LINE__);)
    }
    else
        bl->state = sClean;

    return ret;
}

static int bt_blread(Btree *bt, int f, BufferList *bl)
{
    int ret = TREE_OK;

    SHOW(printf("bt_blread: bt = %Fp, f = %d, bl = %Fp\n",
                        bt, f, bl);)
    SHOW(printf("filename: %s, file: %Fp\n",
                        FDATA(f, filename), FDATA(f, file));)
    SHOW(printf("buffer state=%d, offset=%lx,"
                "buffer=%Fp, next=%Fp\n",
                        bl->state, bl->offset,
                        bl->buffer, bl->next);)
    SHOW(printf("seeking to %ld\n", bl-> offset);)
    SHOW(fflush(stdout);)
    if (fseek(FDATA(f,file), bl->offset, SEEK_SET)) {
        ret = TREE_FAIL;
        SHOW(printf("failing in " __FILE__
                    " at line %d\n", __LINE__);)
    }
    SHOW (else printf(" now at %ld\n", ftell(FDATA(f, file)));)


    if (!ret &&
        fread(bl->buffer,
        SIZES(f,block_size), 1, FDATA(f,file)) != 1) {
          ret = TREE_FAIL;
        SHOW(printf("failing in " __FILE__
                    " at line %d\n", __LINE__);)
    }

    if (ret) {
        bl -> state = sFree;
        bt -> error_code = 10;
        SHOW(printf("failing in " __FILE__
                    " at line %d\n", __LINE__);)
    }
    else
        bl->state = sClean;

    return ret;
}

/* Load a disk block into a free buffer */
static BufferList *bt_getbuf(Btree *bt, int f, DISKOFFSET dof)
{
    BufferList *bl, *free_b, *free_p, *parent, *gparent;

    /* Already loaded? */
    free_b = free_p = parent = gparent = NULL;
    for (bl = FDATA(f,bufferlist); bl; ) {
        if (bl->state == sFree) {
            free_p = parent;
            free_b = bl;
        }
        else if (bl->offset == dof) {
            if (parent) { /* move to top of list */
                parent->next = bl -> next;
                bl -> next = FDATA(f,bufferlist);
                FDATA(f, bufferlist) = bl;
            }
            return bl;
        }

        gparent = parent;
        parent = bl;
        bl=bl->next;
    }

    if (!free_b) { /* Must free up an active buffer */
        /*
         * parent points to last buffer in list, and
         * gparent is the parent of this buffer. Because we
         * always move the most recently used buffer to the head
         * of the list, the last buffer is the least-recently
         * used buffer and gets tossed out.
         */
        free_b = parent;
        parent = gparent;
        if (free_b->state == sDirty && bt_blwrite(bt, f, free_b))
           free_b = NULL;
    }
    else
        parent = free_p;

    if (free_b) { /* A free buffer */
        SHOW(printf("bt_getbuf: Assigning bl at %Fp"
                    "to offset %ld\n", free_b, dof);)
        free_b -> offset = dof;
        if (bt_blread(bt, f, free_b)) {
            free_b -> state = sFree;
            free_b = NULL;
        }
        else {
            free_b -> state = sClean;
        }
    }

    if (free_b && parent) { /* Move new buffer to head of list */
        parent -> next = free_b -> next;
        free_b -> next = FDATA(f,bufferlist);
        FDATA(f,bufferlist) = free_b;
    }

    return free_b;
}

/* Get a block for read only */
char *bt_getblock4read(Btree *bt, int f, DISKOFFSET dof)
{
    BufferList *bl;
    SHOW(printf("bt_getblock4read(%Fp, %d, %ld)\n",bt,f,dof);)
    bl = bt_getbuf(bt, f, dof);
    if (bl)
        return bl -> buffer;
    else
        return NULL;
}

/* Get a block for read & write */
char *bt_getblock4write(Btree *bt, int f, DISKOFFSET dof)
{
    BufferList *bl;

    SHOW(printf("bt_getblock4write(%Fp, %d, %ld)\n",bt,f,dof);)
    bl = bt_getbuf(bt, f, dof);
    if (bl) {
        bl -> state = sDirty;
        return bl -> buffer;
    }
    else
        return NULL;
}

/* Get a new block for read & write */
BufferList *bt_getnew4write(Btree *bt, int f)
{
    BufferList *bl;
    BlockControl *bc;
    DISKOFFSET dof;

    /* first, are then any empty blocks already available? */
    if ((dof = FDATA(f, free_block)) != 0) {
        SHOW(printf("bt_getnew4write(%Fp, %d, %ld)\n",bt,f,dof);)
        bl = bt_getbuf(bt, f, dof);
        if (!bl)
            return bl;
        bc = (BlockControl *) bl->buffer;
        FDATA(f, free_block) = bc->next;
        FDATA(f, modified) = 1;
    }
    else {
        /* extend the file */
        if (fseek(FDATA(f,file), 0L, SEEK_END))
            return NULL;
        dof = ftell(FDATA(f, file));

        /* just write something */
        if (fwrite(FDATA(f,bufferlist),
                         SIZES(f,block_size),
                        1,
                        FDATA(f, file)) != 1) {
            bt -> error_code = 10;
            SHOW(printf("failing in " __FILE__
                        " at line %d\n", __LINE__);)
            return NULL;
        }
        SHOW(printf("bt_getnew4write #2(%Fp, %d, %ld)\n",
                        bt,f,dof);)
        bl = bt_getbuf(bt, f, dof);
        if (!bl)
            return bl;
    }

    /* initialize block */
    bc = (BlockControl *) bl->buffer;
    bc -> bfree = sizeof(BlockControl);
    bc -> blockstate = sAvail;
    bc -> parent = 0;
    bc -> next = 0;

    /* and ensure that all is written out */
    bt_flush(bt);

    bl -> state = sDirty;
    return bl;
}

int bt_flush(Btree *bt) {
    int i, ret;

    ret = TREE_OK;
    for (i=0; i<2; i++) {
        if (FDATA(i,file) && FDATA(i,bufferlist)) {
            char *p;
            DISKOFFSET *d;
            BufferList *bl;

            /* root block data changed? */
            if (FDATA(i, modified)) {

                /* get 1st block */
                p = bt_getblock4write(bt, i, 0L);
                p += sizeof(SIGNATURE) + sizeof(SIG_INDEX) - 1;
                p += sizeof(Sizes);
                d = (DISKOFFSET *) p;
                *d = FDATA(i, root_block);
                d++;
                *d = FDATA(i, free_block);
                FDATA(i, modified) = 0;
            }

            for (bl = FDATA(i,bufferlist); bl; bl = bl->next) {
                if (bl->state == sDirty)
                    ret = bt_blwrite(bt, i, bl);
            }

            fflush(FDATA(i, file));
        }
    }

    return ret;
}

int bt_bufinit(Btree *bt, int f)
{
    int i, cnt;
    BufferList dummy, *bl;

    if (FDATA(f,bufferlist) == NULL) {
        bl = &dummy;
        if (f == DAT)
            cnt = 5;
        else
            cnt = (int) SIZES(f,levels) * 3 / 2;
        if (cnt < 3)
            cnt = 3; /* minimum number */

        for (i = 0; i < cnt; i++) {
            bl -> next = malloc(sizeof(BufferList));
            if (bl -> next == NULL) {
                bt -> error_code = 5;
                return TREE_FAIL;
            }
            bl = bl -> next;
            bl -> buffer =
                malloc(SIZES(f,block_size));
            if (bl -> buffer == NULL) {
                bt -> error_code = 5;
                return TREE_FAIL;
            }
            bl -> state = sFree;
            bl -> offset = 0;
            bl -> next = NULL;
        }

        FDATA(f,bufferlist) = dummy.next;
    }
    return TREE_OK;
}

/* Release memory allocated to buffers */
void bt_bufrelease(Btree *bt, int f)
{
    BufferList *bl, *bl2;

    bl = FDATA(f,bufferlist);
    if (bl) {
        while(bl) {
            free(bl -> buffer);
            bl2 = bl -> next;
            free(bl);
            bl = bl2;
        }
        FDATA(f,bufferlist) = NULL;
    }
}

/* put a block on the free chain */
int  bt_releaseblock(Btree *bt, int f, DISKOFFSET dof)
{
    char *pblock;
    BlockControl *bc;

    pblock = bt_getblock4write(bt, f, dof);
    if (!pblock)
        return TREE_FAIL;
    bc = (BlockControl *) pblock;

    bc -> next = FDATA(f, free_block);
    bc -> blockstate = sAvail;
    FDATA(f, free_block) = dof;
    FDATA(f, modified) = 1;

    return TREE_OK;
}

