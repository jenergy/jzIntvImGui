OBJECTS = main.o parser.o token.o
TARGETS = imasm
LIBRARIES = 
CC=g++ 

default: imasm

imasm: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBRARIES) -o imasm

main.o: main.cpp	
	$(CC) -c main.cpp
token.o: token.cpp
	$(CC) -c token.cpp
parser.o: parser.cpp
	$(CC) -c parser.cpp



clean:
	rm -f $(TARGETS)	rm -f *.o	rm -f *~