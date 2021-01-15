/*--- mslzunc.h --------------------------- Listing 9-7 --------
 * Values and bit-manipulation macros used in mslzunc.c
 *-------------------------------------------------------------*/

typedef struct tagCOMPHEADER {
	long    Magic1;
	long    Magic2;
    char    Is41;    /* 0x41 */
    char    FileFix; /* character saved for -r option */
    long    UncompSize;
} COMPHEADER;

/* Microsoft's magic numbers in header. Magic1 = "SZDD" */

#define MAGIC1 0x44445A53
#define MAGIC2 0x3327F088

/* Constants and macros for uncompression */

#define WINSIZE  4096
#define LENGTH(x) ((((x) & 0x0F)) + 3)
#define OFFSET(x1, x2) \
    ((((x2 & 0xF0) << 4) + x1 + 0x0010) & 0x0FFF)
#define WRAPFIX(x)	((x) & (WINSIZE - 1))
#define BITSET(byte, bit)  (((byte) & (1<<bit)) > 0)


