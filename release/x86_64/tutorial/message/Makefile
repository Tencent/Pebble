# make file for examples

BASE_PATH = ../..

INC_PATH = $(BASE_PATH)/include

LIB_PATH =  $(BASE_PATH)/lib

SERVER_SRC = message_server.cpp 
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = message_server

CLIENT_SRC = message_client.cpp
CLIENT_OBJ = $(subst .cpp,.o, $(CLIENT_SRC))
CLIENT = message_client

INC_FLAGS = -I$(INC_PATH) -I$(INC_PATH)/source/message 

LD_FLAGS = -L$(LIB_PATH)/message -L$(LIB_PATH)/rpc \
			-L$(LIB_PATH)/common -L$(LIB_PATH)/zookeeper -L$(LIB_PATH)/zeromq \
			-lpebble_message -lpebble_rpc_s -lpebble_common \
			-lzookeeper_st -lzmq -lrt -lpthread

CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

$(CLIENT): $(CLIENT_OBJ) 
	$(CC) -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -f $(SERVER) $(CLIENT) ./*.o 

