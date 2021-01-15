/*--- CCITTOTF.C  ------------------------- Listing 10-4 -------
 * Compute CCITT-CRC on-the-fly.
 * Usage:   ccittotf filename
 *
 * Based on a similar program by Nigel Cort in C Gazette 5.1
 * (Autumn 1990).
 *------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#define DRIVER 1 /* DRIVER includes the file processing */


/*  Compute single-byte CRC-CCITT on-the-fly */

unsigned short GetCCITT ( unsigned short crc, unsigned short ch )
{
    static unsigned int i;
    ch <<= 8;                             /* Move to MSB       */
    for ( i = 8; i > 0; i-- )             /* Go through 8 bits */
    {
        if ( (ch ^ crc) & 0X8000 )        /* Perform CRC calc. */
             crc = ( crc << 1 ) ^ 0x1021;
        else
             crc <<= 1;
        ch <<= 1;
    }
    return ( crc );
}

#ifdef DRIVER
int main ( int argc, char * argv[] )
{
   FILE *fin;            /* file we're reading into buffer */
   char *buffer;         /* buffer we're working on        */
   size_t i, j;          /* counters of bytes in buff      */
   unsigned short crc;   /* the CRC value being computed   */

   if ( argc < 2 )
   {
      fprintf ( stderr, "Error! Must specify filename.\n" );
      return ( EXIT_FAILURE );
   }

   if (( fin = fopen ( argv[1], "rb" )) == NULL )
   {
      fprintf ( stderr, "Cannot open %s", argv[1] );
      return ( EXIT_FAILURE );
   }

   /*--------------------------------------------------
    * Set up a very large input buffer of 32K bytes.
    * This program does no good if it doesn't fly!
    *-------------------------------------------------*/

   if (( buffer = (char *) malloc ( 32766 )) == NULL )
   {
      fprintf ( stderr, "Out of memory\n" );
      return ( EXIT_FAILURE );
   }

   crc = 0;

   for (;;)
   {
      i = fread ( buffer, 1, 32766, fin );
      if ( i == 0 )
      {
         if ( feof ( fin ))  /* we're done, so show results */
         {
            printf ( "CCITT CRC for %s is %04X\n",
                                            argv[1], crc );
            return ( EXIT_SUCCESS );
         }
         else         /* read another 32K of file */
            continue;
      }

      for ( j = 0 ; j < i; j ++ )  /* loop through the buffer */
            crc = GetCCITT ( crc, buffer [j] );
   }
}
#endif
