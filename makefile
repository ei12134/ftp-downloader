# compiler
CCC = gcc

# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g -O3 -Wall

# source files
SRC = $(wildcard src/*.c)

OBJ = $(patsubst src%, bin/obj%.o, $(SRC))

OUT = bin/ftp-downloader

.SUFFIXES: .c

default: $(OUT)

bin/obj/%.o: src/%
	mkdir -p bin/obj
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	mkdir -p bin
	$(CCC) $(INCLUDES) $(CCFLAGS) $(OBJ) $(LIBS) -o $(OUT)

clean:
	rm -f $(OBJ) $(OUT)

test:
	echo $(SRC)
	echo $(OBJ)

all:
	make
