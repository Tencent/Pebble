# make file for examples

BASE_PATH = ../..

INC_PATH = $(BASE_PATH)/include
LIB_PATH =  $(BASE_PATH)/lib
PEBBLE_LIB = $(LIB_PATH)/pebble


SERVER_SRC = server.cpp 
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = server


INC_FLAGS = -I$(BASE_PATH) -I$(INC_PATH)/pebble 

LD_FLAGS = -L$(PEBBLE_LIB) \
	-lpebble -lpthread

CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(SERVER) 

$(SERVER): $(PEBBLE_OBJ) $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -rf $(SERVER) ./*.o 

