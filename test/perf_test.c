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

void main() {
    x86_check_features();
    if (x86_cpu_has_sse2 && x86_cpu_has_avx2)
        test_slide_hash();
}
#else
void main() {}
#endif