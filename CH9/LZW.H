/*--- lzw.h ------------------------------- Listing 9-8 --------
 * Header file for LZW compression routines                    *
 *-------------------------------------------------------------*/

/* select dictionary size */
#if !defined(BITS)
#define BITS 12
#endif

#if BITS == 12
/* Critical sizes for LZW */
#define PRESET_CODE_MAX 256    /* codes like this are preset */
#define END_OF_INPUT    256    /* this code terminates input */
#define NEW_DICTIONARY  257    /* reinitialize the dictionary */
#define STARTING_CODE   258    /* first code we can use */
#define MAX_CODE        4096   /* 2 ^ BITS */
#define UNUSED_CODE     4096   /* an invalid code that is never
                                  output. may be >= MAX_CODE if
                                  CodeType can hold it. The
                                  other option is to define
                                  after the fashion of the
                                  code for END_OF_INPUT */
#define DICTIONARY_SIZE 5021   /* a prime # > MAX_CODE * 1.2 */

/* all of these should be unsigned */
typedef unsigned short CodeType;  /* can hold MAX_CODE */
typedef unsigned short IndexType; /* can hold DICTIONARY_SIZE */
typedef unsigned long  CountType; /* used for statistics only */

#define SIGNATURE "LZW12"

#elif BITS == 16 /* requires a 32-bit context */
/* Critical sizes for LZW */
#define PRESET_CODE_MAX 256    /* codes like this are preset */
#define END_OF_INPUT    256    /* this code terminates input */
#define NEW_DICTIONARY  257    /* reinitialize the dictionary */
#define UNUSED_CODE     258    /* an invalid code */
#define STARTING_CODE   259    /* first code we can use */
#define MAX_CODE        65536  /* 2 ^ BITS */
#define DICTIONARY_SIZE 81901L /* a prime # > MAX_CODE * 1.2 */

typedef unsigned short CodeType; /* can hold MAX_CODE */
typedef unsigned long IndexType; /* can hold DICTIONARY_SIZE */
typedef unsigned long CountType; /* used for statistics only */

#define SIGNATURE "LZW16"

#else
#error Undefined BITS value!
#endif

