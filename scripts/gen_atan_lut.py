#!/usr/bin/env python3

import math

lut_size = 64

values = []
for i in range(lut_size):
    values.append(math.degrees(math.atan(i / lut_size)))

lut = 'constexpr size_t atanLutSize = {};\n'.format(lut_size)
lut += 'static const FixedPoint atanLut[atanLutSize] =\n{\n'
for i in range(len(values)):
    value = values[i]
    lut += '    FixedPoint("{}")'.format(value)
    
    if i != len(values) - 1:
        lut += ','

    lut += '\n'

lut += '};'

print(lut)
