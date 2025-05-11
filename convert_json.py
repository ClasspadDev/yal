import sys
import json

input = sys.argv[1]
output = sys.argv[2]

with open(input, 'r') as f:
    data = json.load(f)

if data['isInMemory'] != False:
    raise ValueError('This script is only for out of memory functions')

with open(output, 'wb') as f:
    f.write(data['safeGuard'].encode('ascii') + b'\0')
    for item in data['addresses']:
        f.write(item['FunctionName'].encode('ascii') + b'\0')
        f.write(int(item['startAddress'], base=0).to_bytes(4, byteorder='big', signed=False))