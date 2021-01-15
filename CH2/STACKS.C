/*--- stacks.c ---------------------------- Listing 2-7 ---------
 * Primitives for array-based stacks
 *-------------------------------------------------------------*/

#define IN_STACK_LIB 1

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stacks.h"

/*---------------------------------------------------------------
 *  clear the stack by pointing the top of the stack
 *  at an invalid item; that is, the stack is empty.
 *-------------------------------------------------------------*/
void ClearStack ( Stack *this_stack )
{
    this_stack->top = -1;
}

/*---------------------------------------------------------------
 *  allocate the stack, set the maximum and minimum bounds
 *  on the stack, and show that the stack is empty.
 *-------------------------------------------------------------*/
Stack *CreateStack ( int how_many )
{
    Stack *pstk;

    assert ( how_many > 0 );    /* make sure the size is legal */

    pstk = (Stack *) malloc ( sizeof ( Stack ));
    if ( pstk == NULL )
        return ( NULL );

    pstk->stack_size = how_many;

    pstk->base = ( struct StkElement * )
        malloc ( how_many * sizeof ( struct StkElement ));

    if ( pstk->base == NULL ) /* error in allocating stack */
        return ( NULL );

   pstk->min_stack = 0;
   pstk->max_stack = how_many - 1;

   ClearStack ( pstk );

   return ( pstk );
}

/*---------------------------------------------------------------
 *  pop an element from the stack. If stack is not already empty,
 *  copy the element, and decrement stack top.
 *-------------------------------------------------------------*/
int PopElement ( Stack *this_stack,
                 struct StkElement * destination )
{
    if ( this_stack->top == -1 ) /* stack empty, return error */
        return ( 0 );

    memmove ( destination,
                &(( this_stack->base )[this_stack->top] ),
                sizeof ( struct StkElement ));

    this_stack->top -= 1;

    return ( 1 );
}

/*---------------------------------------------------------------
 *  push an element onto the stack. If stack is not already full,
 *  point StackTop to the next slot, and copy the new element.
 *-------------------------------------------------------------*/
int PushElement ( Stack *this_stack, struct StkElement * to_push )
{
    /* is stack full? */
    if ( this_stack->top == this_stack->max_stack )
        return ( 0 );

    this_stack->top += 1;

    memmove ( &(( this_stack->base )[this_stack->top] ), to_push,
                        sizeof ( struct StkElement ));

    return ( 1 );
}

/*---------------------------------------------------------------
 *  view an element on the stack. Function is passed a value that
 *  specifies the element's position in terms of its distance
 *  from the top. 0 is the top, 1 is the element below the top,
 *  2 is the element below that. If an invalid value is passed,
 *  the function returns NULL; otherwise, it returns a pointer
 *  to the requested element.
 *-------------------------------------------------------------*/
struct StkElement * ViewElement ( Stack *this_stack,
                                  int which_element )
{
    if ( this_stack->top == -1 )
        return ( NULL );

    if ( this_stack->top - which_element < 0 )
        return ( NULL );

    return ( &(( this_stack->base )
                    [this_stack->top - which_element] ));
}
