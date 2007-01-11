#ifndef _LIB_BLOWFISH_H
#define _LIB_BLOWFISH_H

// Find better names for these
typedef unsigned int t_bf_p[18];
typedef unsigned int t_bf_s[4][256];

class Blowfish
{
public:
    Blowfish(const unsigned char* key, unsigned int keylen);

    void encipher(unsigned int& xl, unsigned int& xr) const;
    void encipher(const void* s, void* d, unsigned int size) const;
    void decipher(unsigned int& xl, unsigned int& xr) const;
    void decipher(const void* s, void* d, int size) const;
private:
    unsigned int S(unsigned int x, int i) const;
    unsigned int bf_f(unsigned int x) const;
    void ROUND(unsigned int& a, unsigned int b, int n) const;

    t_bf_p m_p;
    t_bf_s m_s;
};

#endif
