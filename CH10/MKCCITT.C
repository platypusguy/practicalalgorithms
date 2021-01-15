/*---mkccitt.c ---------------------------- Listing 10-5 -------
 *  Makes a CRC-CCITT table for lookups in C-usable
 *  format. Outputs to screen; Redirect to disk as needed.
 *  For example:
 *                  mkccitt > ccitt.tbl
 *
 *  Based on code from Nigel Cort in C Gazette 5.1 (Autumn 1990)
 -------------------------------------------------------------*/

#include <stdio.h>

unsigned short CalculateCCITT ( unsigned int );

unsigned short table [256];

void main ( void )
{
   unsigned int i;

   /* first calculate the CRC values */

   for ( i = 0; i < 256; i++ )
      table [i] = CalculateCCITT ( i );

   /* then print out the table */

   printf ( "/* CCITT Lookup Table */ \n" );
   printf ( "unsigned short ccitt_table[256] =\n{" );

   for ( i = 0; i < 256; i++ )
   {
      /* start a new row every eight CRCs */

      if (( i % 8 ) == 0 )
         printf ( "\n  /* %3u -- */  ", i );

      /* print the CRC */

      printf ( "0x%04X", table [i] );
      if ( i != 255 )
          printf ( ", " );
   }

   /* print closing brace for table */

   printf ( "\n};\n" );
}

unsigned short CalculateCCITT ( unsigned int index )
{
   unsigned short a, i;
   a = 0;         /* serves as an accumulator */

   index <<= 8;

   /* The heart of the CRC-CCITT */

   for ( i = 8; i > 0; i-- )
   {
      if (( index ^ a ) & 0x8000 )
         a = ( a << 1 ) ^ 0x1021;
      else
         a <<= 1;
      index <<= 1;
   }

   return ( a );
}
