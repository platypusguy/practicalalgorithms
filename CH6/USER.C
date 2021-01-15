/*--- user.c ------------------------------ Listing 6-9 ---------
 *Sample support routines and a driver
 *-------------------------------------------------------------*/

#include "bt_hdr.h"
#include <ctype.h>
#include "user.h"

#define TESTSIZES /* use small block limits to force splits */

/*
 * A record with a maximal key
 */
UR MaxRec = { "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
              "Maximal", "record" };

/*
 * The support routines that the btree routines call
 */
unsigned UserGetKeySize(void *k) {
    return strlen( (char *) k ) + 1;
}

unsigned UserGetKeyNRecSize(void *k) {
    return strlen( ((UR *) k) -> key ) + 1;
}

unsigned UserGetRecSize(void *r) {
    return sizeof(UR);
}

int UserKey2KeyCmp(void *k1, void *k2) {
    return strcmp((char *) k1, (char *) k2);
}

int UserKey2RecCmp(void *k, void *r) {
    return strcmp((char *) k, ((UR *) r)->key);
}

void UserRec2KeyCpy(void *rec, void *key) {
	strcpy((char *) key, ((UR *) rec) -> key);
}

/*
 * Some tree display routines
 */
char *BlockStates[] = { "sAvail", "sNode", "sLeaf" };
BLKSIZE index_blksize;
FILE *outfile;

void show_basic(Btree *bt, int f) {
    DISKOFFSET length, blkcount;
    BLKSIZE blksize;

    fprintf(outfile, "File: %s\n", FDATA(f,filename));

    blksize = SIZES(f, block_size);
    fprintf(outfile, "  Block size: %d, split: %d, merge: %d\n",
           blksize,
           SIZES(f, split),
           SIZES(f, merge));

    if (f == NDX)
        fprintf(outfile, "  Index has %d levels\n",
                SIZES(f, levels));

    fseek(FDATA(f, file), 0L, SEEK_END);
    length = ftell(FDATA(f,file));
    blkcount = length/blksize;
    fprintf(outfile, "  File length %ld (%ld blocks)\n",
                length, blkcount);

    if (f == NDX)
        fprintf(outfile, "  Root index block at %ld"
                         " (block no. %ld)\n",
               FDATA(f, root_block),
               FDATA(f, root_block)/blksize);

    fprintf(outfile,
            "  First free block at %ld (block no. %ld)\n",
           FDATA(f, free_block),
           FDATA(f, free_block)/blksize);
}

/*
 * Given index block is at given level. Display contents
 * and then recursively examine the children.
 */
void prefix(int level) {
    for (; level; level--) fprintf(outfile, " * ");
}

void walk_down(Btree *bt, int f, DISKOFFSET block,
                int level, int mode) {
    char *s, *key;
    BlockControl *bc;
    unsigned offset, keysize, keycount = 0;
    DISKOFFSET *pDof;

    prefix(level);
    if (f == NDX)
        fprintf(outfile,
                "Level %d index block %ld at offset %ld.",
                level, block/index_blksize, block);
    else
        fprintf(outfile, "Data block %ld at offset %ld.",
                          block/SIZES(DAT, block_size), block);

    if (mode)
        fprintf(outfile, "\n");

    s = bt_getblock4read(bt, f, block);
    if (!s) {
        bt->error_code = 7;
        return;
    }

    bc = (BlockControl *) s;

    if (mode) {
        prefix(level);
        fprintf(outfile,
           "blockstate: %s, free data: "
           "%d, parent: %ld, next: %ld\n",
           BlockStates[bc->blockstate],
           bc->bfree,
           bc->parent,
           bc->next);
    }

    if (f == NDX) {
        offset = sizeof(BlockControl);
        while (offset < bc->bfree) {
            keysize = GETKEYSIZE(s + offset);
            key = s + offset;
            pDof = (DISKOFFSET *) (key + keysize);
            if (mode) {
                prefix(level);
                fprintf(outfile,
                       "Offset %d: key '%s', DISKOFFSET %ld\n",
                        offset, key, *pDof);
            }
            offset += keysize + sizeof(DISKOFFSET);
            keycount++;
        }
        if (!mode)
            fprintf(outfile, " %u entries\n", keycount);

        offset = sizeof(BlockControl);
        while (offset < bc->bfree) {
            keysize = GETKEYSIZE(s + offset);
            walk_down(bt,
                      bc->blockstate == sLeaf ? DAT : NDX,
                      *((DISKOFFSET *) (s + offset + keysize)),
                      level + 1, mode);

            /* recursive calls may invalidate our buffer */
            s = bt_getblock4read(bt, f, block);
            bc = (BlockControl *) s;
            offset += keysize + sizeof(DISKOFFSET);
        }
    }
    else { /* f == DAT */
        offset = sizeof(BlockControl);
        while (offset < bc->bfree) {
            keysize = GETRECSIZE(s + offset);
            if (mode) {
                prefix(level);
                fprintf(outfile,
                    "Offset %d: key '%s', name: %s, addr: %s\n",
                    offset,
                       ((UR *) (s + offset))->key,
                       ((UR *) (s + offset))->name,
                       ((UR *) (s + offset))->addr);
            }
            offset += keysize;
            keycount++;
        }
        if (!mode)
            fprintf(outfile, " %u entries\n", keycount);
    }
}

/*
 *display contents of data files
 *
 * mode = 0: just display number of pointers at each level
 * mode = 1: display all data
 */
void show_btree(Btree *bt, FILE *f, int mode) {
    outfile = f;
    if (mode) {
        show_basic(bt, NDX);
        show_basic(bt, DAT);
    }

    index_blksize = SIZES(NDX, block_size);
    walk_down(bt, NDX, FDATA(NDX, root_block), 0, mode);
}

/*
 * Block mapping routines
 *
 * Map is built up in map[] with these codes:
 *  C = control block (always block 0)
 *  N = index block
 *  D = data block
 *  . = lost block
 */
struct MapData {
    long length;
    unsigned blkcount;
    char *map;
} mapdata[2];

void show_btree_mapr(Btree *bt, DISKOFFSET iblock)
{
    char *pblock;
    BlockControl *bc;
    unsigned offset;
    DISKOFFSET dof;

    /* say we've been here */
    mapdata[NDX].map[iblock/SIZES(NDX,block_size)] = 'N';

    /* get the block */
    pblock = bt_getblock4read(bt, NDX, iblock);
    if (!pblock)
        return;
    bc = (BlockControl *)pblock;

    /* scan it */
    offset = sizeof(BlockControl);
    while (offset < bc->bfree) {
        offset += GETKEYSIZE(pblock + offset);
        dof = *((DISKOFFSET *) (pblock + offset));
        offset += sizeof(DISKOFFSET);
        if (bc->blockstate == sNode) {
            show_btree_mapr(bt, dof);
            pblock = bt_getblock4read(bt, NDX, iblock);
            if (!pblock)
                return;
            bc = (BlockControl *)pblock;
        }
        else
            mapdata[DAT].map[dof/SIZES(DAT,block_size)] = 'D';
    }
}

void show_btree_map(Btree *bt, FILE *file) {
    int i, j;

    /* setup maps */
    for (i=0; i < 2; i++) {
        fseek(FDATA(i, file), 0L, SEEK_END);
        mapdata[i].length = ftell(FDATA(i, file));
        mapdata[i].blkcount =
                    mapdata[i].length/SIZES(i, block_size);
        mapdata[i].map = malloc(mapdata[i].blkcount);
        for (j=0; j < mapdata[i].blkcount; j++)
            mapdata[i].map[j] = '.';
        mapdata[i].map[0] = 'C';
    }

    /* walk the tree */
    show_btree_mapr(bt, FDATA(NDX, root_block));

    /* trace the free blocks */
    for (i=0; i < 2; i++) {
        DISKOFFSET block;
        char *pblock;
        BlockControl *bc;

        block = FDATA(i, free_block);
        while(block) {
            pblock = bt_getblock4read(bt, i, block);
            if (!pblock)
                break;
            mapdata[i].map[block/SIZES(i, block_size)] = 'f';
            bc = (BlockControl *) pblock;
            block = bc -> next;
        }
    }

    /* display the map */
    for (i=0; i < 2; i++) {
        fprintf(file, "%s Block Map:\n      ",
                    i == 0 ? "Index" : "Data");
        for (j=0; j < 10; j++)
            fprintf(file, " %d", j);

        for (j=0; j < mapdata[i].blkcount; j++) {
            if (j % 10 == 0) {
                fprintf(file,"\n%4d: ", j);
            }
            fprintf(file, " %c", mapdata[i].map[j]);
        }
        fprintf(file, "\n");
        free(mapdata[i].map);
    }

}

/*
 * Sample display function
 */
int DataCount = 0;
int DisplayFunc(Btree *bt, void *rec) {
    UR *ur;
    ur = (UR *) rec;
    fprintf(outfile, "k:'%-10s', n:'%-24s', a:'%-24s'\n",
                      ur->key, ur->name, ur->addr);
    DataCount++;
    return TREE_OK;
}

/*
 * Make a new dataset
 */
int make_dataset(char *name)
{
    Btree bt;
    int retval;

    /*
     * Fill in parts of the structure. For the sizes, we
     * give what we want. The only real restriction is that
     * block_size - split must leave enough room to insert
     * the largest possible record or key.
     */
    strcpy(bt.fdata[NDX].filename, name);
    strcat(bt.fdata[NDX].filename, ".ndx");
    bt.fdata[NDX].modified = 0;
    bt.fdata[NDX].bufferlist = NULL;
    bt.fdata[NDX].sizes.block_size = 2048;
    #if defined(TESTSIZES)
    bt.fdata[NDX].sizes.split      = 80;
    bt.fdata[NDX].sizes.merge      = 45;
    #else
    bt.fdata[NDX].sizes.split      = 2000; /* room for one key */
    bt.fdata[NDX].sizes.merge      = 1024;
    #endif
    bt.fdata[NDX].sizes.levels     = 4;

    strcpy(bt.fdata[DAT].filename, name);
    strcat(bt.fdata[DAT].filename, ".dat");
    bt.fdata[DAT].modified = 0;
    bt.fdata[DAT].bufferlist = NULL;
    bt.fdata[DAT].sizes.block_size  = 4096;
    #if defined(TESTSIZES)
    bt.fdata[DAT].sizes.split       = 500;
    bt.fdata[DAT].sizes.merge       = 300;
    #else
    bt.fdata[DAT].sizes.split       = 3950; /* room for a rec */
    bt.fdata[DAT].sizes.merge       = 2048;
    #endif
    bt.fdata[DAT].sizes.levels      = 0; /* any value is OK */

    bt.getkeysize     = UserGetKeySize; /* user.c */
    bt.getkeyNrecsize = UserGetKeyNRecSize;
    bt.getrecsize     = UserGetRecSize;
    bt.key2keycmp     = UserKey2KeyCmp;
    bt.key2reccmp     = UserKey2RecCmp;
    bt.rec2keycpy     = UserRec2KeyCpy;

    bt.error_code   = 0;
    bt.duplicatesOK = 1;

    retval = bt_new(&bt, MaxRec.key, &MaxRec);
    if(retval == TREE_OK)
        printf("Data files created.\n");
    else
        printf("Create failed: %s.\n",
                ErrorText[bt.error_code]);

    return retval;
}

/*
 * Open a dataset
 */
Btree *open_dataset(char *name)
{
	Btree *bt;

	bt = malloc(sizeof(Btree));
	if (!bt)
		return NULL;

	strcpy(bt->fdata[NDX].filename, name);
	strcat(bt->fdata[NDX].filename, ".ndx");
	bt->fdata[NDX].bufferlist = NULL;

	strcpy(bt->fdata[DAT].filename, name);
	strcat(bt->fdata[DAT].filename, ".dat");
	bt->fdata[DAT].bufferlist = NULL;

    bt->getkeysize     = UserGetKeySize; /* user.c */
    bt->getkeyNrecsize = UserGetKeyNRecSize;
    bt->getrecsize     = UserGetRecSize;
    bt->key2keycmp     = UserKey2KeyCmp;
    bt->key2reccmp     = UserKey2RecCmp;
    bt->rec2keycpy     = UserRec2KeyCpy;

	bt->error_code = 0;
	bt->duplicatesOK = 1;

	if (bt_open(bt)) {
        printf("Open failed: %s.\n",
				ErrorText[bt->error_code]);
		free(bt);
		bt = NULL;
	}
    else
        fprintf(stdout, "Opened\n");
	return bt;
}


/*
 * A sample driver
 */
int DoData(Btree *bt, char *buffer, int mode) {
    UR ur;
    char *s;
    s = strchr(buffer, ';');
    if (!s) {
        if (mode == 0)
            s = ""; /* don't need address for deletes */
        else
            return TREE_FAIL; /* trying to add partial data */
    }
    else
        *s++ = '\0';

    strncpy(ur.key, buffer, 10); /* key is first 10 characters */
    ur.key[10] = '\0';
    strncpy(ur.name, buffer, 24);
    ur.name[24] = '\0';
    strncpy(ur.addr, s, 24);
    ur.addr[24] = '\0';

    if (mode == 0)
        return bt_delete(bt, ur.key);
    else
        return bt_add(bt, &ur, ur.key);
}

#define BUFLEN 100
void LoadFile(Btree *bt, char *fname)
{
    FILE *infile;
    char buffer[BUFLEN], *s;
    int i = 0, j = 0, retval = TREE_OK;

    if ((infile = fopen(fname, "r")) == NULL) {
        fputs(" Couldn't open the file.\n", stdout);
        return;
    }

    while (fgets(buffer, BUFLEN, infile)) {
        s = buffer + strlen(buffer);
        while(iscntrl(*s))
            *s-- = 0;

        printf("Loading %s\n", buffer);
        if (buffer[0] == ';') /* a comment */
            ;
        else if (buffer[0] == '-' && buffer[1] != 0) {
            retval = DoData(bt, buffer+1, 0);
            if (retval) {
                printf("  --delete failed\n");
                retval = 0;
            }
            else
                j++;
        }
        else {
            retval = DoData(bt, buffer, 1);
            if (retval)
                break;
            i++;
        }
    }

    if (retval)
        printf("Failed at line %s\n", buffer);

    fclose(infile);
    printf("Loaded %d items and deleted %d from %s.\n",
        i, j, fname);
}


main(int argc, char **argv)
{
    char inbuf[BUFLEN], *s;
    Btree *bt = NULL;
    FILE *logfile = NULL;

    for (;;) {
        fflush(stdout);
        fputs("Action (? for help): ", stdout);
        fflush(stdout);
        fgets(inbuf, BUFLEN, stdin);
        s = inbuf + strlen(inbuf);
        while(iscntrl(*s))
            *s-- = 0;

        if (logfile)
            fprintf(logfile, "%s\n", inbuf);

        if (!bt && strchr("@adfkKmMsSwW", inbuf[0])) {
            fputs(" **no open dataset\n", stdout);
            continue;
        }


        switch (inbuf[0]) {
            case '?':
                fputs(
        "@file      - load strings in file into tree\n"
        "a string   - add name;addr to tree\n"
        "d string   - delete name;addr from tree\n"
        "dup [0|1]  - disallow/allow duplicates\n"
        "f string   - find name;addr in tree\n"
        "k/K [file] - display key counts (K = overwrite file)\n"
        "l file     - log actions to file\n"
        "l          - turn off action logging\n"
        "m/M [file] - display block usage map\n"
        "n file     - make a new dataset\n"
        "o file     - open an existing dataset\n"
        "s/S [file] - display tree (S = overwrite file)\n"
        "w/W [file] - walk tree, (W = overwrite file)\n"
        "q          - quit\n"
        , stdout);
                fflush(stdout);
                break;

            case '@':
                LoadFile(bt, inbuf + 1);
                break;

            case 'a':
                if (inbuf[1] != ' ' ||
                    !inbuf[2]       ||
                    !strchr(inbuf,';'))
                    fputs(" Not a valid command\n", stdout);
                else
                    if (DoData(bt, inbuf + 2, 1) == TREE_FAIL)
                        fputs(" ** Insertion failed\n", stdout);
                break;

            case 'd':
                if (inbuf[1] == 'u' && inbuf[2] == 'p') {
                    if (inbuf[3] == ' ' &&
                        (inbuf[4] == '0' || inbuf[4] == '1'))
                        bt -> duplicatesOK =
                        inbuf[4] == '0' ? 0 : 1;
                    fputs("duplicates are ", stdout);
                    if (bt -> duplicatesOK == 0)
                        fputs("not ", stdout);
                    fputs("allowed.\n", stdout);
                    break;
                }

                if (inbuf[1] != ' ' || inbuf[2] == 0)
                    fputs(" Not a valid command\n", stdout);


                else {
                    if (DoData(bt, inbuf + 2, 0) == TREE_FAIL)
                        fputs(" ** Delete failed\n", stdout);
                }
                break;

            case 'f':
                if (inbuf[1] != ' ' || inbuf[2] == 0)
                    fputs(" Not a valid command\n", stdout);
                else {
                    UR record;
                    inbuf[12] = '\0';
                    if (bt_find(bt, &record, inbuf+2) ==
                                                    TREE_FAIL)
                        fputs(" ** Find failed\n", stdout);
                    else
                        fprintf(stdout, "found %s;%s\n",
                                record.name, record.addr);
                }
                break;

            case 'k': case 'K': {
                    FILE *out;

                    if (inbuf[1] == ' ' && inbuf[2] != 0) {
                        out = fopen(inbuf+2,
                                    inbuf[0] == 'k' ? "w" : "a");
                        if (!out)
                            printf("Can't open %s\n", inbuf + 2);
                        else {
                            show_btree(bt, out, 0);
                            fclose(out);
                        }
                    }
                    else
                        show_btree(bt, stdout, 0);
                }
                break;

            case 'l':
                if (inbuf[1] != ' ' || inbuf[2] == 0) {
                    if (logfile) {
                        fclose(logfile);
                        logfile = NULL;
                    }
                    else
                        fputs(" Logfile not open\n", stdout);
                }
                else {
                    logfile = fopen(inbuf + 2, "w");
                    if (logfile == NULL)
                        printf("Can't open %s\n", inbuf + 2);
                }
                break;

            case 'm': case 'M': {
                    FILE *out;

                    if (inbuf[1] == ' ' && inbuf[2] != 0) {
                        out = fopen(inbuf+2, inbuf[0] ==
                                                'm' ? "w" : "a");
                        if (!out)
                            printf("Can't open %s\n", inbuf + 2);
                        else {
                            show_btree_map(bt, out);
                            fclose(out);
                        }
                    }
                    else
                        show_btree_map(bt, stdout);
                }
                break;

            case 'n':
                if (inbuf[1] != ' ' || inbuf[2] == 0)
                    fputs(" Not a valid command\n", stdout);
                else
                    make_dataset(inbuf+2);
                break;

            case 'o':
				if (inbuf[1] != ' ' || inbuf[2] == 0)
                    fputs(" Not a valid command\n", stdout);
                else {
                    if (bt) {
                        if (bt_close(bt))
                            printf("\nClose failed: %s\n",
                                    ErrorText[bt->error_code]);
                        else
                            printf("\nData files closed.\n");

                        bt = NULL;
                    }
					bt = open_dataset(inbuf+2);
                }
                break;

            case 'q':
                if (logfile)
                    fclose(logfile);

                if (bt) {
                    if (bt_close(bt))
                        printf("\nClose failed: %s\n",
                                ErrorText[bt->error_code]);
                    else
                        printf("\nData files closed.\n");
                }
                return;

            case 's': case 'S': {
                    FILE *out;

                    if (inbuf[1] == ' ' && inbuf[2] != 0) {
                        out = fopen(inbuf+2, inbuf[0] ==
                                             's' ? "w" : "a");
                        if (!out)
                            printf("Can't open %s\n", inbuf + 2);
                        else {
                            show_btree(bt, out, 1);
                            fclose(out);
                        }
                    }
                    else
                        show_btree(bt, stdout, 1);
                }
                break;

            case 'w': case 'W': {
                    if (inbuf[1] == ' ' && inbuf[2] != 0) {
                        outfile = fopen(inbuf+2, inbuf[0] ==
                                             'w' ? "w" : "a");
                        if (!outfile)
                            printf("Can't open %s\n", inbuf + 2);
                        else {
                            DataCount = 0;
                            bt_walk(bt, DisplayFunc);
                            fprintf(outfile, "%d items\n",
                                    DataCount);
                            fclose(outfile);
                            outfile = NULL;
                        }
                    }
                    else {
                        DataCount = 0;
                        outfile = stdout;
                        bt_walk(bt, DisplayFunc);
                        fprintf(outfile, "%d items\n",
                                    DataCount);
                        outfile = NULL;
                    }
                }
                break;

            case ';':
                break;  /* comment */

            default:
                fputs(" Not a valid command\n", stdout);
                break;
        }
        if (bt && bt->error_code) {
            printf("ERROR: %s\n", ErrorText[bt->error_code]);
            bt->error_code = 0;
        }
    }
}
