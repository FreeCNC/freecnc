/*****************************************************************************
 * ws-key.cpp - code to decode Westwood's 80-byte encryption key
 *
 * Originally from Olaf van der Spek's XCC Mixer program.
 * Introduced to the FreeCNC code base by Kareem Dana
 * Various stylistic changes by Euan MacGregor
 *****************************************************************************/
#include <cstring>
#include "ws-key.h"

const char* pubkey_str = "AihRvNoIbTn85FZRYNZRcT+i6KpU+maCsEqr3Q5q+LDB5tH7Tz2qQ38V";

const char char2num[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

typedef unsigned int bignum4[4];
typedef unsigned int bignum[64];
typedef unsigned int bignum130[130];

struct pubkey_t {
    bignum key1;
    bignum key2;
    unsigned int len;
} pubkey;
bignum glob1;
unsigned int glob1_bitlen, glob1_len_x2;
bignum130 glob2;
bignum4 glob1_hi, glob1_hi_inv;
unsigned int glob1_hi_bitlen;
unsigned int glob1_hi_inv_lo, glob1_hi_inv_hi;

void init_bignum(bignum n, unsigned int val, unsigned int len)
{
    memset((void *)n, 0, len * 4);
    n[0] = val;
}

void move_key_to_big(bignum n, char *key, unsigned int klen, unsigned int blen)
{
    unsigned int sign;
    int i;

    if (key[0] & 0x80)
        sign = 0xff;
    else
        sign = 0;

    for (i = blen*4; (unsigned int)i > klen; i--)
        ((char *)n)[i-1] = (char)sign;
    for (; i > 0; i--)
        ((char *)n)[i-1] = (char)key[klen-i];
}

void key_to_bignum(bignum n, char *key, unsigned int len)
{
    unsigned int keylen;
    int i;

    if (key[0] != 2)
        return;
    key++;

    if (key[0] & 0x80) {
        keylen = 0;
        for (i = 0; i < (key[0] & 0x7f); i++)
            keylen = (keylen << 8) | key[i+1];
        key += (key[0] & 0x7f) + 1;
    } else {
        keylen = key[0];
        key++;
    }
    if (keylen <= len*4)
        move_key_to_big(n, key, keylen, len);
}

unsigned int len_bignum(bignum n, unsigned int len)
{
    int i;
    i = len-1;
    while ((i >= 0) && (n[i] == 0))
        i--;
    return i+1;
}

unsigned int bitlen_bignum(bignum n, unsigned int len)
{
    unsigned int ddlen, bitlen, mask;
    ddlen = len_bignum(n, len);
    if (ddlen == 0)
        return 0;
    bitlen = ddlen * 32;
    mask = 0x80000000;
    while ((mask & n[ddlen-1]) == 0) {
        mask >>= 1;
        bitlen--;
    }
    return bitlen;
}

void init_pubkey()
{
    unsigned int i, i2, tmp;
    char keytmp[256];

    init_bignum(pubkey.key2, 0x10001, 64);

    i = 0;
    i2 = 0;
    while (i < strlen(pubkey_str)) {
        tmp = char2num[(unsigned char)pubkey_str[i++]];
        tmp <<= 6;
        tmp |= char2num[(unsigned char)pubkey_str[i++]];
        tmp <<= 6;
        tmp |= char2num[(unsigned char)pubkey_str[i++]];
        tmp <<= 6;
        tmp |= char2num[(unsigned char)pubkey_str[i++]];
        keytmp[i2++] = (char)((tmp >> 16) & 0xff);
        keytmp[i2++] = (char)((tmp >> 8) & 0xff);
        keytmp[i2++] = (char)(tmp & 0xff);
    }
    key_to_bignum(pubkey.key1, keytmp, 64);
    pubkey.len = bitlen_bignum(pubkey.key1, 64) - 1;
}

unsigned int len_predata()
{
    unsigned int a = (pubkey.len - 1) / 8;
    return (55 / a + 1) * (a + 1);
}

long int cmp_bignum(bignum n1, bignum n2, unsigned int len)
{
    n1 += len-1;
    n2 += len-1;
    while (len > 0) {
        if (*n1 < *n2)
            return -1;
        if (*n1 > *n2)
            return 1;
        n1--;
        n2--;
        len--;
    }
    return 0;
}

void mov_bignum(bignum dest, bignum src, unsigned int len)
{
    memmove(dest, src, len*4);
}

void shr_bignum(bignum n, unsigned int bits, long int len)
{
    unsigned int i, i2;

    i2 = bits / 32;
    if (i2 > 0) {
        for (i = 0; i < len - i2; i++)
            n[i] = n[i + i2];
        for (; i < (unsigned long int)len; i++)
            n[i] = 0;
        bits = bits % 32;
    }
    if (bits == 0)
        return;
    for (i = 0; i < (unsigned long int)len - 1; i++)
        n[i] = (n[i] >> bits) | (n[i + 1] << (32 - bits));
    n[i] = n[i] >> bits;
}

void shl_bignum(bignum n, unsigned int bits, unsigned int len)
{
    unsigned int i, i2;

    i2 = bits / 32;
    if (i2 > 0) {
        for (i = len - 1; i > i2; i--)
            n[i] = n[i - i2];
        for (; i > 0; i--)
            n[i] = 0;
        bits = bits % 32;
    }
    if (bits == 0)
        return;
    for (i = len - 1; i > 0; i--)
        n[i] = (n[i] << bits) | (n[i - 1] >> (32 -
                                              bits));
    n[0] <<= bits;
}

unsigned int sub_bignum(bignum dest, bignum src1, bignum src2, unsigned int carry, unsigned int len)
{
    unsigned int i1, i2;

    len += len;
    while ((long)--len != -1) {
        i1 = *(unsigned short *)src1;
        i2 = *(unsigned short *)src2;
        *(unsigned short *)dest = (unsigned short)(i1 - i2 - carry);
        src1 = (unsigned int *)(((unsigned short *)src1) + 1);
        src2 = (unsigned int *)(((unsigned short *)src2) + 1);
        dest = (unsigned int *)(((unsigned short *)dest) + 1);
        if ((i1 - i2 - carry) & 0x10000)
            carry = 1;
        else
            carry = 0;
    }
    return carry;
}

void inv_bignum(bignum n1, bignum n2, unsigned int len)
{
    bignum n_tmp;
    unsigned int n2_bytelen, bit;
    long int n2_bitlen;

    init_bignum(n_tmp, 0, len);
    init_bignum(n1, 0, len);
    n2_bitlen = bitlen_bignum(n2, len);
    bit = ((unsigned int)1) << (n2_bitlen % 32);
    n1 += ((n2_bitlen + 32) / 32) - 1;
    n2_bytelen = ((n2_bitlen - 1) / 32) * 4;
    n_tmp[n2_bytelen / 4] |= ((unsigned int)1) << ((n2_bitlen - 1) & 0x1f);

    while (n2_bitlen > 0) {
        n2_bitlen--;
        shl_bignum(n_tmp, 1, len);
        if (cmp_bignum(n_tmp, n2, len) != -1) {
            sub_bignum(n_tmp, n_tmp, n2, 0, len);
            *n1 |= bit;
        }
        bit >>= 1;
        if (bit == 0) {
            n1--;
            bit = 0x80000000;
        }
    }
    init_bignum(n_tmp, 0, len);
}

void inc_bignum(bignum n, unsigned int len)
{
    while ((++*n == 0) && (--len > 0))
        n++;
}

void init_two_dw(bignum n, unsigned int len)
{
    mov_bignum(glob1, n, len);
    glob1_bitlen = bitlen_bignum(glob1, len);
    glob1_len_x2 = (glob1_bitlen + 15) / 16;
    mov_bignum(glob1_hi, glob1 + len_bignum(glob1, len) - 2, 2);
    glob1_hi_bitlen = bitlen_bignum(glob1_hi, 2) - 32;
    shr_bignum(glob1_hi, glob1_hi_bitlen, 2);
    inv_bignum(glob1_hi_inv, glob1_hi, 2);
    shr_bignum(glob1_hi_inv, 1, 2);
    glob1_hi_bitlen = (glob1_hi_bitlen + 15) % 16 + 1;
    inc_bignum(glob1_hi_inv, 2);
    if (bitlen_bignum(glob1_hi_inv, 2) > 32) {
        shr_bignum(glob1_hi_inv, 1, 2);
        glob1_hi_bitlen--;
    }
    glob1_hi_inv_lo = *(unsigned short *)glob1_hi_inv;
    glob1_hi_inv_hi = *(((unsigned short *)glob1_hi_inv) + 1);
}

void mul_bignum_word(bignum n1, bignum n2, unsigned int mul, unsigned int len)
{
    unsigned int i, tmp;

    tmp = 0;
    for (i = 0; i < len; i++) {
        tmp = mul * (*(unsigned short *)n2) + *(unsigned short *)n1 + tmp;
        *(unsigned short *)n1 = (unsigned short)tmp;
        n1 = (unsigned int *)(((unsigned short *)n1) + 1);
        n2 = (unsigned int *)(((unsigned short *)n2) + 1);
        tmp >>= 16;
    }
    *(unsigned short *)n1 += (unsigned short)tmp;
}

void mul_bignum(bignum dest, bignum src1, bignum src2, unsigned int len)
{
    unsigned int i;

    init_bignum(dest, 0, len*2);
    for (i = 0; i < len*2; i++) {
        mul_bignum_word(dest, src1, *(unsigned short *)src2, len*2);
        src2 = (unsigned int *)(((unsigned short *)src2) + 1);
        dest = (unsigned int *)(((unsigned short *)dest) + 1);
    }
}

void not_bignum(bignum n, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++) {
        *n = ~*n;
        ++n;
    }
}

void neg_bignum(bignum n, unsigned int len)
{
    not_bignum(n, len);
    inc_bignum(n, len);
}

unsigned int get_mulword(bignum n)
{
    unsigned int i;
    unsigned short *wn;

    wn = (unsigned short *)n;
    i = (((((((((*(wn-1) ^ 0xffff) & 0xffff) * glob1_hi_inv_lo + 0x10000) >> 1)
             + (((*(wn-2) ^ 0xffff) * glob1_hi_inv_hi + glob1_hi_inv_hi) >> 1) + 1)
            >> 16) + ((((*(wn-1) ^ 0xffff) & 0xffff) * glob1_hi_inv_hi) >> 1) +
           (((*wn ^ 0xffff) * glob1_hi_inv_lo) >> 1) + 1) >> 14) + glob1_hi_inv_hi
         * (*wn ^ 0xffff) * 2) >> glob1_hi_bitlen;
    if (i > 0xffff)
        i = 0xffff;
    return i & 0xffff;
}

void dec_bignum(bignum n, unsigned int len)
{
    while ((--*n == 0xffffffff) && (--len > 0))
        n++;
}

void calc_a_bignum(bignum n1, bignum n2, bignum n3, unsigned int len)
{
    unsigned int g2_len_x2, len_diff;
    unsigned short *esi, *edi;
    unsigned short tmp;

    mul_bignum(glob2, n2, n3, len);
    glob2[len*2] = 0;
    g2_len_x2 = len_bignum(glob2, len*2+1)*2;
    if (g2_len_x2 >= glob1_len_x2) {
        inc_bignum(glob2, len*2+1);
        neg_bignum(glob2, len*2+1);
        len_diff = g2_len_x2 + 1 - glob1_len_x2;
        esi = ((unsigned short *)glob2) + (1 + g2_len_x2 - glob1_len_x2);
        edi = ((unsigned short *)glob2) + (g2_len_x2 + 1);
        for (; len_diff != 0; len_diff--) {
            edi--;
            tmp = (unsigned short)get_mulword((unsigned int *)edi);
            esi--;
            if (tmp > 0) {
                mul_bignum_word((unsigned int *)esi, glob1, tmp, 2*len);
                if ((*edi & 0x8000) == 0) {
                    if (sub_bignum((unsigned int *)esi, (unsigned int *)esi, glob1, 0, len))
                        (*edi)--;
                }
            }
        }
        neg_bignum(glob2, len);
        dec_bignum(glob2, len);
    }
    mov_bignum(n1, glob2, len);
}

void clear_tmp_vars(unsigned int len)
{
    init_bignum(glob1, 0, len);
    init_bignum(glob2, 0, len);
    init_bignum(glob1_hi_inv, 0, 4);
    init_bignum(glob1_hi, 0, 4);
    glob1_bitlen = 0;
    glob1_hi_bitlen = 0;
    glob1_len_x2 = 0;
    glob1_hi_inv_lo = 0;
    glob1_hi_inv_hi = 0;
}

void calc_a_key(bignum n1, bignum n2, bignum n3, bignum n4, unsigned int len)
{
    bignum n_tmp;
    unsigned int n3_len, n4_len, n3_bitlen, bit_mask;

    init_bignum(n1, 1, len);
    n4_len = len_bignum(n4, len);
    init_two_dw(n4, n4_len);
    n3_bitlen = bitlen_bignum(n3, n4_len);
    n3_len = (n3_bitlen + 31) / 32;
    bit_mask = (((unsigned int)1) << ((n3_bitlen - 1) % 32)) >> 1;
    n3 += n3_len - 1;
    n3_bitlen--;
    mov_bignum(n1, n2, n4_len);
    while ((long)--n3_bitlen != -1) {
        if (bit_mask == 0) {
            bit_mask = 0x80000000;
            n3--;
        }
        calc_a_bignum(n_tmp, n1, n1, n4_len);
        if (*n3 & bit_mask)
            calc_a_bignum(n1, n_tmp, n2, n4_len);
        else
            mov_bignum(n1, n_tmp, n4_len);
        bit_mask >>= 1;
    }
    init_bignum(n_tmp, 0, n4_len);
    clear_tmp_vars(len);
}

void process_predata(const unsigned char* pre, unsigned int pre_len, unsigned char *buf)
{
    bignum n2, n3;
    const unsigned int a = (pubkey.len - 1) / 8;
    while (a + 1 <= pre_len) {
        init_bignum(n2, 0, 64);
        memmove(n2, pre, a + 1);
        calc_a_key(n3, n2, pubkey.key2, pubkey.key1, 64);

        memmove(buf, n3, a);

        pre_len -= a + 1;
        pre += a + 1;
        buf += a;
    }
}

void get_blowfish_key(const unsigned char* s, unsigned char* d)
{
    static bool public_key_initialized = false;
    if (!public_key_initialized) {
        init_pubkey();
        public_key_initialized = true;
    }
    unsigned char key[256];
    process_predata(s, len_predata(), key);
    memcpy(d, key, 56);
}
