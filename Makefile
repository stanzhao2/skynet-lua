
###################################################################

CCOMPILE   := gcc
CPPCOMPILE := g++
LINK       := g++

CONFIG := ./config
include $(CONFIG)
PREFIXPATH = $(PREFIX)/bin

######################### OPTIONS BEGIN ###########################
#target file

TARGET  = skynet
VERSION = 3.0.1
OUTPUT  = ./bin/$(TARGET)
MAKE    = make
MKDIR   = mkdir -p
RM      = rm -f
INSTALLPATH = $(PREFIXPATH)

#source files
SOURCE  :=  src/socket.io/socket.io.o \
			src/http-parser/http_parser.o \
			src/luaf_http.o \
			src/luaf_allotor.o \
			src/luaf_bind.o \
			src/luaf_clock.o \
			src/luaf_crypto.o \
			src/luaf_gzip.o \
			src/luaf_dir.o \
			src/luaf_json.o \
			src/luaf_metatable.o \
			src/luaf_pack.o \
			src/luaf_pcall.o \
			src/luaf_pload.o \
			src/luaf_print.o \
			src/luaf_ref.o \
			src/luaf_require.o \
			src/luaf_rpcall.o \
			src/luaf_socket.o \
			src/luaf_state.o \
			src/luaf_timer.o \
			src/luaf_skynet.o \
			src/rapidjson/document.o \
			src/rapidjson/schema.o \
			src/rapidjson/values.o \
			src/rapidjson/rapidjson.o
		   
#library path
LIBDIRS := -L$(LUA_LIBDIR)

#include path
INCDIRS := $(LUA_INCLUDE) -I./include -I./include/asio -I./include/eport

#precompile macro
CC_FLAG := -DEPORT_SSL_ENABLE -DEPORT_ZLIB_ENABLE

#compile options
COMPILEOPTION := -std=c++11 -fPIC -w -Wfatal-errors -O2

#rpath options
LIBLOADPATH := -Wl,-rpath=./ -Wl,-rpath=./lib -Wl,-rpath=L$(LUA_LIBDIR)

#link options
LINKOPTION := -o $(OUTPUT) $(LIBLOADPATH)

#dependency librarys
LIBS := -llua -lz -lcrypto -lssl -ldl -lpthread

########################## OPTIONS END ############################

$(OUTPUT): $(SOURCE)
	$(LINK) $(LINKOPTION) $(LIBDIRS) $(SOURCE) $(LIBS)

clean: 
	$(RM) $(SOURCE)
	$(RM) $(OUTPUT)
	
install:
	$(RM) $(INSTALLPATH)/$(TARGET)-$(VERSION)
	cp $(OUTPUT) $(INSTALLPATH)/$(TARGET)-$(VERSION)
	
	$(RM) $(INSTALLPATH)/lua
	cp -r ./bin/lua $(INSTALLPATH)
	
	$(RM) $(PREFIXPATH)/$(TARGET)
	ln -s $(TARGET)-$(VERSION) $(PREFIXPATH)/$(TARGET)
	
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
