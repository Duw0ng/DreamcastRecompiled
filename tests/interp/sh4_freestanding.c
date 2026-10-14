/* Minimal libc pieces GCC may call implicitly from interp_corpus.c (SH-4 build only). */
typedef unsigned int size_t;
void* memcpy(void* d, const void* s, size_t n) { unsigned char* o = d; const unsigned char* i = s; while (n--) *o++ = *i++; return d; }
void* memmove(void* d, const void* s, size_t n) {
    unsigned char* o = d; const unsigned char* i = s;
    if (o < i) { while (n--) *o++ = *i++; } else { o += n; i += n; while (n--) *--o = *--i; }
    return d;
}
void* memset(void* d, int c, size_t n) { unsigned char* o = d; while (n--) *o++ = (unsigned char)c; return d; }
int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* x = a; const unsigned char* y = b;
    for (; n; --n, ++x, ++y) if (*x != *y) return *x - *y;
    return 0;
}
