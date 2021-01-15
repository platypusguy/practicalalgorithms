/*--- bt_open.c --------------------------- Listing 6-5 ---------
 * Open a B-Tree dataset
 *
 * See user.c (Listing 6-9) for a test driver.
 *-------------------------------------------------------------*/

#include "bt_hdr.h"

#if defined(HELP1)
#define SHOW(x) x
#else
#define SHOW(x)
#endif

/* Open files, allocate buffers */
int bt_open(Btree *bt)
{
    int i, ret;
    char buffer[512], *s;

    /* Check values, allocate buffers */
    ret = TREE_OK;
    for (i=0; i < 2; i++) {
        FDATA(i,file) = fopen(FDATA(i,filename), "r+b");
        if (FDATA(i,file) == NULL) {
            bt -> error_code = 6;
            ret = TREE_FAIL;
            break;
        }
        FDATA(i, modified) = 0;
        SHOW(printf("bt_open file: %s, file: %Fp\n",
                        FDATA(i, filename), FDATA(i, file));)

        if (fread(buffer, 512, 1, FDATA(i,file)) != 1) {
            bt -> error_code = 7;
            ret = TREE_FAIL;
            break;
        }

        s = buffer;
        if (strcmp(s, i == NDX ?
                   SIGNATURE SIG_INDEX : SIGNATURE SIG_DATA)) {
            bt -> error_code = 8;
            ret = TREE_FAIL;
            break;
        }

        s += strlen(s) + 1;
        bt -> fdata[i].sizes = *((Sizes *) s);
        s += sizeof(Sizes);
        bt -> fdata[i].root_block = *((DISKOFFSET *) s);
        s += sizeof(DISKOFFSET);
        bt -> fdata[i].free_block = *((DISKOFFSET *) s);

        ret = bt_bufinit(bt, i);
        if (ret)
            break;
    }

    return ret;
}

/* Flush buffers and shut down */
int bt_close(Btree *bt)
{
    int i, ret;

    ret = bt_flush(bt);
    for (i=0; i<2; i++) {
        if (FDATA(i,file)) {
            if (fclose(FDATA(i,file))) {
                bt -> error_code = 9;
                ret = TREE_FAIL;
            }
            FDATA(i,file) = NULL;
        }
        bt_bufrelease(bt, i);
    }

    return ret;
}
