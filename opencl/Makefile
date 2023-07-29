PROJECT := opencl

$(PROJECT).bin: $(PROJECT).cpp
	g++ -g -Wall -o $(PROJECT).bin $(PROJECT).cpp -lOpenCL

run: $(PROJECT).bin
	./$(PROJECT).bin $(PROJECT).input $(PROJECT).output
	cat $(PROJECT).output

clean:
	rm -rf *.bin
