import sys
from iced_x86 import *
import subprocess

faxina_path = './faxina'

def get_bytes(string):
    return [string[i:i+2] for i in range(0, len(string), 2)]


def str_to_bytes(string):
    hex_values = get_bytes(string)
    return bytes(int(hex_value, 16) for hex_value in hex_values)


def generate_faxina_input(code_bytes):
    decoder = Decoder(64, code)
    faxina_input = []
    num_instructions, pos = 0, 0
    while (decoder.can_decode):
        decoder.decode()
        num_instructions += 1
        faxina_input.append(code[pos:decoder.position])
    return num_instructions, faxina_input


def pass_faxina_input(process, num_instructions, faxina_input):
    process.stdin.write(num_instructions + "\n")
    for line in faxina_input:
        formatted_line = " ".join(line) + "\n"
        process.stdin.write(formatted_line)
    process.stdin.flush()

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <input program>")
        return
    fp = sys.argv[1]
    with open(fp, 'r') as exemplo:
        # assumes one-line file
        code_bytes = str_to_bytes(exemplo.readline())
    process = subprocess.Popen(faxina_path,
                               stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    n, faxina_input = generate_faxina_input(code_bytes)
    pass_faxina_input(process, n, faxina_input)

if __name__ == "__main__":
    main()
