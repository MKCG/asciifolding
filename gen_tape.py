#!python

lines = []

with open('./ASCIIFoldingFilter.java') as file:
    while True:
        line = file.readline()

        if not line:
            break

        line = line.strip()

        if line.startswith('output[outputPos++]') or line.startswith('case ') or line.startswith('break'):
            lines.append(line)

def map_unicode_to_ascii():
    hex_mapping = {
        '0': 0,
        '1': 1,
        '2': 2,
        '3': 3,
        '4': 4,
        '5': 5,
        '6': 6,
        '7': 7,
        '8': 8,
        '9': 9,
        'A': 10,
        'B': 11,
        'C': 12,
        'D': 13,
        'E': 14,
        'F': 15
    }

    unicodes = []
    asciis = []

    for i in range(127):
        yield chr(i), i.to_bytes(), chr(i)

    for line in lines:
        if line.startswith('case '):
            unicode = 0

            for char in line.split("'")[1][2:]:
                unicode <<= 4
                unicode += hex_mapping[char]

            unicodes.append(unicode.to_bytes(4).decode('utf-16be'))

        if line.startswith('output[outputPos++]'):
            if len(unicodes) != 0:
                asciis.append(line.split("'")[1])

        if line == 'break;':
            for unicode in unicodes:
                encoded = unicode.encode('utf-8')

                while len(encoded) > 1 and encoded[0] == 0:
                    encoded = encoded[1:]

                yield unicode, encoded, ''.join(asciis)

            unicodes = []
            asciis = []

def is_valid_lut(lut_size, w):
    lut = [ None for i in range(lut_size) ]
    invalid = False
    processed = 0

    for unicode, encoded, asciis in map_unicode_to_ascii():
        idx = 5381

        if len(encoded) == 1:
            idx = (idx * w) + ord(unicode)
        if len(encoded) == 2:
            idx = (idx * w) + encoded[0]
            idx = (idx * w) + encoded[1]
        if len(encoded) == 3:
            idx = (idx * w) + encoded[0]
            idx = (idx * w) + encoded[1]
            idx = (idx * w) + encoded[2]

        idx = idx % lut_size
        processed += 1

        if lut[idx] is None:
            lut[idx] = asciis
        elif lut[idx] != asciis:
            return False

    return True

def optimize_hash_params():
    return 3242, 64

    from math import ceil

    matches = []
    min_size = len([ _ for _ in map_unicode_to_ascii() ]) * 2
    best_match = min_size * 100

    for w in range(513):
        step = ceil(min_size / 2)
        max_size = best_match
        previously_found = False

        while 1:
            print(w, max_size, step)
            found = False

            for lut_size in range(min_size, max_size, step):
                if is_valid_lut(lut_size, w):
                    max_size = lut_size
                    found = True
                    previously_found = True
                    matches.append([lut_size, w])

                    if lut_size < best_match:
                        best_match = lut_size

                    break

            if step == 1 or (previously_found is False and best_match == min_size * 100):
                break

            step = ceil(step / 2)

    matches.sort(key=lambda x: x[0])

    return matches[0]

def create_tape(tape_size, w):
    tape = [ 0 for _ in range((tape_size * 5) + 256) ]

    for i in range(128, 256):
        tape[i - 128] = 1
        tape[i - 127] = i

    for unicode, encoded, asciis in map_unicode_to_ascii():
        idx = 5381

        if len(encoded) == 1:
            idx = (idx * w) + ord(unicode)
        if len(encoded) == 2:
            idx = (idx * w) + encoded[0]
            idx = (idx * w) + encoded[1]
        if len(encoded) == 3:
            idx = (idx * w) + encoded[0]
            idx = (idx * w) + encoded[1]
            idx = (idx * w) + encoded[2]

        idx = idx % tape_size
        idx *= 5
        idx += 256

        tape[idx] = len(asciis)
        idx += 1

        for i in range(len(asciis)):
            tape[idx] = ord(asciis[i])
            idx += 1

    return tape

size, w = optimize_hash_params()
tape = create_tape(size, w)

import json

print(f'''#ifndef ASCIIFOLDING_TAPE_H
#define ASCIIFOLDING_TAPE_H

#define ASCIIFOLDING_HASH_TABLE_SIZE {size}
#define ASCIIFOLDING_HASH_WEIGHT {w}

unsigned char ascii_tape[{len(tape)}] = {{
    {json.dumps(tape)[1:-1]}
}};

#endif''')
