#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

uint64_t v64[256] =  { 0 };
__uint128_t v128[256] = { 0 };

const uint64_t expmod(const uint64_t b, const uint64_t n, const uint64_t p) {
	__uint128_t a = 1;
	for(uint64_t i = 0; i < n; i++)
		a = (a * b) % p;
	return a;
}

const uint64_t hardcoded() {
	v64['b'] = 2;
	v64['n'] = 123456789;
	v64['p'] = 18446744073709551533ULL;
	v128['a'] = 1;
	for(v64['i'] = 0; v64['i'] < v64['n']; v64['i']++) {
		v128['a'] = v128['a'] * v64['b'];
		//printf("mul a = %llX%llX\n", (uint64_t)(v128['a'] >> 64), (uint64_t)(v128['a']));
		v128['a'] = v128['a'] % v64['p'];
		//printf("mod a = %llX%llX\n", (uint64_t)(v128['a'] >> 64), (uint64_t)(v128['a']));
	}
	return v128['a'];
}

const uint64_t interpret(uint8_t* code, uint32_t n) {
	uint8_t eq = 0, gt = 0, lt = 0, i, j , k;
	uint32_t index = 0;
	uint64_t value = 0;
	while(index < n) {
		switch(code[index]) {
			case 0x00: // set64
				index++;
				i = code[index];
				index++;
				v64[i] = *(uint64_t*)(&code[index]);
				//printf("set64 %c = %llu\n", code[index - 1], v64[i]);
				index += 8;
				break;
			case 0x01: // set128
				index++;
				i = code[index];
				index++;
				v128[i] = *(__uint128_t*)(&code[index]);
				//printf("set128 %c = %llu\n", code[index - 1], v128[i]);
				index += 16;
				break;
			case 0x02: // cmp64
				index++;
				i = code[index];
				index++;
				j = code[index];
				eq = v64[i] == v64[j];
				gt = v64[i] > v64[j];
				lt = v64[i] < v64[j];
				index++;
				break;
			case 0x03: // bge
				//printf("bge @ %u\n", index);
				index++;
				if(eq || gt) index += *(int32_t*)(&code[index]);
				index += 4;
				//printf("bge next = %u\n", index);
				break;
			case 0x04: // bun
				//printf("bun @ %u\n", index);
				index++;
				index += *(int32_t*)(&code[index]) + 4;
				//printf("bun next = %u\n", index);
				break;
			case 0x05: // mul128
				index++;
				i = code[index];
				index++;
				j = code[index];
				index++;
				k = code[index];
				//printf("%c = %c * %c = %llX%llX\n", i, j, k, (uint64_t)(v128[i] >> 64), (uint64_t)(v128[i]));
				v128[i] = v128[j] * v64[k];
				index++;
				break;
			case 0x06: // mod128
				index++;
				i = code[index];
				index++;
				j = code[index];
				index++;
				k = code[index];
				v128[i] = v128[j] % v64[k];
				//printf("%c = %c %% %c = %llX%llX\n", i, j, k, (uint64_t)(v128[i] >> 64), (uint64_t)(v128[i]));
				index++;
				break;
			case 0x07: // inc
				index++;
				v64[code[index]]++;
				index++;
				break;
			case 0x08: // return
				index++;
				value = v128[code[index]];
				index++;
				//printf("return @ %u = %llu\n", index, value);
				break;
			default:
				printf("Undefined instruction @ %u = 0x%02X\n", index, code[index]);
				abort();
		}
	}
	return value;
}

uint8_t code[] = {
	0x55,                   // push rbp
	0x48, 0x89, 0xE5,       // mov rbp, rsp
	0x48, 0x89, 0xD1,       // mov rcx, rdx <-> rcx = p
	0x66, 0xB8, 0x01, 0x00, // mov ax, 1    <-> a = 1
	0x4D, 0x31, 0xC0,       // xor r8, r8   <-> i = 0
	0x49, 0x39, 0xF0,       // cmp r8, rsi  <-> i ? n
	0x0F, 0x83, 0x11, 0x00, 0x00, 0x00, // jae 0x11 <-> if i >= n jump to end
	0x48, 0xF7, 0xE7,                   // mul rdi  <-> rdx|rax = rax * rdi = a * b
	0x48, 0xF7, 0xF1,                   // div rcx  <-> rdx = rdx|rax % rcx = (a * b) % p
	0x48, 0x89, 0xD0,                   // mov rax, rdx <-> a = (a * b) % p
	0x49, 0xFF, 0xC0,	               // inc r8 <-> i++
	0xE9, 0xE6, 0xFF, 0xFF, 0xFF,       // jmp -0x1A <-> jump to loop
	0x5D,                               // pop rbp
	0xC3                                // ret
};

int main() {
	//printf("2^123456789 mod 18446744073709551533 = %llu\n", expmod(2, 123456789, 18446744073709551533ULL));
	//printf("2^123456789 mod 18446744073709551533 = %llu\n", hardcoded());
	/*FILE* file = fopen("program", "r");
	uint8_t code[128] = { 0 };
	uint32_t byte = 0, n = 0;
	while(fscanf(file, "%X", &byte) == 1)
		code[n++] = byte;
	printf("2^123456789 mod 18446744073709551533 = %llu\n", interpret(code, n));*/
	uint32_t length = sysconf(_SC_PAGE_SIZE);
	void* memory = mmap(0, length, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	mprotect(memory, length, PROT_WRITE);
	memcpy(memory, (void*)(code), sizeof(code));
	mprotect(memory, length, PROT_EXEC);
	const uint64_t (*jit)(const uint64_t, const uint64_t, const uint64_t) = (const uint64_t(*)(const uint64_t, const uint64_t, const uint64_t))(memory);
	printf("2^123456789 mod 18446744073709551533 = %llu\n", (*jit)(2, 123456789, 18446744073709551533ULL));
	munmap(memory, length);
	return 0;
}
