CXX := gcc
CXX_FLAGS := -Wall -g
PROJECT := pqp

$(PROJECT).bin: $(PROJECT).c
	$(CXX) $(CXX_FLAGS) -o $(PROJECT).bin $(PROJECT).c

run: $(PROJECT).bin
	./$(PROJECT).bin $(PROJECT).input $(PROJECT).output
	cat $(PROJECT).output

clean:
	rm -rf *.bin *.o
