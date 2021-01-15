/*--- bt_data.c --------------------------- Listing 6-6 ---------
 * Add, remove, and find records
 *
 * See test driver is in user.c  (Listing 6-9)
 *-------------------------------------------------------------*/

#include "bt_hdr.h"

#if defined(HELP1)
#define SHOW(x) x
#else
#define SHOW(x)
#endif

/*
 * Support routines for find, search, walk
 */

int FindFunc(Btree *bt, void *rec) {
    int compare = KEY2RECCMP(bt->SearchKey, rec);
    if (compare > 0)
        return TREE_OK;
    else if (compare == 0) {
        if (bt->FoundRec)
            /*
             * find calls us with FoundRec = a
             * data area where the round record is
             * to be copied
             */
            memcpy(bt->FoundRec, rec, GETRECSIZE(rec));
        else
            /*
             * delete and add call with FoundRec == NULL. When
             * we find the record, we set FoundRec non-NULL
             * to indicate our success.
             */
            bt->FoundRec = rec; /* irrelevant, non-NULL value */
    }
    else
        bt->FoundRec = NULL;
    return TREE_FAIL;
}

int LocateFunc(Btree *bt, void *rec) {
    int compare = KEY2RECCMP(bt->SearchKey, rec);
    if (compare > 0)
        return TREE_OK;
    else if (compare == 0)
        bt->FoundRec = rec;
    else
        bt->FoundRec = NULL;
    return TREE_FAIL;
}

/*
 * Walk the tree, calling user function df for each data record
 */
#define SELECTIVE  0  /* modes for bt_walk_down */
#define EXHAUSTIVE 1
int bt_walk_down(Btree *bt,         /* the tree                */
                 int f,             /* NDX or DAT?             */
                 DISKOFFSET block,  /* the block               */
                 DoFunc df,         /* an action function      */
                 int level,         /* depth of search         */
                 int mode)          /* SELECTIVE or EXHAUSTIVE */
{
    char *s;
    BlockControl *bc;
    unsigned offset, keysize;
    int retval;
    DISKOFFSET datablock;

    s = bt_getblock4read(bt, f, block);
    if (!s) {
        bt->error_code = 7;
        return TREE_FAIL;
    }

    bc = (BlockControl *) s;
    retval = TREE_OK;

    if (f == NDX) {
        offset = sizeof(BlockControl);
        while (offset < bc->bfree) {
            keysize = GETKEYSIZE(s + offset);
            datablock = *((DISKOFFSET *) (s + offset + keysize));
            if (mode == SELECTIVE) {
                if (KEY2KEYCMP(bt->SearchKey, s+offset) > 0) {
                    offset += keysize + sizeof(DISKOFFSET);
                    continue;
                }
            }
            retval =  bt_walk_down(bt,
                      bc->blockstate == sLeaf ? DAT : NDX,
                      datablock, df, level + 1, mode);
            if (retval)
                break;

            /* recursive calls may invalidate our buffer */
            s = bt_getblock4read(bt, f, block);
            bc = (BlockControl *) s;
            offset += keysize + sizeof(DISKOFFSET);
        }
    }
    else { /* f == DAT */
        bt->CurrentDataBlock = block;
        offset = sizeof(BlockControl);
        while (offset < bc->bfree) {
            retval = df(bt, s + offset); /* do user's bidding */
            if (retval)
                break;
            offset += GETRECSIZE(s + offset);
        }
    }

    return retval;
}

/*
 * Systematically walk the tree
 */
int bt_walk(Btree *bt, DoFunc df) {
    bt->CurrentDataBlock = 0L;
    return bt_walk_down(
        bt, NDX, FDATA(NDX, root_block), df, 0, EXHAUSTIVE);
}

/*
 * Search for a record
 */
int bt_find(Btree *bt, void *rec, void *key) {
    int retval;

    bt->SearchKey = key;
    bt->FoundRec = rec;
    retval = bt_walk_down(
        bt, NDX, FDATA(NDX, root_block), FindFunc, 0, SELECTIVE);
    if (retval && bt->FoundRec)
        return TREE_OK; /* found it */
    else
        return TREE_FAIL;
}

/*
 * update index blocks
 */
DISKOFFSET bt_fix_index (Btree *bt,
                         void *replacekey,
                         DISKOFFSET fixblock,
                         DISKOFFSET oldblock,
                         void *newkey,
                         DISKOFFSET newblock)
{
    /*
     * This is a very heavily used routine that performs
     * major updates to index blocks. It can insert a new
     * index pointer, replace a pointer, or delete a pointer.
     * It will split the block if it overflows.
     *
     * If replacekey == NULL: (inserting a new key)
     *   In index block fixblock, find the key/DISKOFFSET pair
     *   that has oldblock for its DISKOFFSET. Then,
     *    1) update this key/DOF pair so that DOF is newblock
     *    2) insert newkey/oldblock as a new pair just in front
     *    3) split index block if now too big
     *
     * If replacekey != NULL (replacing an existing key)
     *   In index block fixblock, find the key/DISKOFFSET pair
     *   that has oldblock for its DISKOFFSET This pair will
     *   actually be replacekey/oldblock. The value of replacekey
     *   is used both as a flag and to allow the routine to see
     *   quickly if it will need to split the block. Then,
     *    1) update this key/DOF pair so that key is newkey (**)
     *    2) split index block if now too big
     *    3) promulgate key replacement upwards if key/DOF pair
     *       is the last key/DOF pair in the block
     *
     *   (**) If newkey is NULL, then the key/DISKOFFSET pair is
     *        simply deleted and not replaced.
     *
     * Note that we do not actually do these operations in this
     * order. We proceed by first determining if we need to split
     * the index block. If so, we allocate a new block and then
     * recursively call ourself to place the new entry into the
     * next level up. Only if this succeeds do we actually commit
     * a change to the database.
     *
     * We return the DISKOFFSET of oldblock and newblock's new
     * parent.
     */

    char *pblock,           /* current index block (fixblock) */
         *pblock1 = NULL;   /* added index block, if we need one */
    char *keycopy;
    BlockControl *bc, *bc1 = NULL;
    DISKOFFSET newindexblock = 0, newparent;
    BufferList *bl = NULL;
    unsigned keysize, newsize, previous;
    unsigned offset, lastoffset, replacesize;
    int delta;
    int bt_merger(Btree *, int, DISKOFFSET);

    SHOW(printf("bt_fix_index/a: %Fp, %d, %ld\n",
                                            bt, NDX, fixblock);)
    pblock = bt_getblock4write(bt, NDX, fixblock);
    if (!pblock)
        return 0L;
    bc = (BlockControl *) pblock;
    newsize = -sizeof(DISKOFFSET);
    if (newkey)
        newsize = GETKEYSIZE(newkey);
    replacesize = 0;
    if (replacekey)
        replacesize = GETKEYSIZE(replacekey) + sizeof(DISKOFFSET);
    delta = (int) newsize + (int) sizeof(DISKOFFSET) -
                                            (int) replacesize;
    if (bc->bfree + delta > SIZES(NDX,split)) {
        int oldblockseen = 0;

        if (bc->parent == 0) {
            /*
             * We're trying to split the root block!
             * Complain & abort.
             */
             bt->error_code = 17;
             return 0L;
        }

        /* allocate a new index block */
        bl = bt_getnew4write(bt,NDX);
        if (!bl)
            return 0L;
        pblock1 = bl -> buffer;
        newindexblock = bl -> offset;
        bc1 = (BlockControl *) pblock1;
        bc1 -> blockstate = sNode;

        /* locate the break point */
        offset = sizeof(BlockControl);
        previous = 0;
        while (offset < bc->bfree/2) {
            previous = offset;
            keysize = GETKEYSIZE(pblock + offset);
            if (*((DISKOFFSET *)(pblock + offset + keysize)) ==
                                                    oldblock)
                oldblockseen = 1;
            offset += keysize + sizeof(DISKOFFSET);
        }
        if (previous == 0) {
            /*
             * Major problem. there is one key that it so large
             * we can't divide the block. This should never
             * happen, as keys should be small relative to the
             * size of the block. Complain and abort. The index
             * may now be corrupt.
             */
            bt->error_code=18;
            return 0L;
        }
        keysize = GETKEYSIZE(pblock + previous);
        keycopy = malloc(keysize);
        memcpy(keycopy, pblock + previous, keysize);

        /* make sure we can alter our parent */
        newparent =
            bt_fix_index(bt,
                         NULL,
                         bc->parent,
                         fixblock,
                         keycopy,
                         newindexblock);
        free (keycopy);
        if (!newparent)
            return 0L;

        /* refresh our pointers */
        SHOW(printf("bt_fix_index/b: %Fp, %d, %ld\n",
                                    bt, NDX, fixblock);)
        pblock  = bt_getblock4write(bt,NDX,fixblock);
        SHOW(printf("bt_fix_index/c: %Fp, %d, %ld\n",
                                    bt, NDX, newindexblock);)
        pblock1 = bt_getblock4write(bt,NDX,newindexblock);
        if (!pblock || !pblock1)
            return 0L;
        bc  = (BlockControl *) pblock;
        bc1 = (BlockControl *) pblock1;
        bc -> parent = bc1 -> parent = newparent;

        /* divide'em up */
        memcpy(pblock1 + bc1->bfree, pblock + offset,
                                        bc->bfree - offset);
        bc1 -> bfree += bc -> bfree - offset;
        bc  -> bfree = offset;

        /* common parent */
        bc1 -> parent = bc -> parent = newparent;

        /* same level, same use */
        bc1 -> blockstate = bc -> blockstate;

        /* tell children of new index block */
        /* ... about their new parent */
        offset = sizeof(BlockControl);
        while(offset < bc1 -> bfree) {
            char *pblock2;
            BlockControl *bc2;
            DISKOFFSET goal;

            keysize = GETKEYSIZE(pblock1 + offset);
            goal = *((DISKOFFSET *)(pblock1 + offset + keysize));
            SHOW(printf("bt_fix_index/d: %Fp, %d, %ld\n",
                 bt, bc1->blockstate == sLeaf ? DAT : NDX,
                 goal);)
            pblock2 = bt_getblock4write(bt,
                      bc1->blockstate == sLeaf ? DAT : NDX,
                      goal);
            if (!pblock2)
                return TREE_FAIL;
            bc2 = (BlockControl *) pblock2;
            bc2 -> parent = newindexblock;

            /* freshen our pointers */
            SHOW(printf("bt_fix_index/e: %Fp, %d, %ld\n",
                 bt, NDX, newindexblock);)
            pblock1 = bt_getblock4write(bt, NDX, newindexblock);
            bc1 = (BlockControl *) pblock1;
            offset += keysize + sizeof(DISKOFFSET);
        }

        /* transfer our attention to new index block */
        if (!oldblockseen)
            fixblock = newindexblock;
    }

    /*
     * Now, after many struggles, it is safe to insert our new
     * pointer into the block pointed to by pblock. We start by
     * locating the key/DISKOFFSET pair that has oldblock for
     * its DISKOFFSET...
     */
    pblock =
        bt_getblock4write(bt, NDX, fixblock); /* fresh pointer */
    if (!pblock)
        return 0L;
    bc = (BlockControl *) pblock;

    offset = sizeof(BlockControl);
    lastoffset = 0;
    keysize = GETKEYSIZE(pblock + offset);
    while (offset < bc ->bfree) {
        keysize = GETKEYSIZE(pblock + offset);
        if (*((DISKOFFSET *)(pblock + offset + keysize)) ==
                                                      oldblock)
            break;

        lastoffset = offset;
        offset += keysize + sizeof(DISKOFFSET);
    }
    if (*((DISKOFFSET *)(pblock + offset + keysize)) !=
                                                      oldblock) {
        /* something is very wrong */
        bt -> error_code = 19;
        return 0L;
    }

    if (replacekey) { /* change keys but not DISKOFFSETs */
        DISKOFFSET saveblock;

        /* move remaining data up */
        memmove(pblock + offset + replacesize + delta,
                pblock + offset + replacesize,
                bc->bfree - offset - replacesize);

        /* insert newkey/oldblock */
        if (newkey) { /* if NULL, we are actually deleting */
            memmove(pblock + offset, newkey, newsize);
            *((DISKOFFSET *)(pblock + offset + newsize)) =
                                                      oldblock;
        }
        bc->bfree += delta;

        /*
         * Is this the last pointer in the block? If so, must
         * update our parent so that it knows about the change
         */
        saveblock = fixblock;
        if (offset + newsize + sizeof(DISKOFFSET) >=
                                                     bc->bfree) {
            int delete_newkey = 0;
            if (!newkey && lastoffset) {
                /*
                 * We are deleting the last key of a non-empty
                 * block. We must promote a new last key.
                 */
                newkey =
                    malloc(GETKEYNRECSIZE(pblock + lastoffset));
                if (!newkey) {
                        bt->error_code=16;
                        return 0L;
                }
                REC2KEYCPY(pblock + lastoffset, newkey);
                delete_newkey = 1;
            }
            fixblock = bt_fix_index(bt, replacekey, bc->parent,
                                    fixblock, newkey, 0L);

            if (delete_newkey)
                free(newkey);

            /* is the block empty? if so, delete it. */
            if (bc->bfree == sizeof(BlockControl)) {
                if (bt_releaseblock(bt, NDX, saveblock))
                    fixblock = 0L;
            }
            else { /* should we merge the block? */
                if (bc->bfree < SIZES(NDX, merge))
                    if (bt_merger(bt, NDX, saveblock))
                        fixblock = 0L;
            }
        }

        return fixblock;
    }
    else {
        /* update existing key/DISKOFFSET to point to newblock */
        *((DISKOFFSET *)(pblock + offset + keysize)) = newblock;

        /* insert newkey/oldblock */
        memmove(pblock + offset + newsize + sizeof(DISKOFFSET),
                pblock + offset,
                bc->bfree - offset);
        memmove(pblock + offset, newkey, newsize);
        *((DISKOFFSET *)(pblock + offset + newsize)) = oldblock;
        bc -> bfree += newsize + sizeof(DISKOFFSET);

        return fixblock;
    }
}

/*
 * Add a new record
 *
 * Our general add strategy is:
 *
 *  1) Locate the spot for the insertion
 *  2) Split the data block if necessary (bt_split)
 *  3) Split index blocks if necessary (bt_fix_index)
 *  4) Re-locate the spot for the insertion
 *  5) Insert can now proceed without further splits
 *
 * Note that we actually commit step 3 to the database
 * before step 2--this allows us to abort if step 3
 * fails.
 */

/* split bottom data block in half */
int bt_split(Btree *bt)
{
    char *pblock, *pblock1, *keycopy;
    BufferList *bl;
    DISKOFFSET datablock, datablock1, newparent;
    BlockControl *bc, *bc1;
    unsigned previous, offset;

    datablock = bt->CurrentDataBlock;
    SHOW(printf("bt_split/a: %Fp, %d, %ld\n",
                            bt, DAT, datablock);)
    pblock = bt_getblock4write(bt, DAT, datablock);
    if (!pblock) {
        bt->error_code = 7;
        return TREE_FAIL;
    }

    bc = (BlockControl *) pblock;
    SHOW(printf("bt_split: parent of %ld is %ld\n",
                            datablock, bc->parent);)

    offset = sizeof(BlockControl);
    previous= 0;
    while (offset < bc->bfree/2) {
        previous = offset;
        offset += GETRECSIZE(pblock + offset);
    }

    if (previous == 0) { /* major problem - can't divide block */
        bt -> error_code = 15;
        return TREE_FAIL;
    }

    /* get a copy of the break record's key */
    keycopy = malloc(GETKEYNRECSIZE(pblock + previous));
    if (!keycopy) {
        bt -> error_code = 16;
        return TREE_FAIL;
    }
    REC2KEYCPY(pblock + previous, keycopy);

    /* get a new block */
    bl = bt_getnew4write(bt, DAT);
    if (!bl) {
        bt->error_code = 7;
        return TREE_FAIL;
    }

    pblock1 = bl -> buffer;
    datablock1 = bl -> offset;
    bc1 = (BlockControl *) pblock1;
    bc1 -> blockstate = sLeaf;

    /*
     * do insert into upper level blocks first. If this fails,
     * we have not committed any changes to the database and we
     * can just abort.
     */
    SHOW(printf("calling bt_fix_index(%Fp,%s,%ld,%ld,%ld\n",
                    bt,keycopy,bc->parent,datablock,datablock1);)
    newparent =
        bt_fix_index(bt, NULL, bc->parent, datablock,
                     keycopy, datablock1);
    free(keycopy);
    if (!newparent)
        return TREE_FAIL;

    /* Now divide the data */
    SHOW(printf("bt_split/b: %Fp, %d, %ld\n",
                                bt, DAT, datablock);)
    pblock  = bt_getblock4write(bt, DAT, datablock);

    SHOW(printf("bt_split/c: %Fp, %d, %ld\n",
                                bt, DAT, datablock1);)
    pblock1 = bt_getblock4write(bt, DAT, datablock1);

    bc  = (BlockControl *) pblock;
    bc1 = (BlockControl *) pblock1;
    memcpy(pblock1 + bc1->bfree, pblock + offset,
                                bc->bfree - offset);
    bc1 -> bfree += bc -> bfree - offset;
    bc  -> bfree = offset;
    bc1 -> parent = bc -> parent = newparent; /* common parent */

    return TREE_OK;
}

int bt_add_record(Btree *bt, void *rec, void *key)
{
    char *s;
    BlockControl *bc;
    unsigned offset, datasize;
    DISKOFFSET datablock;
    int retval;

  top:
    datablock = bt->CurrentDataBlock;
    SHOW(printf("bt_add_record: %Fp, %d, %ld\n",
                                bt, DAT, datablock);)
    s = bt_getblock4write(bt, DAT, datablock);
    if (!s) {
        bt->error_code = 7;
        return TREE_FAIL;
    }

    bc = (BlockControl *) s;
    datasize = GETRECSIZE(rec);

    if (bc->bfree + datasize > SIZES(DAT,split)) {
        if (bt_split(bt))
            return TREE_FAIL; /* couldn't do it */
        else {
            /* re-locate position for block to insert */
            retval = bt_walk_down(
                bt, NDX, FDATA(NDX, root_block),
                LocateFunc, 0, SELECTIVE);
            if (!retval)
                return retval;
            goto top; /* try again */
        }
    }

    /* looks good. make a space */
    offset = sizeof(BlockControl);
    while (offset < bc->bfree) {
        if (KEY2RECCMP(key, s + offset) < 0)
            break; /* goes before this record */
        offset += GETRECSIZE(s + offset);
    }
    /* make our moves */
    memmove(s + offset + datasize, s + offset,
                            bc->bfree - offset);
    memmove(s + offset, rec, datasize);
    bc->bfree += datasize;
    return TREE_OK;
}

int bt_add(Btree *bt, void *rec, void *key)
{
    int retval = TREE_OK;

    bt->SearchKey = key;
    bt->FoundRec = NULL;
    retval = bt_walk_down(
        bt, NDX, FDATA(NDX, root_block),
        LocateFunc, 0, SELECTIVE);
    if (!retval) {
        bt->error_code = 14;
        return TREE_FAIL; /* should never occur */
    }

    /*
     * Now, if FoundRec == NULL, then the current data record is
     * the right place for the new record and the new record
     * does not exist.
     *
     * If FoundRec != NULL, then FoundRec points to a data record
     * with the same key as the record to be inserted
     */

    retval = TREE_OK;
    if (bt->FoundRec) {
        if (!bt->duplicatesOK)
            retval = TREE_FAIL;
    }

    if (!retval)
        retval = bt_add_record(bt, rec, key);

    return retval;
}

/*
 * Delete a record
 *
 * Our general delete strategy starts in
 * bt_delete_record() and is:
 *
 *  1) Locate & delete offending record
 *
 *  2) If record was the rightmost record in its block,
 *     update key in parent index block(s). Note that
 *     this process involves removing an old key and
 *     inserting a new one: if the new key is larger
 *     than the old one, the block could potentially
 *     exceed the split limit. This could in turn split
 *     the parent, etc. Alternatively, the parent
 *     could need to merged. All of this is handled by
 *     bt_fix_index().
 *
 *  3) If datablock that held the deleted record is now
 *     smaller than merge, locate and examine the
 *     left and then the right sisters of this block.
 *     If the current block can be combined with
 *     either, then we will merge them. To merge,
 *     we first shift the tree so that the target
 *     datablocks have the same index block as their
 *     parent, we combine the blocks, and then drop
 *     one entry from the parent index block.
 */

/*
 * Routines to merge blocks
 */
#define LEFT 0
#define RIGHT 1

/* find sister of child in parent at level==0 */
DISKOFFSET bt_find_sister(Btree *bt,
                           DISKOFFSET parent,
                           DISKOFFSET child,
                           int direction,
                           int level) {

    char *pblock;
    BlockControl *bc;
    unsigned offset, lastoffset, keysize;
    DISKOFFSET dof;

    pblock = bt_getblock4read(bt, NDX, parent);
    if (!pblock)
        return 0L;
    bc = (BlockControl *) pblock;

    /* find key/diskoffset with child for its diskoffset */
    lastoffset = 0;
    offset = sizeof(BlockControl);
    dof = 0;
    while (offset < bc -> bfree) {
        keysize = GETKEYSIZE(pblock + offset);
        dof = *((DISKOFFSET *)(pblock + offset + keysize));
        if (dof == child)
            break;
        lastoffset = offset;
        offset += keysize + sizeof(DISKOFFSET);
    }

    if (dof != child) {
        bt->error_code = 22;
        return 0L;
    }

    if (direction == LEFT) {
        if (lastoffset) { /* it is in this block */
            keysize = GETKEYSIZE(pblock + lastoffset);
            dof =
                *((DISKOFFSET *)(pblock + lastoffset + keysize));
            while (level > 0) { /* unwind the chain */
                level--;
                pblock = bt_getblock4read(bt, NDX, dof);
                if (!pblock)
                    return 0L;
                bc = (BlockControl *) pblock;
                offset = sizeof(BlockControl);

                /* find rightmost entry */
                while (offset < bc -> bfree) {
                    keysize = GETKEYSIZE(pblock + offset);
                    dof = *((DISKOFFSET *)(pblock + offset +
                                                      keysize));
                    offset += keysize + sizeof(DISKOFFSET);
                }
            }
            return dof;
        }
        else if (bc->parent)
            return bt_find_sister(bt, bc->parent, parent,
                                  LEFT, level + 1);
        else
            return 0L;
    }
    else { /* direction == RIGHT */
        keysize = GETKEYSIZE(pblock + offset);
        offset += keysize + sizeof(DISKOFFSET);

        if (offset < bc->bfree) { /* it is in this block */
            keysize = GETKEYSIZE(pblock + offset);
            dof = *((DISKOFFSET *)(pblock + offset + keysize));
            while (level > 0) { /* unwind the chain */
                level--;
                pblock = bt_getblock4read(bt, NDX, dof);
                if (!pblock)
                    return 0L;
                bc = (BlockControl *) pblock;
                offset = sizeof(BlockControl);
                keysize = GETKEYSIZE(pblock + offset);
                dof =
                    *((DISKOFFSET *)(pblock + offset + keysize));
            }
            return dof;
        }
        else
        if (bc->parent)
            return bt_find_sister(bt, bc->parent, parent,
                                  RIGHT, level + 1);
        else
            return 0L;
    }
}

/* merge left into right, if possible */
int bt_do_merge(Btree *bt,
                int f,  /* DAT or NDX */
                DISKOFFSET left,
                DISKOFFSET right) {
    char *pleft, *pright;
    BlockControl *bcleft, *bcright;
    void *oldkey;
    unsigned offset, recsize;

    pleft = bt_getblock4read(bt, f, left);
    if (!pleft)
        return TREE_FAIL;
    pright = bt_getblock4read(bt, f, right);
    if (!pright)
        return TREE_FAIL;

    bcleft  = (BlockControl *) pleft;
    bcright = (BlockControl *) pright;

    if (bcleft->bfree + bcright->bfree - sizeof(BlockControl) <
            SIZES(f, split)) { /* let's do it! */
        unsigned moving;

        /* get modifiable copies */
        pleft = bt_getblock4write(bt, f, left);
        if (!pleft)
            return TREE_FAIL;
        pright = bt_getblock4write(bt, f, right);
        if (!pright)
            return TREE_FAIL;

        bcleft  = (BlockControl *) pleft;
        bcright = (BlockControl *) pright;

        /* this many new bytes */
        moving = bcleft->bfree - sizeof(BlockControl);

        /* make room & copy */
        memmove(pright + sizeof(BlockControl) + moving,
                pright + sizeof(BlockControl),
                bcright->bfree);
        memmove(pright + sizeof(BlockControl),
                pleft  + sizeof(BlockControl),
                moving);
        bcright->bfree += moving;

        /* now, to discard the left block ... */

        /* ... we first find & copy the last record's key */
        offset = sizeof(BlockControl);
        if (f == NDX)
            recsize = GETKEYSIZE(pleft + offset) +
                                        sizeof(DISKOFFSET);
        else
            recsize = GETRECSIZE(pleft + offset);
        while (offset + recsize < bcleft->bfree) {
            offset += recsize;
            if (f == NDX)
                recsize = GETKEYSIZE(pleft + offset) +
                                        sizeof(DISKOFFSET);
            else
                recsize = GETRECSIZE(pleft + offset);
        }

        if (f == NDX) {
            unsigned keysize;

            keysize = GETKEYSIZE(pleft + offset);
            oldkey = malloc(keysize);
            if (!oldkey) {
                    bt->error_code = 16;
                    return TREE_FAIL;
            }
            memmove(oldkey, pleft + offset, keysize);
        }
        else {
            oldkey = malloc(GETKEYNRECSIZE(pleft + offset));
            if (!oldkey) {
                bt->error_code = 16;
                return TREE_FAIL;
            }
            REC2KEYCPY(pleft + offset, oldkey);
        }

        /* ... and then run up the tree, deleting oldkey */
        if (!bt_fix_index(bt, oldkey, bcleft->parent,
                          left, 0L, 0L))
            return TREE_FAIL;
        free (oldkey);

        /* ... and finally free the block */
        if (bt_releaseblock(bt, f, left))
            return TREE_FAIL;

        /* tell children about their new parent */
        if (f == NDX) {
            pright = bt_getblock4write(bt, f, right);
            if (!pright)
                return TREE_FAIL;

            bcright = (BlockControl *) pright;
            moving += sizeof(BlockControl);
            offset  = sizeof(BlockControl);
            while(offset < moving) {
                char *pblock2;
                BlockControl *bc2;
                DISKOFFSET goal;
                unsigned keysize;

                keysize = GETKEYSIZE(pright + offset);
                goal = *((DISKOFFSET *)(pright + offset +
                                                    keysize));
                SHOW(printf("bt_do_merge: %Fp, %d, %ld\n",
                       bt, bcright->blockstate == sLeaf ? DAT :
                       NDX, goal);)
                pblock2 = bt_getblock4write(bt,
                          bcright->blockstate == sLeaf ? DAT :
                          NDX, goal);
                if (!pblock2)
                    return TREE_FAIL;
                bc2 = (BlockControl *) pblock2;
                bc2 -> parent = right;

                /* freshen our pointers */
                pright = bt_getblock4write(bt, NDX, right);
                bcright = (BlockControl *) pright;
                offset += keysize + sizeof(DISKOFFSET);
            }
        }

        return TREE_OK;
    }
    else
        return TREE_FAIL; /* didn't fit */
}

/* main merge routine */
int bt_merger(Btree *bt, int f, DISKOFFSET block)
{
    char *pblock;
    BlockControl *bc;
    DISKOFFSET sister;

    pblock = bt_getblock4read(bt, f, block);
    if (!pblock)
        return TREE_FAIL;
    bc = (BlockControl *) pblock;

    if (bc->bfree < SIZES(f,merge)) { /* try to do a merge! */
        /* look left */
        sister = bt_find_sister(bt, bc->parent, block, LEFT, 0);
        if (sister && !bt_do_merge(bt, f, sister, block))
            return TREE_OK;

        /* look right */
        sister = bt_find_sister(bt, bc->parent, block, RIGHT, 0);
        if (sister && !bt_do_merge(bt, f, block, sister))
            return TREE_OK;
    }

    if (!bt->error_code)
        return TREE_OK;
    else
        return TREE_FAIL;
}

int bt_delete_record(Btree *bt)
{
    DISKOFFSET block;
    char *pblock;
    BlockControl *bc;
    unsigned offset, recsize, lastoffset;
    int compare, retval = TREE_OK;

    /* target record should be in this block */
    block = bt->CurrentDataBlock;
    pblock = bt_getblock4write(bt, DAT, block);
    if (!pblock)
        return TREE_FAIL;
    bc = (BlockControl *) pblock;

    /* find the record */
    offset = sizeof(BlockControl);
    lastoffset = 0;
    while (offset < bc->bfree) {
        recsize = GETRECSIZE(pblock + offset);
        compare = KEY2RECCMP(bt->SearchKey, pblock + offset);
        if (compare < 0) {
            bt -> error_code = 20;
            return TREE_FAIL; /* should not happen */
        }
        else if (compare == 0)
            break;
        lastoffset = offset;
        offset += recsize;
    }

    if (offset >= bc->bfree) {
        bt -> error_code = 20;
        return TREE_FAIL;
    }

    /* expunge the record */
    bc->bfree -= recsize;
    if (bc->bfree > offset) { /* not the last record */
        memmove(pblock + offset,
                pblock + offset + recsize,
                bc->bfree - offset);
        retval = bt_merger(bt, DAT, block);
    }
    else {
        /* deleting the last record in a block. must promulgate
           the key of the new last record upward.
         */
        void *oldkey, *newkey = NULL;
        oldkey = malloc(GETKEYNRECSIZE(pblock + offset));
        if (!oldkey) { bt->error_code=16; return TREE_FAIL; }
        REC2KEYCPY(pblock + offset, oldkey);

        /* is there a new last record? */
        if (lastoffset) {
            newkey = malloc(GETKEYNRECSIZE(pblock + lastoffset));
            if (!newkey) {
                bt->error_code = 16;
                return TREE_FAIL;
            }
            REC2KEYCPY(pblock + lastoffset, newkey);
        }

        /* run up the tree, replacing oldkey with newkey */
        if (!bt_fix_index(bt, oldkey, bc->parent, block,
                          newkey, 0L))
            retval = TREE_FAIL;

        if (!retval) {
            if (lastoffset) /* merge if needed */
                retval = bt_merger(bt, DAT, block);

            else /* this block is empty */
                retval = bt_releaseblock(bt, DAT, block);
        }

        free(oldkey);
        if (newkey)
            free(newkey);
    }

    return retval;
}

int bt_delete(Btree *bt, void *key)
{
    int retval;

    bt->SearchKey = key;
    bt->FoundRec = NULL;
    retval = bt_walk_down(
        bt, NDX, FDATA(NDX, root_block), FindFunc, 0, SELECTIVE);
    if (retval && bt->FoundRec)
        return bt_delete_record(bt);
    else
        return TREE_FAIL;
}

