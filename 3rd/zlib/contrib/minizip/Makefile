BIN=libunz.a
CC=gcc
AR=ar rcs
CFLAGS=
LIBS=../../libz.a

INCS=-I../..
SRCS:=unzip.o ioapi.o 
COBJS:=$(SRCS:.c=.o)

all:$(BIN)

$(COBJS) : %.o: %.c
	$(CC) -c $< -o $@ $(INCS) $(CFLAGS)

$(BIN):$(COBJS)
	$(AR) -o $(BIN) $(COBJS) $(LIBS) $(CFLAGS)

clean:
	rm $(BIN) $(COBJS)
	