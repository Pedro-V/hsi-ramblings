CXX := g++
CXX_FLAGS := -Wall -O3 -g
PROJECT := pqp

$(PROJECT).bin: $(PROJECT).cpp
	$(CXX) $(CXX_FLAGS) -o $(PROJECT).bin $(PROJECT).cpp

run: $(PROJECT).bin
	./$(PROJECT).bin $(PROJECT).input $(PROJECT).output
	cat $(PROJECT).output

clean:
	rm -rf *.bin
