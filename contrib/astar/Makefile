CC = g++
INCLUDES = -I/usr/local/include -Iinclude
CCFLAGS = -g
SRC = src/astar_boost_wrapper.cpp
OBJ = $(SRC:.cpp=.o)
OUT = libs/libastar.a

default: $(OUT)
	
.cpp.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
	
$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)
	
clean:
	rm -f $(OBJ) $(OUT) Makefile.bak