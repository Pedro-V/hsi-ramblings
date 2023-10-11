CXX := gcc
CXX_FLAGS := -Wall -O3 -g
PROJECT := pqp

$(PROJECT).bin: $(PROJECT).c
	$(CXX) $(CXX_FLAGS) -o $(PROJECT).bin $(PROJECT).c

run: $(PROJECT).bin
	./$(PROJECT).bin $(PROJECT).input $(PROJECT).output
	cat $(PROJECT).output

golden: $(PROJECT).bin
	./$(PROJECT).bin golden.input golden.output
	cat golden.output
clean:
	rm -rf *.bin *.o
