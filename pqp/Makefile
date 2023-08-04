PROJECT := pqp

$(PROJECT).bin: $(PROJECT).c
	gcc -g -Wall -o $(PROJECT).bin $(PROJECT).c

run: $(PROJECT).bin
	./$(PROJECT).bin $(PROJECT).input $(PROJECT).output
	cat $(PROJECT).output

clean:
	rm -rf *.bin
