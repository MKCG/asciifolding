/**
 * Author   : Kévin Masseix
 * Email    : masseix.kevin@gmail.com
 * Github   : https://github.com/MKCG/
 * LinkedIn : https://www.linkedin.com/in/k%C3%A9vin-m-228a328b/
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
 * Important :
 *     it is possible for a valid UTF8 character to be wrongfully mapped into incorrect ASCII characters
 *     if the UTF8 character is not handled by the ASCII folding filter from Lucene
 *
 * @param  input_utf8   UTF-8 encoded string, an extra 3 bytes must be allocated at the end
 * @param  input_length
 * @param  output_ascii ASCII encoded string
 *
 * @return              Length of the ascii encoded string
 */
unsigned long long asciifolding(const unsigned char * input_utf8, unsigned long long input_length, unsigned char * output_ascii) {
    unsigned long long ascii_length = 0;

    #ifdef ASCIIFOLDING_IS_BRANCHY
        const unsigned char * tail_utf8 = input_utf8 + input_length;
        const unsigned char * head_ascii = output_ascii;

        while (input_utf8 < tail_utf8) {
            #if defined(__AVX2__) && defined(ASCIIFOLDING_USE_SIMD)
                if (tail_utf8 - input_utf8 >= 32) {
                    __m256i mask    = _mm256_set1_epi8(0x80);
                    __m256i vector  = _mm256_loadu_si256((__m256i *) input_utf8);
                    __m256i masked  = _mm256_and_si256(vector, mask);

                    int multibytes_length = _mm256_movemask_epi8(masked);
                    int nb_leading_ascii  = _tzcnt_u32(multibytes_length);

                    if (nb_leading_ascii == 32) {
                        _mm256_storeu_si256((__m256i *) output_ascii, vector);
                        input_utf8 += 32;
                        output_ascii += 32;
                        continue;
                    } else {
                        while (nb_leading_ascii >= 4) {
                            *((unsigned int*) output_ascii) = *((unsigned int*) input_utf8);
                            input_utf8 += 4;
                            output_ascii += 4;
                            nb_leading_ascii -= 4;
                        }

                        while (nb_leading_ascii-- > 0) {
                            *(output_ascii++) = *(input_utf8++);
                        }

                        goto process_non_ascii;
                    }
                }
            #else
                if (sizeof(unsigned long long) == 8 && tail_utf8 - input_utf8 >= 8 && (*((unsigned long long *) input_utf8) & 0x8080808080808080) == 0) {
                    memcpy(output_ascii, input_utf8, 8);
                    // *((unsigned long long*) output_ascii) = *((unsigned long long *) input_utf8);
                    input_utf8   += 8;
                    output_ascii += 8;
                    continue;
                }
            #endif

            if (*input_utf8 < 128) {
                *(output_ascii++) = *(input_utf8++);
            } else {
                process_non_ascii: {
                    unsigned long long index = 5381;
                    unsigned int char_length_utf8 = lut_char_lengths_utf8[*input_utf8];

                    switch (char_length_utf8) {
                        case 4: index = (index * ASCIIFOLDING_HASH_WEIGHT) + *(input_utf8++);
                        case 3: index = (index * ASCIIFOLDING_HASH_WEIGHT) + *(input_utf8++);
                        case 2: {
                            index = (index * ASCIIFOLDING_HASH_WEIGHT) + *(input_utf8++);
                            index = (index * ASCIIFOLDING_HASH_WEIGHT) + *(input_utf8++);
                        }
                    }

                    index = ((index % ASCIIFOLDING_HASH_TABLE_SIZE) * 9) + 256;

                    unsigned char * block = &ascii_tape[index];

                    int is_same = char_length_utf8 > 0;

                    switch (char_length_utf8) {
                        case 4: is_same &= *(block++) == *(input_utf8 - 4);
                        case 3: is_same &= *(block++) == *(input_utf8 - 3);
                        case 2: {
                            is_same &= *(block++) == *(input_utf8 - 2);
                            is_same &= *(block++) == *(input_utf8 - 1);
                        }
                    }

                    unsigned char char_length_ascii;
                    unsigned char * replacement;

                    if (is_same) {
                        char_length_ascii = *(block++);
                        replacement = block;
                    } else {
                        char_length_ascii = char_length_utf8 + (char_length_utf8 == 0);
                        replacement = input_utf8 - char_length_utf8;
                    }

                    switch (char_length_ascii) {
                        case 4: *(output_ascii++) = *(replacement++);
                        case 3: *(output_ascii++) = *(replacement++);
                        case 2: *(output_ascii++) = *(replacement++);
                        case 1: *(output_ascii++) = *(replacement++);
                    }
                }
            }
        }

        return output_ascii - head_ascii;
    #else
        unsigned int i = 0;

        while (i < input_length) {
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
            index = ((index % ASCIIFOLDING_HASH_TABLE_SIZE) * 9) + 256;
            index = (index * char_is_valid) + (invalid_index * (char_is_valid == 0));

            unsigned int char_length_ascii = (unsigned int) ascii_tape[index];
            ascii_length += char_length_ascii;

            // output
            index++;

            // @todo : detect any collision
            index += char_length_utf8;

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
        }

        *output_ascii = 0;
    #endif

    return ascii_length;
}

#endif
