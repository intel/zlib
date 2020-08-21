/*
 * AVX2 optimized hash slide
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#include "deflate.h"

#ifdef USE_AVX2_SLIDE
#include <immintrin.h>

void slide_hash_avx2(deflate_state *s)
{
    unsigned n;
    Posf *p;
    uInt wsize = s->w_size;
    z_const __m256i ymm_wsize = _mm256_set1_epi16(s->w_size);

    n = s->hash_size;
    p = &s->head[n] - 16;
    do {
        __m256i value, result;

        value = _mm256_loadu_si256((__m256i *)p);
        result= _mm256_subs_epu16(value, ymm_wsize);
        _mm256_storeu_si256((__m256i *)p, result);
        p -= 16;
        n -= 16;
    } while (n > 0);

#ifndef FASTEST
    n = wsize;
    p = &s->prev[n] - 16;
    do {
        __m256i value, result;

        value = _mm256_loadu_si256((__m256i *)p);
        result= _mm256_subs_epu16(value, ymm_wsize);
        _mm256_storeu_si256((__m256i *)p, result);

        p -= 16;
        n -= 16;
    } while (n > 0);
#endif
}

#endif
