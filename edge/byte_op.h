#ifndef __BYTE_OP_H__     // Prevent multiple inclusion of this header file
#define __BYTE_OP_H__

#include <cstdio>         
#include <cstring>

// ====== Variable to Memory Conversion ======
// Store a 1-byte (8-bit) variable into memory
#define VAR_TO_MEM_1BYTE_BIG_ENDIAN(v, p) \
  *(p++) = v & 0xff;    // Store the lowest 8 bits of the value, then increment the pointer

// Store a 2-byte (16-bit) variable into memory in Big Endian order
#define VAR_TO_MEM_2BYTES_BIG_ENDIAN(v, p) \      
  *(p++) = (v >> 8) & 0xff;       /* Store the high byte first */  \
  *(p++) = v & 0xff;              /* Then the low byte */

// Store a 4-byte (32-bit) variable into memory in Big Endian order
#define VAR_TO_MEM_4BYTES_BIG_ENDIAN(v, p) \
  *(p++) = (v >> 24) & 0xff;      /* Store the most significant byte */ \
  *(p++) = (v >> 16) & 0xff;      /* Then the next most significant byte */ \
  *(p++) = (v >> 8) & 0xff;       /* Then the next byte */ \
  *(p++) = v & 0xff;              /* Store the least significant byte */

// ====== Memory to Variable Conversion ======
// Read a 1-byte value from memory and store it in a variable
#define MEM_TO_VAR_1BYTE_BIG_ENDIAN(p, v) \
  v = (p[0] & 0xff);              /* Read a single byte */ \
  p += 1;                         /* Advance pointer to next byte */

// Read a 2-byte (16-bit) Big Endian value from memory
#define MEM_TO_VAR_2BYTES_BIG_ENDIAN(p, v) \
  v = ((p[0] & 0xff) << 8) | (p[1] & 0xff);  /* Combine high and low bytes */ \
  p += 2;                                    /* Advance pointer by 2 bytes */

// Read a 4-byte (32-bit) Big Endian value from memory
#define MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, v) \
  v = ((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) | ((p[2] & 0xff) << 8) | (p[3] & 0xff); \
  p += 4;                                    /* Advance pointer by 4 bytes */

// ====== Buffer Debug Printing ======
// This macro prints a byte buffer in hex format for debugging.
// It prints 16 bytes per line with proper formatting.
#define PRINT_MEM(p, len) \
  printf("Print buffer:\n  >> "); \
  for (int i=0; i<len; i++) { \
    printf("%02x ", p[i]);                   /* Print each byte in two-digit hex */ \                  
    if (i % 16 == 15) printf("\n  >> ");     /* Add line break every 16 bytes */ \  
  } \
  printf("\n");                              /* Final newline after the buffer is printed */

#endif /* __BYTE_OP_H__ */
