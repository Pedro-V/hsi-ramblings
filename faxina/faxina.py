import sys
from iced_x86 import *

faxina_path = './faxina'

def get_bytes(string):
    return [string[i:i+2] for i in range(0, len(string), 2)]

def str_to_bytes(hex_values):
    return bytes(int(hex_value, 16) for hex_value in hex_values)


def generate_faxina_input(code, code_bytes):
    decoder = Decoder(64, code_bytes)
    faxina_input = []
    num_instructions, pos = 0, 0
    while (decoder.can_decode):
        decoder.decode()
        num_instructions += 1
        new_instruction = code[pos : decoder.position]
        faxina_input.append((
            new_instruction,
            len(new_instruction)))
        pos = decoder.position
    return num_instructions, faxina_input


def write_faxina_input(num_instructions, faxina_input):
    with open(sys.argv[1] + ".faxina", "w") as file:
        file.write(str(num_instructions) + "\n")
        for (instr, size) in faxina_input:
            formatted_line = str(size) + "\n" + " ".join(instr) + "\n"
            file.write(formatted_line)

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <input program>")
        return
    fp = sys.argv[1]
    with open(fp, 'r') as file:
        # assumes one-line file
        code = get_bytes(file.readline())[:-1]
        code_bytes = str_to_bytes(code)
    n, faxina_input = generate_faxina_input(code, code_bytes)
    write_faxina_input(n, faxina_input)

if __name__ == "__main__":
    main()
