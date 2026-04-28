import struct
import sys

bpb = bytearray(512)

bpb[0] = 0xeb
bpb[1] = 0x3c
bpb[2] = 0x90

for i, c in enumerate(b'MrOS_FS'):
    bpb[3 + i] = c

struct.pack_into('<H', bpb, 11, 512)
bpb[13] = 1
struct.pack_into('<H', bpb, 14, 1)
bpb[16] = 2
struct.pack_into('<H', bpb, 17, 224)
struct.pack_into('<H', bpb, 19, 2880)
bpb[21] = 0xF0
struct.pack_into('<H', bpb, 22, 9)
struct.pack_into('<H', bpb, 24, 18)
struct.pack_into('<H', bpb, 26, 2)
struct.pack_into('<I', bpb, 28, 0)
struct.pack_into('<I', bpb, 32, 0)
struct.pack_into('<H', bpb, 510, 0xAA55)

open(sys.argv[1], 'wb').write(bpb)