# super simple makefile
# call it using 'make NAME=name_of_code_file_without_extension'
# (assumes a .cpp extension)
NAME = "a1-basic"

all:
	@echo "Compiling..."
	g++ -o $(NAME) $(NAME).cpp -L/opt/X11/lib -lX11 -lstdc++

run: all
	@echo "Running..."
	./$(NAME) 

clean:
	-rm *o
