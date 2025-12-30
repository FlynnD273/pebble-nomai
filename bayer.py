import sys
LEN = 8
array = [
    0,
    32,
    8,
    40,
    2,
    34,
    10,
    42,
    48,
    16,
    56,
    24,
    50,
    18,
    58,
    26,
    12,
    44,
    4,
    36,
    14,
    46,
    6,
    38,
    60,
    28,
    52,
    20,
    62,
    30,
    54,
    22,
    3,
    35,
    11,
    43,
    1,
    33,
    9,
    41,
    51,
    19,
    59,
    27,
    49,
    17,
    57,
    25,
    15,
    47,
    7,
    39,
    13,
    45,
    5,
    37,
    63,
    31,
    55,
    23,
    61,
    29,
    53,
    21,
]
output = f"switch(y % {LEN}) {{"
for y in range(len(array) // LEN):
    output += f"\tcase {y}:\n\t\tpatt=0b"
    for x in range(LEN):
        if array[x + y * LEN] < int(sys.argv[1]):
            output += "0"
        else:
            output += "1"
    output += ";\n\tbreak;\n"
output += "}"
print(output)

