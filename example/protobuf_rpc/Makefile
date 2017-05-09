# make file for examples

BASE_PATH = ../..
PROTOC = $(BASE_PATH)/tools/protoc
PB_RPC_PLUGIN = $(BASE_PATH)/tools/protobuf_rpc

INC_PATH = $(BASE_PATH)/include
LIB_PATH =  $(BASE_PATH)/lib
PEBBLE_LIB = $(LIB_PATH)/pebble
THIRDPATY = $(LIB_PATH)/thirdparty

PROTOBUF_IDL = calculator.proto
PROTOBUF_SRC = calculator.pb.cc calculator.rpc.pb.cc
PROTOBUF_H = calculator.pb.h calculator.rpc.pb.h calculator.rpc.pb.inh
PROTOBUF_OBJ = $(subst .cc,.o, $(PROTOBUF_SRC))

SERVER_SRC = server.cpp 
SERVER_OBJ = $(subst .cpp,.o, $(SERVER_SRC))
SERVER = server

CLIENT_SRC = client.cpp
CLIENT_OBJ = $(subst .cpp,.o, $(CLIENT_SRC))
CLIENT = client

INC_FLAGS = -I$(BASE_PATH) -I$(INC_PATH)/pebble -I$(INC_PATH)/thirdparty

LD_FLAGS = -L$(PEBBLE_LIB) -L$(THIRDPATY) \
	-lpebble -lprotobuf
LD_FLAGS_CLIENT = -L$(PEBBLE_LIB) -L$(THIRDPATY) \
	-lpebble_client -lprotobuf
        
CC_FLAGS = -g -Wall -Werror $(INC_FLAGS)

CC = g++

.PHONY: all clean

all: $(SERVER) $(CLIENT)

$(SERVER): $(PROTOBUF_OBJ) $(SERVER_OBJ)
	$(CC) -o $@ $^ $(LD_FLAGS)

$(CLIENT): $(PROTOBUF_OBJ) $(CLIENT_OBJ) 
	$(CC) -o $@ $^ $(LD_FLAGS_CLIENT)

$(PROTOBUF_SRC): $(PROTOBUF_IDL)
	$(PROTOC) --cpp_out=./ $<
	$(PROTOC) --plugin=protoc-gen-rpc=$(PB_RPC_PLUGIN) --rpc_out=./ $<

%.o: %.cpp
	$(CC) $(CC_FLAGS) -o $@ -c $< 

%.o: %.cc
	$(CC) $(CC_FLAGS) -o $@ -c $< 

clean: 
	rm -rf $(SERVER) $(CLIENT) ./*.o ./log $(PROTOBUF_SRC) $(PROTOBUF_H) 

