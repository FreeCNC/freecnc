#ifndef _LIB_WSKEY_H
#define _LIB_WSKEY_H

// Decodes Westwood's 80-byte encryption key
// src: The 80 byte westwood key.
// dest: The resulting 56 byte blowfish key will be stored here.
void decode_westwood_key(const unsigned char* src, unsigned char* dest);

#endif
