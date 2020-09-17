#include "zlib.h"
#include "zconf.h"
#include "../deflate.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#if defined(ZLIB_X86)
#include "../x86.h"

extern void slide_hash_sse(deflate_state *s);
extern void slide_hash_avx2(deflate_state *s);

#define ALLOC(n, m) calloc(n, m)
#define FREE(p) free(p)

deflate_state* alloc_deflate_state()
{
    int i;
    deflate_state* s;
    s = (deflate_state *) ALLOC(1, sizeof(deflate_state));
    s->w_size = 1<<15;
    s->hash_size = 1<<15;
    s->prev   = (Posf *)  ALLOC(s->w_size, sizeof(Pos));
    s->head   = (Posf *)  ALLOC(s->hash_size, sizeof(Pos));
    // fill the buf
    for (i = 0; i < s->w_size; i++) {
        s->prev[i] = 123 + i;
    }
    for (i = 0; i < s->hash_size; i++) {
        s->head[i] = 456 + i;
    }
    return s;
}

void free_deflate_state(s)
    deflate_state *s;
{
    s = (deflate_state *) ALLOC(1, sizeof(deflate_state));
    FREE(s->head);
    FREE(s->prev);
    FREE(s);
}

void test_slide_hash() {
    int i;
    deflate_state *sse, *avx2;
    clock_t begin, end;

    printf("slide_hash unit test:\n");
    sse = alloc_deflate_state();
    avx2 = alloc_deflate_state();
    slide_hash_sse(sse);
    slide_hash_avx2(avx2);
    // check the result sanity
    for (i =0; i < sse->w_size; i++) {
        assert(sse->prev[i] == avx2->prev[i]);
    }
    for (i = 0; i < sse->hash_size; i++) {
        assert(sse->head[i] == avx2->head[i]);
    }
    // check the performance
    #define TEST_LOOP 10000
    begin = clock();
    for (i = 0; i < TEST_LOOP; i++) slide_hash_sse(sse);
    end = clock();
    printf("SSE time: %.2fms\n", (end - begin)/1000.0);
    begin = clock();
    for (i = 0; i < TEST_LOOP; i++) slide_hash_avx2(avx2);
    end = clock();
    printf("AVX2 time: %.2fms\n", (end - begin)/1000.0);

    free_deflate_state(sse);
    free_deflate_state(avx2);
}

extern void crc_fold_init(unsigned crc[4 * 5]);
extern void crc_fold_copy(unsigned crc[4 * 5], unsigned char *dst, const unsigned char *src, long len);
extern unsigned crc_fold_512to32(unsigned crc[4 * 5]);

void test_crc_fold_copy() {
    unsigned crc0[4 * 5];
    unsigned crc0_v[4 * 5];
    unsigned char *src, *dst, *src_v, *dst_v;
    size_t i;
    const size_t size = 1024 * 16 + 255;
    clock_t begin, end;
    printf("crc_fold_copy testing:\n");

    src = (unsigned char *) ALLOC(size, sizeof(char));
    src_v = (unsigned char *) ALLOC(size, sizeof(char));
    dst = (unsigned char *) ALLOC(size, sizeof(char));
    dst_v = (unsigned char *) ALLOC(size, sizeof(char));

    for (i = 0; i < 4 * 5; i++) {
        crc0[i] = 0;
        crc0_v[i] = 0;
    }

    for (i = 0; i < size; i++) { // fill src buf
        src[i] = i & 0xFF;
        src_v[i] = i & 0xFF;
    }

    /* sanity check */
    x86_cpu_has_vpclmulqdq = 0;  // PCLMUL mode
    crc_fold_init(crc0);
    crc_fold_copy(crc0, dst, src, size);
    x86_cpu_has_vpclmulqdq = 1;  // VPCLMULQDQ mode
    crc_fold_init(crc0_v);
    crc_fold_copy(crc0_v, dst_v, src_v, size);
    // check crc values
    assert(crc_fold_512to32(crc0) == crc_fold_512to32(crc0_v));
    // check copy
        for (i = 0; i < size; i++) {
        assert(dst[i] == dst_v[i]);
    }

    // mutiple call test
    size_t half = size/2;
    x86_cpu_has_vpclmulqdq = 0;  // PCLMUL mode
    crc_fold_init(crc0);
    crc_fold_copy(crc0, dst, src, size);
    crc_fold_copy(crc0, dst + half, src + half, size - half);
    x86_cpu_has_vpclmulqdq = 1;  // VPCLMULQDQ mode
    crc_fold_init(crc0_v);
    crc_fold_copy(crc0_v, dst_v, src_v, size);
    crc_fold_copy(crc0_v, dst_v + half, src_v + half, size - half);
    // check crc values
    assert(crc_fold_512to32(crc0) == crc_fold_512to32(crc0_v));

    // check the performance
    #define TEST_LOOP 10000
    x86_cpu_has_vpclmulqdq = 0;
    begin = clock();
    for (i = 0; i < TEST_LOOP; i++) crc_fold_copy(crc0, dst, src, size);
    end = clock();
    printf("PCLMUL time: %.2fms\n", (end - begin)/1000.0);

    x86_cpu_has_vpclmulqdq = 1;
    begin = clock();
    for (i = 0; i < TEST_LOOP; i++) crc_fold_copy(crc0_v, dst_v, src_v, size);
    end = clock();
    printf("VPCLMULQDQ time: %.2fms\n", (end - begin)/1000.0);

    FREE(src);
    FREE(dst);
    FREE(src_v);
    FREE(dst_v);
}

void main() {
    x86_check_features();
    if (x86_cpu_has_sse2 && x86_cpu_has_avx2)
        test_slide_hash();
    if (x86_cpu_has_vpclmulqdq)
        test_crc_fold_copy();
}
#else
void main() {}
#endif