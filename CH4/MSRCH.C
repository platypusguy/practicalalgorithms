/*--- MSRCH.C ----------------------------- Listing 4-4 --------
 * Purpose: search text for multiple keywords simultaneously
 * Switches: DRIVER  - will cause a test driver to be compiled
 *           MAXCHAR - maximum number of symbols recognized
 *
 * Usage: The sample driver illustrates all the key points.
 *        There are three routines:
 *
 *        (1) MsrchInit ( struct kword *) is passed list of
 *            words to search for
 *
 *        (2) MsrchGo ( int (*MsrchData) (),
 *                       void (*MsrchSignal) (char *) );
 *            does the work. It uses two pointers to functions:
 *            the first retrieves a character, the second is
 *            called when a match is found.
 *
 *        (3) MsrchEnd ( void ) cleans up the work areas
 *
 *------------------------------------------------------------*/

#define DRIVER 1            /* compile a test driver */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct kword {              /* linked list of keywords */
    unsigned char *word;    /*   to search for.        */
    struct  kword *next;
};


#define MAXCHAR  256        /* max number of different chars
                                                we search for */
static int MaxState;        /* max number of states
                                                we have room for */

static int *MatchArray;     /*  First level of matching.
                             *  Possible values:
                             *      (1) EMPTY_SLOT  -2
                             *      (2) a character
                             *      (3) MULTI_WAY   -1
                             */

#define MULTI_WAY   -1      /* flags for match_array */
#define EMPTY_SLOT  -2

union GotoTable {      /* values in MatchArray take us here:  */
    int GotoState;     /* go here if MatchArray is a character*/
    int *BranchTable;  /* or to this MULTI_WAY branching table*/
} static *GotoArray;

#define FAIL_STATE  -1      /* in GotoState or BranchTable,
							    this means failure */

/* OutArray[] is the Output function */
                        /* list of keywords 'found' by states */
static struct kword **OutArray;

/* FailArray[] is the Fail function */
static int *FailArray;     /* failure transition array */


/* variable to track next free state */
static int HighState;

/* functions we use */
static void AddStateTrans ( int, int, int );
static void ComputeFail ( void );
static void Enter ( unsigned char * );
static void FindFail ( int state, int s, int a );
static void QueueAdd ( int *queue, int qbeg, int new );

/* set up tables needed by MsrchGo() */
void MsrchInit ( struct kword *klist )
{
	int i;
	struct kword *ktemp;

    /* compute maximum number of possible states */
    MaxState = 1;
    for ( ktemp = klist; ktemp != NULL; ktemp = ktemp->next )
          MaxState += strlen ( ktemp->word );

	/* allocate space for arrays */

    MatchArray = (int *) malloc ( sizeof(int) * MaxState );
    GotoArray  = (union GotoTable *) malloc (
                        sizeof(union GotoTable) * MaxState );
    OutArray   = (struct kword **) malloc (
                        sizeof(struct kword *) * MaxState );
    FailArray  = (int *) malloc ( sizeof(int) * MaxState );

	/* initialize state arrays */
    for ( i = 0; i < MaxState; i++ )
    {
        MatchArray[i] = EMPTY_SLOT;
        OutArray[i]   = NULL;
	}

	/* initialize state_array[0] */
    HighState = 0;
    AddStateTrans ( 0, 'a', FAIL_STATE );
         /* force a multiway table */
    AddStateTrans ( 0, 'b', FAIL_STATE );

    /* step through keywords */
    for ( ; klist != NULL; klist = klist->next )
         Enter ( klist->word );

	/* setup return to zero transitions for state[0] */
    for  ( i = 0; i < MAXCHAR; i++ )
        if ( GotoArray[0].BranchTable[i] == FAIL_STATE )
             GotoArray[0].BranchTable[i] = 0;

	/* and compute failure array */
    ComputeFail();
}

/* add transition from OldState -> NewState for MatchChar */
static void AddStateTrans ( int OldState,
                              int MatchChar,
                              int NewState )
{
	int i, *temp;

	/* is this slot empty? */
    if ( MatchArray[OldState] == EMPTY_SLOT )  /* this is easy */
    {
        MatchArray[OldState] = MatchChar;
        GotoArray[OldState].GotoState = NewState;
	}
        /* is there already a multi-way table? */
    else
    if ( MatchArray[OldState] == MULTI_WAY ) /* easy, too */
         GotoArray[OldState].BranchTable[MatchChar] = NewState;

        /* need to convert to multi-way table */
    else
    {
        temp = (int *) malloc ( sizeof(int) * MAXCHAR );
        for ( i = 0; i < MAXCHAR; i++ )
			temp[i] = FAIL_STATE;

		/* copy data from single way branch */
        temp[MatchArray[OldState]] =
            GotoArray[OldState].GotoState;

		/* and new data */
        temp[MatchChar] = NewState;

		/* and load it all into state_array */
        MatchArray[OldState] = MULTI_WAY;
        GotoArray[OldState].BranchTable = temp;
	}
}

/* add kword to list of words our machine recognizes */
static void Enter ( unsigned char *kword )
{
	int state, k;
	char *save;
	struct kword *ktemp;

	state = 0;
	save = kword;	/* keep a copy */

    /*  first, see whether we can place this word
     *  on top of an existing one
     */

    for ( ; *kword != '\0'; kword++ )
    {
        /* is this a single char slot? */

        if ( MatchArray[state] == *kword )
            state = GotoArray[state].GotoState;

        else      /* multi-way? */
        if ( MatchArray[state] == MULTI_WAY )
        {
            if (( k = GotoArray[state].BranchTable[*kword] )
                    == FAIL_STATE )
				break;
            else    /* we have a transition for this char */
                state = k;
		}
        else        /* no match for this char */
            break;
	}

	/* now add new states as needed */
    for ( ; *kword != '\0'; kword++ )
    {
        HighState += 1;
        if ( HighState >= MaxState )  /* uh-oh ... */
        {
            fputs( "INTERNAL ERROR: too many states\n", stderr );
            exit ( EXIT_FAILURE );
		}
        AddStateTrans ( state, *kword, HighState );
        state = HighState;
	}

	/* now add this keyword to output list for final state */
    ktemp = (struct kword *) malloc ( sizeof ( struct kword ));
	ktemp->word = save;
    ktemp->next = OutArray[state];
    OutArray[state] = ktemp;
}

/* build FailArray and update GotoArray */
static void ComputeFail()
{
    int *queue, qbeg, r, s;
	int i;

	/* allocate a queue */
    queue = (int *) malloc ( sizeof ( int ) * MaxState );
	qbeg = 0;
	queue[0] = 0;

    /* scan first level and setup initial values for FailArray */
    for ( i = 0; i < MAXCHAR; i++ )
        if (( s = GotoArray[0].BranchTable[i] ) != 0 )
        {
            FailArray[s] = 0;
            QueueAdd ( queue, qbeg, s );
		}

	/* now scan lower levels */
    while ( queue[qbeg] != 0 )
    {
        /* pull off state from front of queue and advance qbeg*/

        r = queue[qbeg];
        qbeg = r;

		/* now investigate this state */
        if ( MatchArray[r] == EMPTY_SLOT )
             continue;      /* no more to do */
        else
        if ( MatchArray[r] == MULTI_WAY )
        {
			/* scan its subsidiary states */
            for ( i = 0; i < MAXCHAR; i++ )
                                /* scan BranchTable */
                if (( s = GotoArray[r].BranchTable[i] )
                        != FAIL_STATE )
                {               /* add new state to queue */
                    QueueAdd ( queue, qbeg, s );
                    FindFail ( FailArray[r], s, i );
				}
		}
        else  /* single char */
        {
            QueueAdd ( queue, qbeg, GotoArray[r].GotoState );
            FindFail ( FailArray[r], GotoArray[r].GotoState,
                        MatchArray[r] );
		}
	}

	/* tidy up */
    free ( queue );
}

/*---------------------------------------------------------------
 *  Actually compute failure transition.  We know that 'a'
 *  would normally cause us to go from state s1 to s2.
 *  To compute the failure value, we backtrack in search
 *  of other places 'a' might go.
 *-------------------------------------------------------------*/
static void FindFail ( int s1, int s2, int a )
{
	int on_fail;
	struct kword *ktemp, kdummy, *out_copy, *kscan;

    for ( ; ; s1 = FailArray[s1] )
        if ( MatchArray[s1] == a )
        {
            if (( on_fail = GotoArray[s1].GotoState )
                        != FAIL_STATE )
				break;
		}
        else
        if ( MatchArray[s1] != EMPTY_SLOT )
            if (( on_fail = GotoArray[s1].BranchTable[a] )
                        != FAIL_STATE )
				break;

    FailArray[s2] = on_fail;

	/* merge output lists */

    /* first, make a copy of OutArray[on_fail] */
    if ( OutArray[on_fail] == NULL )
        out_copy = NULL;
    else
    {
        kscan = OutArray[on_fail];
        out_copy = malloc ( sizeof ( struct kword ));
		out_copy->word = kscan->word;
		out_copy->next = NULL;
        for ( kscan = kscan->next;
              kscan != NULL;
              kscan = kscan->next )
        {
            ktemp = malloc ( sizeof ( struct kword ));
			ktemp->word = kscan->word;
			ktemp->next = out_copy->next;
			out_copy->next = ktemp;
		}
	}

	/* now merge them */
    if (( kdummy.next = OutArray[s2] ) != NULL )
    {
		ktemp = &kdummy;
        for ( ; ktemp->next->next != NULL; ktemp = ktemp->next )
                ;
		ktemp->next->next = out_copy;
	}
	else
        OutArray[s2] = out_copy;
}

/* add new to end of queue */
static void QueueAdd ( int *queue, int qbeg, int new )
{
	int q;

	q = queue[qbeg];
    if ( q == 0 )           /* is list empty? */
        queue[qbeg] = new;  /* yes */
    else                    /* no: scan to next-to-last link */
    {
        for ( ; queue[q] != 0; q = queue[q] )
                ;
		queue[q] = new;	/* put this state at end of queue */
	}

	/* and terminate list */
	queue[new] = 0;
}

/* do the actual search */
void MsrchGo ( int  (*MsrchData) (),
               void (*MsrchSignal) (char *) )
{
	int state, c, g, m;
	struct kword *kscan;

	state = 0;
    while (( c = MsrchData() ) != EOF )
    {
        /* what is goto ( state, c ) ? */
        for ( ;; )
        {
            /*-------------------------------------------------
             *  We cheat slightly in the interest of
             *  speed/simplicity. The machine will spend most
             *  of its time in state==0, and this state is
             *  always a MULTI_WAY table. Since this is a
             *  simple test, we make it first and try to save
             *  the calculation of an array index
             *-----------------------------------------------*/

            if ( state == 0 ||
                 ( m = MatchArray[state] ) == MULTI_WAY )
                g = GotoArray[state].BranchTable[c];
            else
            if ( m == c )
                g = GotoArray[state].GotoState;
			else
				g = FAIL_STATE;

            if ( g != FAIL_STATE )
				break;

            state = FailArray[state];
		}
		state = g;

		/* anything to output? */
        if (( kscan = OutArray[state] ) != NULL )
            for ( ; kscan != NULL; kscan = kscan->next )
                MsrchSignal ( kscan->word );
	}
}

/* free all the arrays we created */
void MsrchEnd ( void )
{
	int i;
	struct kword *kscan;

    for ( i = 0; i < MaxState; i++ )
          if ( MatchArray[i] == MULTI_WAY )
               free ( GotoArray[i].BranchTable );

    free ( MatchArray );
    free ( GotoArray );
    free ( FailArray );

    for ( i = 0; i < MaxState; i++ )
        if ( OutArray[i] != NULL )
             for ( kscan = OutArray[i];
                   kscan!=NULL;
                   kscan = kscan->next )
                 free ( kscan );
    free ( OutArray );
}

/*---------------------------------------------------------------
 * This test driver expects a command line of the form
 *    msrch file word-1 word-2 word-3 .... word-n
 *
 * It will then search file for all words on the command line.
 * The results are written to stdout. This illustrates all the
 * features of using the multisearch routines.
 *
 * This is an admittedly simple design--the search routine would
 * certainly be faster if the character fetch routine was put
 * directly into the MsrchGo() module.  However, to avoid using
 * application-specific code in the demonstration version of
 * these routines, it is coded as a separate subroutine.
 *--------------------------------------------------------------*/

#ifdef DRIVER

#define BUFSIZE 200

FILE *infile;
char inbuf[BUFSIZE];
char *inbufptr;
int  linecount;

/* declare the routines that MsrchGo() will use */
int RetrieveChar ( void );
void FoundWord();

int main ( int argc, char **argv )
{
	char infile_name[80];
	struct kword *khead, *ktemp;
	int i;

    if ( argc < 3 )
    {
        fprintf ( stderr,
            "Usage: msrch infile word-1 word-2 ... word-n\n" );
        exit ( EXIT_FAILURE );
	}

    strcpy ( infile_name, argv[1] );

    if (( infile = fopen ( infile_name, "r" )) == NULL )
    {
        fprintf ( stderr, "Cannot open %s\n", infile_name );
        exit ( EXIT_FAILURE );
	}
	linecount = 0;
	inbufptr = NULL;

    /* turn command-line parameters into a list of words */
	khead = NULL;
    for ( i = 3; i <= argc; i++ )
    {
        ktemp = (struct kword *) malloc ( sizeof ( struct kword ));
		ktemp->word = argv[i-1];
		ktemp->next = khead;
		khead = ktemp;
	}

    MsrchInit ( khead ); /* setup system; pass list of words */

    /* Now search. Note call to functions by use of pointers */
    MsrchGo ( RetrieveChar, FoundWord );

    MsrchEnd();            /* clean up */
    return ( EXIT_SUCCESS );
}


/* get next character from input stream.  Routine returns either
 *   (a) a character (as an int without its sign extended), or
 *   (b) EOF
 */
int RetrieveChar ( void )
{
	int c;

    if ( inbufptr == NULL || *(++inbufptr) == '\0' )
    {    /* read a new line of data */
        if ( fgets ( inbuf, BUFSIZE, infile ) == NULL )
        {
            fclose ( infile );
            return ( EOF );
		}
		inbufptr = inbuf;
        linecount += 1;
	}

	c = *inbufptr;
	c &= 0x00FF;	/* make sure it is not sign extended */
    return ( c );
}

/* FoundWord: called by MsrchGo() when it finds a match */
void FoundWord(char *word)
{
	int i;

    fprintf ( stdout, "Line %d\n%s", linecount, inbuf );

    i = ( inbufptr - inbuf ) - strlen ( word ) + 1;
    for ( ; i > 0; i-- )
         fputc ( ' ', stdout );

    fprintf ( stdout, "%s\n\n", word );
}
#endif
