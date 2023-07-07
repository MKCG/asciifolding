/**
 * Author : Kévin Masseix - masseix.kevin@gmail.com
 */

#ifndef ASCIIFOLDING_FILTER_H
#define ASCIIFOLDING_FILTER_H

unsigned int lut_char_lengths_utf8[256] = {
    // [0:127] ascii
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    // [128:191] invalid
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    // [192:223]
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,

    // [224:239]
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,

    // [240:247]
    4, 4, 4, 4, 4, 4, 4, 4,

    // [248:251] invalid
    0, 0, 0, 0,

    // [252:253] invalid
    0, 0,

    // [254] invalid
    0,

    // [255] invalid
    0
};

#include "asciifolding_tape.h"

#if defined(ASCIIFOLDING_IS_BRANCHY)
    #if defined(__AVX2__) && defined(ASCIIFOLDING_USE_SIMD)
        #include <immintrin.h>
    #else
        #include <string.h>
    #endif
#endif

/**
 * Convert an UTF-8 encoded string into an ASCII encoded string using the same rules as defined by the ASCII folding filter in Lucene
 *
 * @param  input_utf8   UTF-8 encoded string, an extra 3 bytes must be allocated at the end
 * @param  input_length
 * @param  output_ascii ASCII encoded string
 * @return              Length of the ascii encoded string
 */
unsigned int asciifolding(const unsigned char * input_utf8, unsigned int input_length, unsigned char * output_ascii) {
    unsigned int ascii_length = 0;
    unsigned int i = 0;

    while (i < input_length) {
        #ifdef ASCIIFOLDING_IS_BRANCHY
            #if defined(__AVX2__) && defined(ASCIIFOLDING_USE_SIMD)
                if (i + 32 < input_length) {
                    __m256i mask     = _mm256_set1_epi8(0x80);
                    __m256i vector   = _mm256_loadu_si256((__m256i *) &input_utf8[i]);
                    __m256i masked   = _mm256_and_si256(vector, mask);

                    int is_not_ascii = _mm256_movemask_epi8(masked);
                    int is_ascii     = __builtin_ctz(is_not_ascii);

                    if (is_ascii == 32) {
                        _mm256_storeu_si256((__m256i *) output_ascii, vector);
                        i += 32;
                        output_ascii += 32;
                        ascii_length += 32;
                        continue;
                    } else {
                        for (int j = 0; j < is_ascii; j++) {
                            *(output_ascii++) = input_utf8[i++];
                        }

                        ascii_length += is_ascii;
                        goto process_non_ascii;
                    }
                }
            #else
                if (sizeof(unsigned long long) == 8 && i + 8 < input_length && (*((unsigned long long *) &input_utf8[i]) & 0x8080808080808080) == 0) {
                    memcpy(output_ascii, &input_utf8[i], 8);
                    // *((unsigned long long*) output_ascii) = *((unsigned long long *) &input_utf8[i]);
                    i += 8;
                    output_ascii += 8;
                    ascii_length += 8;
                    continue;
                }
            #endif

            if (input_utf8[i] < 128) {
                *(output_ascii++) = input_utf8[i++];
                ascii_length++;
            } else {
                process_non_ascii: {
                    unsigned int char_length_utf8 = lut_char_lengths_utf8[input_utf8[i]];

                    unsigned long long index = 5381;

                    switch (char_length_utf8) {
                        case 4: index = (index * ASCIIFOLDING_HASH_WEIGHT) + input_utf8[i++];
                        case 3: index = (index * ASCIIFOLDING_HASH_WEIGHT) + input_utf8[i++];
                        case 2: index = (index * ASCIIFOLDING_HASH_WEIGHT) + input_utf8[i++];
                        case 1: index = (index * ASCIIFOLDING_HASH_WEIGHT) + input_utf8[i++];
                    }

                    index = ((index % ASCIIFOLDING_HASH_TABLE_SIZE) * 5) + 256;

                    unsigned char char_length_ascii = ascii_tape[index];
                    ascii_length += char_length_ascii;

                    unsigned char *replacement = &ascii_tape[index + 1];

                    switch (char_length_ascii) {
                        case 4: *(output_ascii++) = *(replacement++);
                        case 3: *(output_ascii++) = *(replacement++);
                        case 2: *(output_ascii++) = *(replacement++);
                        case 1: *(output_ascii++) = *(replacement++);
                    }
                }
            }
        #else
            unsigned int char_length_utf8 = lut_char_lengths_utf8[input_utf8[i]];
            unsigned int char_is_valid = char_length_utf8 > 0;
            unsigned int rehash = 0;

            unsigned long long index = 5381;
            int invalid_index = (input_utf8[i] - 128) * 2;

            // 1st byte
            index = (index * ASCIIFOLDING_HASH_WEIGHT) + input_utf8[i];
            i++;

            // 2nd byte
            char_is_valid &= ((input_utf8[i] & 192) == 128) | (char_length_utf8 < 2);
            rehash = char_length_utf8 > 1;
            index = (index * (1 + ((ASCIIFOLDING_HASH_WEIGHT - 1) * rehash))) + (input_utf8[i] * rehash);
            i += char_length_utf8 > 1;

            // 3nd byte
            char_is_valid &= ((input_utf8[i] & 192) == 128) | (char_length_utf8 < 3);
            rehash = char_length_utf8 > 2;
            index = (index * (1 + ((ASCIIFOLDING_HASH_WEIGHT - 1) * rehash))) + (input_utf8[i] * rehash);
            i += char_length_utf8 > 2;

            // 4th byte
            char_is_valid &= ((input_utf8[i] & 192) == 128) | (char_length_utf8 < 4);
            rehash = char_length_utf8 > 3;
            index = (index * (1 + ((ASCIIFOLDING_HASH_WEIGHT - 1) * rehash))) + (input_utf8[i] * rehash);
            i += char_length_utf8 > 3;

            // tape access
            index = ((index % ASCIIFOLDING_HASH_TABLE_SIZE) * 5) + 256;
            index = (index * char_is_valid) + (invalid_index * (char_is_valid == 0));

            unsigned int char_length_ascii = (unsigned int) ascii_tape[index];
            ascii_length += char_length_ascii;

            // output
            index++;

            *output_ascii = ascii_tape[index];
            output_ascii++;

            index += char_length_ascii > 1;
            *output_ascii = ascii_tape[index];
            output_ascii += char_length_ascii > 1;

            index += char_length_ascii > 2;
            *output_ascii = ascii_tape[index];
            output_ascii += char_length_ascii > 2;

            index += char_length_ascii > 3;
            *output_ascii = ascii_tape[index];
            output_ascii += char_length_ascii > 3;
        #endif
    }

    *output_ascii = 0;

    return ascii_length;
}

#endif
