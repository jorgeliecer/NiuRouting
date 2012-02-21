CC=g++
INCLUDES=-I/usr/local/include -Iinclude
CCFLAGS = -g
SRC = src/trsp_core.cpp src/GraphDefinition.cpp
OBJ = $(SRC:.cpp=.o)
OUT = libs/libtrsp.a

.SUFFIXES: .cpp

default: $(OUT)
	
.cpp.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
	
$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
	
clean:
	rm -f $(OBJ) $(OUT) Makefile.bak