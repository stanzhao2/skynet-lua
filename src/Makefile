
###################################################################

CCOMPILE   := gcc
CPPCOMPILE := g++
LINK       := g++

######################### OPTIONS BEGIN ###########################
#target file
OUTPUT  := libskynet-lua.so

#source files
SOURCE  :=  socket.io/socket.io.o \
			luaf_allotor.o \
			luaf_bind.o \
			luaf_clock.o \
			luaf_crypto.o \
			luaf_gzip.o \
			luaf_json.o \
			luaf_metatable.o \
			luaf_pack.o \
			luaf_pcall.o \
			luaf_pload.o \
			luaf_print.o \
			luaf_ref.o \
			luaf_require.o \
			luaf_rpcall.o \
			luaf_socket.o \
			luaf_state.o \
			luaf_timer.o \
			rapidjson/document.o \
			rapidjson/schema.o \
			rapidjson/values.o \
			rapidjson/rapidjson.o

#library path
LIBDIRS := 

#include path
INCDIRS := -I../include/asio -I../include/eport -I../3rd/lua/5.4/src

#precompile macro
CC_FLAG := -DEPORT_SSL_ENABLE -DEPORT_ZLIB_ENABLE

#compile options
COMPILEOPTION := -std=c++11 -fPIC -w -Wfatal-errors -O2

#link options
LINKOPTION := -shared -Wl,--retain-symbols-file=symbol/skynet.so.export -Wl,--version-script=symbol/skynet.so.script -o $(OUTPUT)

#dependency librarys
LIBS := -llua -lz -lcrypto -lssl -ldl -lpthread

########################## OPTIONS END ############################

$(OUTPUT): $(SOURCE)
	$(LINK) $(LINKOPTION) $(LIBDIRS) $(SOURCE) $(LIBS)

clean: 
	rm -f $(SOURCE)
	rm -f $(OUTPUT)

all: clean $(OUTPUT)
.PRECIOUS:%.cpp %.c %.C
.SUFFIXES:
.SUFFIXES:  .c .o .cpp .ecpp .pc .ec .C .cc .cxx

.cpp.o:
	$(CPPCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS)  $*.cpp
	
.cc.o:
	$(CCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS)  $*.cx

.cxx.o:
	$(CPPCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS)  $*.cxx

.c.o:
	$(CCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS) $*.c

.C.o:
	$(CPPCOMPILE) -c -o $*.o $(CC_FLAG) $(COMPILEOPTION) $(INCDIRS) $*.C

########################## MAKEFILE END ############################
