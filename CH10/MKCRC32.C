/*--- mkcrc32.c ---------------------------- Listing 10-8 ------
 *  Makes a CRC-32 table for 32-bit lookups in C-usable
 *  format. Outputs to screen; redirect to disk as needed.
 *  For example:
 *                  mkcrc32 > crc32.tbl
 *
 *  Adapted from code by Mark Nelson (DDJ, May 1992)
 -------------------------------------------------------------*/

#include <stdio.h>

unsigned long table[ 256 ];

unsigned long CalculateCRC32 ( int );

void main ( void )
{
   unsigned int i;

   /* first calculate the CRC-32 values */

   for ( i = 0; i < 256; i++ )
      table [i] = CalculateCRC32 ( i );

   /* then print out the table */

   printf ( "/* CRC-32 Lookup Table */ \n" );
   printf ( "unsigned long crc32_table[256] =\n{" );

   for ( i = 0; i < 256; i++ )
   {
      /* start a new row every four CRCs */

      if (( i % 4 ) == 0 )
         printf ( "\n  /* %3u -- */  ", i );

      /* print the CRC */

      printf ( "0x%08lX", (unsigned long) table [i] );
      if ( i != 255 )
          printf ( ", " );
   }

   /* print closing brace for table */

   printf ( "\n};\n" );
}

unsigned long CalculateCRC32 ( int i )
{
    int j;
    unsigned long crc;

    crc = i;

    for ( j = 8 ; j > 0; j-- )
    {
        if ( crc & 1 )
            crc = ( crc >> 1 ) ^ 0xEDB88320L;
        else
            crc >>= 1;
    }
    return ( crc );
}
