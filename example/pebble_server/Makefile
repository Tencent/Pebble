# make file for examples

BASE_PATH = ../..
PEBBLE = $(BASE_PATH)/tools/pebble

INC_PATH = $(BASE_PATH)/include
LIB_PATH =  $(BASE_PATH)/lib
PEBBLE_LIB = $(LIB_PATH)/pebble
THIRDPATY = $(LIB_PATH)/thirdparty

PEBBLE_IDL = calculator.pebble
PEBBLE_SRC = calculator.cpp calculator_Calculator.cpp
PEBBLE_H = calculator.h calculator_Calculator.h calculator_Calculator.inh
PEBBLE_OBJ = $(subst .cpp,.o, $(PEBBLE_SRC))

SERVER_SRC = server.cpp 
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = server

CLIENT_SRC = client.cpp
CLIENT_OBJ = $(subst .cpp,.o, $(CLIENT_SRC))
CLIENT = client

INC_FLAGS = -I$(BASE_PATH) -I$(INC_PATH)/pebble -I$(INC_PATH)/thirdparty

LD_FLAGS = -L$(PEBBLE_LIB) -L$(THIRDPATY) \
	-lpebble
LD_FLAGS_CLIENT = -L$(PEBBLE_LIB) -L$(THIRDPATY) \
	-lpebble_client
        
CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(SERVER): $(PEBBLE_OBJ) $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

$(CLIENT): $(PEBBLE_OBJ) $(CLIENT_OBJ) 
	$(CC) -o $@ $^ $(LD_FLAGS_CLIENT)

$(PEBBLE_SRC): $(PEBBLE_IDL)
	$(PEBBLE) -r -out ./ --gen cpp $<

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -rf $(SERVER) $(CLIENT) ./*.o ./log $(PEBBLE_SRC) $(PEBBLE_H) 

