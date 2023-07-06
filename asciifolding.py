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
            idx = (idx * w) + (encoded[1] & 63)
            idx = (idx * w) + (encoded[0] & 31)
        if len(encoded) == 3:
            idx = (idx * w) + (encoded[0] & 31)
            idx = (idx * w) + (encoded[1] & 63)
            idx = (idx * w) + (encoded[2] & 63)

        idx = idx % lut_size
        processed += 1

        if lut[idx] is None:
            lut[idx] = asciis
        elif lut[idx] != asciis:
            return False

    return True

def optimize_hash_params():
    return 8314, 513

    from math import ceil

    matches = []

    for w in [31, 33, 63, 65, 127, 129, 255, 257, 511, 513]:
        step = 1000
        min_size = len([ _ for _ in map_unicode_to_ascii() ]) * 2
        max_size = min_size * 100
        previously_found = False

        while 1:
            found = False

            for lut_size in range(min_size, max_size, step):
                if is_valid_lut(lut_size, w):
                    max_size = lut_size
                    found = True
                    previously_found = True
                    matches.append([lut_size, w])
                    break

            if step == 1 or previously_found is False:
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
            idx = (idx * w) + (encoded[1] & 63)
            idx = (idx * w) + (encoded[0] & 31)
        if len(encoded) == 3:
            idx = (idx * w) + (encoded[0] & 31)
            idx = (idx * w) + (encoded[1] & 63)
            idx = (idx * w) + (encoded[2] & 63)

        idx *= 5
        idx = idx % tape_size
        idx += 256

        tape[idx] = len(asciis)
        idx += 1

        for i in range(len(asciis)):
            tape[idx] = ord(asciis[i])
            idx += 1

    return tape

size, w = optimize_hash_params()
tape = create_tape(size, w)

print(f'hash table size    : {size}')
print(f'hash multiplicator : {w}')
print(f'tape length : {len(tape)}')
