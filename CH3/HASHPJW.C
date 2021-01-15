/*--- HashPJW ----------------------------- Listing 3-2 ---------
 *	An adaptation of Peter Weinberger's (PJW) generic hashing
 *  algorithm based on Allen Holub's version.
 *
 *  Accepts a pointer to a datum to be hashed and returns an
 *  unsigned integer. This integer is called "calculated-key"
 *  in the text.
 *-------------------------------------------------------------*/

#include <limits.h>

#define BITS_IN_int ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH		((int) (BITS_IN_int / 8))
#define HIGH_BITS		( ~((unsigned int)(~0) >> ONE_EIGHTH ))

unsigned int HashPJW ( const char * datum )
{
	unsigned int hash_value, i;

	for ( hash_value = 0; *datum; ++datum )
	{
		hash_value = ( hash_value << ONE_EIGHTH ) + *datum;
        if (( i = hash_value & HIGH_BITS ) != 0 )
			hash_value =
                ( hash_value ^ ( i >> THREE_QUARTERS )) &
                        ~HIGH_BITS;
	}

	return ( hash_value );
}
