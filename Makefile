CC = g++
INCLUDES = -I/usr/local/include -Iinclude -Icontrib/trsp/include
CFLAGS = -bundle -fPIC -Wall
SRC = src/main.c
OBJ = $(SRC:.c=.o)
LIBS = -Lcontrib/trsp/libs -ltrsp
OUT = bin/niurouting.sqlext

default: $(OUT)
	
.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
	
$(OUT): $(OBJ)
	$(CC) $(INCLUDES) -o $@ $^ $(CFLAGS) $(LIBS)
	
clean:
	rm -f $(OBJ) $(OUT) Makefile.bak