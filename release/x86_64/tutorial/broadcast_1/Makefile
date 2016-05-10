# make file for examples

BASE_PATH = ../..
PEBBLE = $(BASE_PATH)/tools/pebble/pebble

INC_PATH = $(BASE_PATH)/include

LIB_PATH =  $(BASE_PATH)/lib

PEBBLE_IDL = broadcast_tutorial.pebble
PEBBLE_SRC = idl/broadcast_tutorial.cpp idl/broadcast_tutorial_BroadcastTest.cpp idl/broadcast_tutorial_LoginService.cpp
PEBBLE_H = idl/broadcast_tutorial.h idl/broadcast_tutorial_BroadcastTest_if.h idl/broadcast_tutorial_LoginService_if.h
PEBBLE_OBJ = $(subst .cpp,.o, $(PEBBLE_SRC))

SERVER_SRC = broadcast_server.cpp
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = broadcast_server

CLIENT_SRC = broadcast_client.cpp
CLIENT_OBJ = $(subst .cpp,.o, $(CLIENT_SRC))
CLIENT = broadcast_client

INC_FLAGS = -I$(BASE_PATH) -I$(INC_PATH) -I$(INC_PATH)/source  

LD_FLAGS = -L$(LIB_PATH)/app -L$(LIB_PATH)/rpc -L$(LIB_PATH)/message -L$(LIB_PATH)/service \
			-L$(LIB_PATH)/oss -L$(LIB_PATH)/common -L$(LIB_PATH)/log  \
			-L$(LIB_PATH)/zookeeper -L$(LIB_PATH)/inih-master -L$(LIB_PATH)/zeromq \
			-lpebble_app -lpebble_rpc_s -lpebble_message -lpebble_service_manage \
			-lpebble_common -lpebble_log \
			-lzookeeper_st -lanl -linih -lzmq -lrt -lpthread

CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(SERVER): $(PEBBLE_OBJ) $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

$(CLIENT): $(PEBBLE_OBJ) $(CLIENT_OBJ) 
	$(CC) -o $@ $^ $(LD_FLAGS)

$(PEBBLE_SRC): $(PEBBLE_IDL)
	$(PEBBLE) -r -out ./idl --gen cpp $<

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -f $(SERVER) $(CLIENT) ./*.o $(PEBBLE_SRC) $(PEBBLE_H) ./idl/*.o

