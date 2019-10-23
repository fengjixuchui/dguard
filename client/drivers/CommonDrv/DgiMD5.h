#pragma once

#include <ntifs.h>

typedef unsigned char	MD5BYTE; /* 8-bit byte */
typedef unsigned int	MD5WORD; /* 32-bit word */
 
typedef struct _SZI_MD5STATE {
    MD5WORD count[2];    /* message length in bits, lsw first */
    MD5WORD abcd[4];     /* digest buffer */
    MD5BYTE buf[64];     /* accumulate block */
} SZI_MD5STATE;
 
VOID
DgiMD5Init(SZI_MD5STATE *pms);
 
VOID
DgiMD5Append(SZI_MD5STATE *pms, const MD5BYTE *data, int nbytes);
 
VOID
DgiMD5Finish(SZI_MD5STATE *pms, MD5BYTE digest[16]);
