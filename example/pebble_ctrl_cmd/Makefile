# make file for examples

BASE_PATH = ../..

INC_PATH = $(BASE_PATH)/include
LIB_PATH =  $(BASE_PATH)/lib
PEBBLE_LIB = $(LIB_PATH)/pebble
THIRDPATY = $(LIB_PATH)/thirdparty


SERVER_SRC = server.cpp
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = server


INC_FLAGS = -I$(BASE_PATH) -I$(INC_PATH)/pebble -I$(INC_PATH)/thirdparty

LD_FLAGS = -L$(PEBBLE_LIB) -L$(THIRDPATY) \
	-lpebble -lhttp_protocol_api -ltbuspp -ltbuspp_proto -lprotobuf -lmysqlpp -lmysqlclient -lz -lpthread
        
CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(SERVER)

$(SERVER): $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -rf $(SERVER) ./*.o ./*.pid ./log 

