/*--- ElfHash ----------------------------- Listing 3-3 ---------
 *  The published hash algorithm used in the UNIX ELF format
 *  for object files.
 *
 *  Accepts a pointer to a string to be hashed and returns an
 *  unsigned long. Algorithm is similar to that implemented
 *  in HashPJW (see Listing 3-2).
 *-------------------------------------------------------------*/

unsigned long ElfHash ( const unsigned char *name )
{
    unsigned long   h = 0, g;

    while ( *name )
    {
        h = ( h << 4 ) + *name++;
        if ( g = h & 0xF0000000 )
            h ^= g >> 24;

        h &= ~g;
    }
    return h;
}
