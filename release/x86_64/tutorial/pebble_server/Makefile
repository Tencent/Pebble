# make file for examples

BASE_PATH = ../..
PEBBLE = $(BASE_PATH)/tools/pebble/pebble

INC_PATH = $(BASE_PATH)/include

LIB_PATH =  $(BASE_PATH)/lib

PEBBLE_IDL = tutorial.pebble
PEBBLE_SRC = tutorial.cpp tutorial_BaseService.cpp
PEBBLE_H = tutorial.h tutorial_BaseService_if.h
PEBBLE_OBJ = $(subst .cpp,.o, $(PEBBLE_SRC))

SERVER_SRC = server.cpp 
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = server

CLIENT_SRC = client.cpp
CLIENT_OBJ = $(subst .cpp,.o, $(CLIENT_SRC))
CLIENT = client

INC_FLAGS = -I$(BASE_PATH) -I$(INC_PATH) -I$(INC_PATH)/source  

LD_FLAGS = -L$(LIB_PATH)/app -L$(LIB_PATH)/oss -L$(LIB_PATH)/rpc -L$(LIB_PATH)/message \
			-L$(LIB_PATH)/service -L$(LIB_PATH)/common -L$(LIB_PATH)/log  \
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
	$(PEBBLE) -r -out ./ --gen cpp $<

%.o: %.cpp
	$(CC) -o $@ -c $< $(CC_FLAGS)

clean: 
	rm -f $(SERVER) $(CLIENT) ./*.o $(PEBBLE_SRC) $(PEBBLE_H) 

