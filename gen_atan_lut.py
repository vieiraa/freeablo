import math

lut_size = 128

values = []
for i in range(lut_size):
    values.append(math.atan(i / lut_size))

lut = '{\n'
for i in range(len(values)):
    value = values[i]
    lut += '    FixedPoint("{}")'.format(value)
    
    if i != len(values) - 1:
        lut += ','

    lut += '\n'

lut += '}'

print(lut)
