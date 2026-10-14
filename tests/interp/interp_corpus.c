/*
 * Differential corpus for the SH-4 interpreter fallback.
 *
 * The same source is compiled twice:
 *   - natively into tests_sh4_interp (host reference results), and
 *   - with an SH-4 GCC (-ml -m4, little endian) into a raw image that the test
 *     executes through call_recompiled() -> interpreter.
 * Both builds must produce bit-identical out[] arrays.
 *
 * Rules that keep host (LP64) and SH-4 (ILP32) results identical:
 *   - only fixed-width typedefs below, never long or pointer arithmetic results;
 *   - no undefined behaviour (signed overflow, INT_MIN/-1, oversize shifts);
 *   - both sides compile with -ffp-contract=off so float math rounds the same.
 */

typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned long long u64;
typedef long long s64;

#define DC_NOINLINE __attribute__((noinline))
#define DC_COUNT 96

static u32 lcg_state;
static u32 lcg(void) { lcg_state = lcg_state * 1103515245u + 12345u; return lcg_state; }

static u32 rotl32(u32 v, u32 s) { s &= 31u; return s ? (v << s) | (v >> (32u - s)) : v; }

DC_NOINLINE u32 dc_square(u32 x) { return x * x + 1u; }
DC_NOINLINE u32 dc_mix(u32 a, u32 b) { return rotl32(a ^ (b * 0x9E3779B9u), b & 31u) + a; }
DC_NOINLINE u32 dc_apply(u32 (*fn)(u32), u32 x) { return fn(x) + fn(x + 1u) * 3u; }

DC_NOINLINE u32 arith_mix(u32 seed) {
    u32 h = seed;
    for (u32 i = 0; i < 2000u; ++i) {
        const u32 r = lcg();
        h ^= r + (h << 6) + (h >> 2);
        h = rotl32(h, r & 31u);
        if ((s32)h < 0) h -= r >> 3; else h += r << 1;
        h += (u32)(s32)(s16)(r >> 7);
        h ^= (u32)(s32)(s8)(r >> 13);
    }
    return h;
}

DC_NOINLINE u32 div_mix(void) {
    static const s32 a[] = { 0, 1, -1, 7, -7, 100, -100, 123456789, -123456789, 2147483647, -2147483647, 65535, -65536, 999983 };
    static const s32 b[] = { 1, -1, 2, -2, 3, 7, -7, 10, -10, 255, 65536, -65537, 1000003, 2147483647 };
    u32 h = 0x12345678u;
    for (u32 i = 0; i < sizeof(a) / sizeof(a[0]); ++i)
        for (u32 j = 0; j < sizeof(b) / sizeof(b[0]); ++j) {
            const s32 q = a[i] / b[j];
            const s32 r = a[i] % b[j];
            const u32 uq = (u32)a[i] / (u32)b[j];
            const u32 ur = (u32)a[i] % (u32)b[j];
            h = h * 31u + (u32)q;
            h = h * 31u + (u32)r;
            h = h * 31u + uq;
            h = h * 31u + ur;
        }
    return h;
}

DC_NOINLINE u64 wide_mix(u64 x) {
    u64 acc = 0x0123456789ABCDEFull;
    for (u32 i = 1; i < 300u; ++i) {
        const u32 y_hi = lcg();   /* sequenced: operand order of | is unspecified */
        const u64 y = ((u64)y_hi << 32) | lcg();
        acc ^= x * y + (acc >> 7);
        acc += (y / (u64)(i * 2654435761u | 1u)) ^ (acc << (i & 31u));
        acc ^= (u64)((s64)(y ^ acc) % (s64)(i * 7919 + 1));
        if ((s64)acc < (s64)y) acc = ~acc;
        x = acc ^ (x >> (i & 15u));
    }
    return acc;
}

DC_NOINLINE s32 big_switch(s32 v) {
    switch (v) {
        case 0: return v * 3 + 1;
        case 1: return lcg() & 0xFF;
        case 2: return v << 9;
        case 3: return -v;
        case 4: return (s32)dc_square((u32)v);
        case 5: return v ^ 0x5A5A;
        case 6: return v * v * v;
        case 7: return (s32)(lcg() >> 20);
        case 8: return 77;
        case 9: return v - 1000;
        case 10: return (s32)dc_mix((u32)v, 3u);
        case 11: return v | 0x100;
        case 12: return v & 3;
        case 13: return 13 * 13;
        default: return -1;
    }
}

DC_NOINLINE u32 fib(u32 n) { return n < 2u ? n : fib(n - 1u) + fib(n - 2u); }

DC_NOINLINE u32 ackermann(u32 m, u32 n) {
    if (m == 0u) return n + 1u;
    if (n == 0u) return ackermann(m - 1u, 1u);
    return ackermann(m - 1u, ackermann(m, n - 1u));
}

static void quicksort(s32* v, s32 lo, s32 hi) {
    while (lo < hi) {
        const s32 pivot = v[(lo + hi) / 2];
        s32 i = lo, j = hi;
        while (i <= j) {
            while (v[i] < pivot) ++i;
            while (v[j] > pivot) --j;
            if (i <= j) { const s32 t = v[i]; v[i] = v[j]; v[j] = t; ++i; --j; }
        }
        if (j - lo < hi - i) { quicksort(v, lo, j); lo = i; }
        else { quicksort(v, i, hi); hi = j; }
    }
}

DC_NOINLINE u32 sort_mix(void) {
    s32 values[257];
    for (u32 i = 0; i < 257u; ++i) values[i] = (s32)(lcg() >> 8) - 0x400000;
    quicksort(values, 0, 256);
    u32 h = 0u;
    for (u32 i = 0; i < 257u; ++i) {
        if (i && values[i - 1] > values[i]) return 0xDEADu;
        h = h * 33u ^ (u32)values[i];
    }
    return h;
}

typedef u32 (*op_fn)(u32, u32);
static u32 op_add(u32 a, u32 b) { return a + b; }
static u32 op_sub(u32 a, u32 b) { return a - b; }
static u32 op_xor(u32 a, u32 b) { return a ^ b; }
static u32 op_mul(u32 a, u32 b) { return a * b; }
static u32 op_mix(u32 a, u32 b) { return dc_mix(a, b); }

DC_NOINLINE u32 pointer_mix(void) {
    static const op_fn ops[] = { op_add, op_sub, op_xor, op_mul, op_mix };
    u32 h = 1u;
    for (u32 i = 0; i < 500u; ++i) h = ops[lcg() % 5u](h, i * 2654435761u);
    return h;
}

struct packed_like {
    u8 a;
    s8 b;
    u16 c;
    s16 d;
    u32 e;
    u32 flags : 5;
    u32 kind : 3;
    s32 delta : 9;
    u32 rest : 15;
};

DC_NOINLINE u32 struct_mix(void) {
    struct packed_like items[16];
    for (u32 i = 0; i < 16u; ++i) {
        const u32 r = lcg();
        items[i].a = (u8)r; items[i].b = (s8)(r >> 8); items[i].c = (u16)(r >> 3); items[i].d = (s16)(r >> 11);
        items[i].e = r ^ 0xA5A5A5A5u;
        items[i].flags = r & 31u; items[i].kind = (r >> 5) & 7u;
        items[i].delta = (s32)((r >> 9) & 511u) - 256; items[i].rest = r >> 17;
    }
    struct packed_like copy[16];
    for (u32 i = 0; i < 16u; ++i) copy[15u - i] = items[i];
    u32 h = 7u;
    for (u32 i = 0; i < 16u; ++i) {
        h = h * 131u + copy[i].a + (u32)(s32)copy[i].b + copy[i].c + (u32)(s32)copy[i].d;
        h ^= copy[i].e + copy[i].flags + (copy[i].kind << 4) + (u32)copy[i].delta + copy[i].rest;
    }
    return h;
}

DC_NOINLINE u32 crc32_buffer(const u8* data, u32 size) {
    u32 crc = 0xFFFFFFFFu;
    for (u32 i = 0; i < size; ++i) {
        crc ^= data[i];
        for (u32 k = 0; k < 8u; ++k) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

DC_NOINLINE u32 bytes_mix(void) {
    u8 buffer[512];
    for (u32 i = 0; i < sizeof(buffer); ++i) buffer[i] = (u8)(lcg() >> 16);
    u16 halves[64];
    for (u32 i = 0; i < 64u; ++i) halves[i] = (u16)(buffer[i * 2u] | (buffer[i * 2u + 1u] << 8));
    u32 h = crc32_buffer(buffer, sizeof(buffer));
    for (u32 i = 0; i < 64u; ++i) h += (u32)(s32)(s16)halves[i] * (i + 1u);
    return h;
}

static u32 float_bits(float f) { union { float f; u32 u; } v; v.f = f; return v.u; }
static u32 double_lo(double d) { union { double d; u64 u; } v; v.d = d; return (u32)v.u; }
static u32 double_hi(double d) { union { double d; u64 u; } v; v.d = d; return (u32)(v.u >> 32); }

DC_NOINLINE u32 float_mix(void) {
    float acc = 0.0f, dot = 0.0f;
    u32 h = 0u;
    for (u32 i = 0; i < 400u; ++i) {
        const float a = (float)(s32)(lcg() >> 16) / 1024.0f - 16.0f;
        const float b = (float)(lcg() >> 20) / 256.0f + 0.5f;
        acc = acc * 0.75f + a / b;
        dot += a * b;
        if (a > b) h ^= float_bits(a - b);
        if (b != 0.0f) h += float_bits(__builtin_sqrtf(b));
        h += (u32)(s32)(a * 37.0f);
        h ^= float_bits(-acc) + float_bits(__builtin_fabsf(a));
    }
    return h ^ float_bits(acc) ^ (float_bits(dot) << 1);
}

DC_NOINLINE u32 double_mix(void) {
    double acc = 1.0, dot = 0.0;
    u32 h = 0u;
    for (u32 i = 0; i < 300u; ++i) {
        const double a = (double)(s32)lcg() / 65536.0;
        const double b = (double)(lcg() >> 4) / 4096.0 + 0.25;
        acc = acc * 0.875 + a / b;
        dot += a * b;
        if (a < b) h += double_lo(b - a);
        h ^= double_hi(__builtin_sqrt(b)) + double_lo(__builtin_sqrt(b));
        const float narrowed = (float)a;
        const double widened = (double)narrowed * 3.0;
        h += float_bits(narrowed) ^ double_hi(widened);
        h += (u32)(s32)(a / 1024.0);
    }
    return h ^ double_hi(acc) ^ double_lo(dot);
}

DC_NOINLINE u32 conversion_mix(void) {
    u32 h = 0u;
    for (u32 i = 0; i < 200u; ++i) {
        const s32 si = (s32)lcg() >> (i & 15u);
        const float f = (float)si;
        const double d = (double)si * 1.5;
        h ^= float_bits(f) + (u32)(s32)(f * 0.5f) + double_lo(d) + (u32)(s32)(d / 3.0);
        h = rotl32(h, 5u);
    }
    return h;
}

DC_NOINLINE u32 long_loop(u32 iterations) {
    u32 a = 1u, b = 2u;
    for (u32 i = 0; i < iterations; ++i) {
        a += b ^ i;
        b = rotl32(b + a, 3u);
    }
    return a ^ b;
}

void dc_test_main(u32* out) {
    for (u32 i = 0; i < DC_COUNT; ++i) out[i] = 0u;
    lcg_state = 0xC0FFEE11u;
    out[0] = arith_mix(0x31415926u);
    out[1] = div_mix();
    {
        const u64 w = wide_mix(0xFEDCBA9876543210ull);
        out[2] = (u32)w;
        out[3] = (u32)(w >> 32);
    }
    for (s32 v = -1; v < 15; ++v) out[4 + (v + 1)] = (u32)big_switch(v);   /* out[4..19] */
    out[20] = fib(20u);
    out[21] = ackermann(2u, 3u);
    out[22] = sort_mix();
    out[23] = pointer_mix();
    out[24] = struct_mix();
    out[25] = bytes_mix();
    out[26] = float_mix();
    out[27] = double_mix();
    out[28] = conversion_mix();
    out[29] = long_loop(50000u);
    out[30] = dc_apply(dc_square, 7u);
    out[31] = dc_mix(3u, 4u) + dc_apply(dc_square, 100u);
    out[32] = 0x600DF00Du;
}
