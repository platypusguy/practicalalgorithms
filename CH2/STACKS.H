/*--- stacks.h ---------------------------- Listing 2-6 ---------
 *  Header file for stack operations
 *-------------------------------------------------------------*/

#ifndef STACKS_H
#define STACKS_H    1

#ifdef IN_STACK_LIB
#define StkExtern
#else
#define StkExtern   extern
#endif

struct stack_struct {
    struct StkElement *base;        /* point to base of stack */
    int                stack_size;  /* number of elements */
    int                min_stack;   /* bottom-most element */
    int                max_stack;   /* last possible element */
    int                top;         /* current top */
};

typedef struct stack_struct Stack;

StkExtern void   ClearStack    ( Stack * );
StkExtern Stack* CreateStack   ( int );
StkExtern int    PopElement    ( Stack *, struct StkElement * );
StkExtern int    PushElement   ( Stack *, struct StkElement * );
StkExtern struct StkElement *
                 ViewElement   ( Stack *, int );

/*--- Application-specific material goes below ---*/

StkExtern struct StkElement  {
              int   line_no;
              char  opener;
};

#endif
