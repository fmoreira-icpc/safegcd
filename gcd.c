#define _CRT_RAND_S
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <immintrin.h>

#include "check.h"
#include "xorshift.h"

inline int64_t i64_abs(int64_t x) {
    int64_t a = x - ((x >> 63) & (x << 1));

    // post-condition: a >= 0 || a == INT64_MIN
    return a;
}

inline void i64_sort2(int64_t *x, int64_t *y) {
    // precondition: *x >= 0 && *y >= 0

    int64_t d = *y - *x;
    d = (d >> 63) & d;
    *x += d;
    *y -= d;

    // post-condition: *x <= *y
}

inline int64_t i64_min(int64_t x, int64_t y) {
    // precondition: x >= 0 && y >= 0

    i64_sort2(&x, &y);

    // post-condition: <return-value> <= x && <return-value> <= y
    return x;
}

inline int64_t i64_max(int64_t x, int64_t y) {
    // precondition: x >= 0 && y >= 0

    i64_sort2(&x, &y);

    // post-condition: <return-value> >= x && <return-value> >= y
    return y;
}

#if 1
// Technically the `& 63` is unnecessary, but this protects against UB.
inline int64_t i64_shl(int64_t x, int64_t e) {
    return x << (e & 63);
}

inline int64_t i64_shr(int64_t x, int64_t e) {
    return x >> (e & 63);
}
#else
inline int64_t i64_shl(int64_t x, int64_t e) {
    // TODO: barrel shifter
}

inline int64_t i64_shr(int64_t x, int64_t e) {
    // TODO: barrel shifter
}
#endif

#if 1
inline int64_t i64_extract_tz(int64_t *x) {
    int64_t e = _tzcnt_u64(*x);
    *x = i64_shr(*x, e);
    return e;
}
#else
// FIXME: x == 0
inline int64_t i64_extract_tz(int64_t *x) {
    int64_t e = 0;
    int64_t m = ~(int64_t) 0;
    for (int64_t i = 5; i >= 0; i--) {
        int64_t s = 1 << i;
        m = (uint64_t) m >> s;
        int64_t p = ~(-(*x & m) >> 63);
        e += p & s;
        *x ^= p & (*x ^ (*x >> s));
    }
    return e;
}
#endif

int64_t i64_gcd_unsafe_euclid(int64_t a, int64_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;

    while (b != 0L) {
        int64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// cf. https://gcd.cr.yp.to/
int64_t i64_gcd_unsafe_djb(int64_t a, int64_t b) {
    // Ensure that a, b are positive.
    a = i64_abs(a);
    b = i64_abs(b);

    // If possible, get rid of INT64_MIN.
    a -= (a >> 63) & (a ^ b ^ INT64_MIN);
    b -= (b >> 63) & (a ^ b ^ INT64_MIN);

    // Extract even factor and ensure a is odd.
    int64_t e = i64_min(i64_extract_tz(&a), i64_extract_tz(&b));

    // Protect against int64_t overflow.
    i64_sort2(&b, &a);

    int64_t d = 1;
    while (b != 0) {
        int64_t s = -d >> 63;
        int64_t z = -(b & 1);
        int64_t q = s & z;
        int64_t x = q & (a ^ b);
        int64_t y = z & (a - (s & a << 1));
        d = 1 + d - (q & d << 1);
        a ^= x;
        b += y;
        b >>= 1;
    }

    return i64_shl(i64_abs(a), e);
}

const int N = 1 << 18;

int64_t A[N];
int64_t B[N];

void test() {
    CHECK(i64_gcd_unsafe_djb(0, 0) == 0);

    CHECK(i64_gcd_unsafe_djb(0, INT64_MIN) == INT64_MIN);
    CHECK(i64_gcd_unsafe_djb(INT64_MIN, 0) == INT64_MIN);
    CHECK(i64_gcd_unsafe_djb(INT64_MIN, INT64_MIN) == INT64_MIN);

    CHECK(i64_gcd_unsafe_djb(0, INT64_MAX) == INT64_MAX);
    CHECK(i64_gcd_unsafe_djb(INT64_MAX, 0) == INT64_MAX);
    CHECK(i64_gcd_unsafe_djb(INT64_MAX, INT64_MAX) == INT64_MAX);

    CHECK(i64_gcd_unsafe_djb(INT64_MAX, INT64_MIN) == 1);
    CHECK(i64_gcd_unsafe_djb(INT64_MIN, INT64_MAX) == 1);

    {
        int64_t factor[] = {1, 7, 73, 127, 337, 92737, 649657};
        for (int i = 0; i < 7; i++) {
            int64_t f = factor[i];
            CHECK(i64_gcd_unsafe_djb(INT64_MAX, f) == f);
            CHECK(i64_gcd_unsafe_djb(f, INT64_MAX) == f);
            CHECK(i64_gcd_unsafe_djb(INT64_MAX, INT64_MAX - f) == f);
            CHECK(i64_gcd_unsafe_djb(INT64_MAX - f, INT64_MAX) == f);
        }
    }

    {
        int64_t f = 1;
        for (int i = 0; i < 63; i++) {
            CHECK(i64_gcd_unsafe_djb(f, INT64_MIN) == f);
            CHECK(i64_gcd_unsafe_djb(INT64_MIN, f) == f);
            f <<= 1;
        }
    }
    // {
    //     for (int i = 0; i < N; i++) {
    //         int64_t gA = i64_gcd_unsafe_djb(A[i], 0);
    //         int64_t gB = i64_gcd_unsafe_djb(0, B[i]);
    //         CHECK(gA == A[i]);
    //         CHECK(gB == B[i]);
    //     }
    //     for (int i = 0; i < N; i++) {
    //         int64_t gA = i64_gcd_unsafe_djb(A[i], 0x8000000000000000);
    //         int64_t gB = i64_gcd_unsafe_djb(0x8000000000000000, B[i]);
    //         CHECK(gA == (int64_t) 1 << i64_tz_count(A[i]));
    //         CHECK(gB == (int64_t) 1 << i64_tz_count(B[i]));
    //     }
    // }
}

void benchmark() {
    for (int h = 0; h < (1 << 8); h++) {
        for (int i = 0; i < N; i++) {
            uint64_t u = XS_next();
            uint64_t v = XS_next();
            A[i] = u;
            B[i] = v;
        }
        for (int i = 0; i < N; i++) {
            int64_t g0 = i64_gcd_unsafe_djb(A[i], B[i]);

            // Prevent the function call from being optimized away.
            CHECK(g0 != -1);
        }
    }
}

int main() {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    XS_init();

    test();
    benchmark();
    return 0;
}