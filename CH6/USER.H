/*--- user.h ------------------------------ Listing 6-8 ---------
 * Sample data record definition
 *-------------------------------------------------------------*/

/* example data records */
typedef struct UserRecord {
    char key[11];
    char name[25];
    char addr[25];
} UR;

/* a record with a maximal key. initialized in user.c */
extern UR MaxRec;

/* The routines */
unsigned UserGetKeySize ( void *r );
unsigned UserGetRecSize ( void *r );
int UserKey2KeyCmp ( void *k1, void *k2 );
int UserKey2RecCmp ( void *k, void *r );
void UserRec2KeyCpy ( void *k, void *r );
