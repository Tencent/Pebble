# make file for examples

BASE_PATH = ../..

INC_PATH = $(BASE_PATH)/include

LIB_PATH =  $(BASE_PATH)/lib

EXAMPLE_SRC = coroutine_tutorial.cpp
EXAMPLE_OBJ = $(subst .cpp,.o, $(EXAMPLE_SRC))
EXAMPLE = coroutine_tutorial

INC_FLAGS = -I$(INC_PATH) 

LD_FLAGS = -L$(LIB_PATH)/common -L$(LIB_PATH)/log  -lpebble_common -lpebble_log -lpthread

CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(EXAMPLE)

$(EXAMPLE): $(EXAMPLE_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -f $(EXAMPLE) ./*.o

