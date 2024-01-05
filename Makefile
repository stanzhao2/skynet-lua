
LIBDIR  = skynet-lua
VERSION = 1.0.1
LIBNAME = $(TARGET).$(VERSION)
TARGET  = lib$(LIBDIR).so

SRC = ./src
PREFIX  = /usr/local
PREFIXPATH  = $(PREFIX)/lib
INSTALLPATH = $(PREFIXPATH)/$(LIBDIR)

MAKE    = make
MKDIR   = mkdir -p
RM      = rm -f

all:
	cd $(SRC) && $(MAKE)
	
clean:
	cd $(SRC) && $(MAKE) clean
	
install:
	$(MKDIR) $(INSTALLPATH)
	cp -a $(SRC)/$(TARGET) $(INSTALLPATH)/$(LIBNAME)
	
	$(RM) $(PREFIXPATH)/$(TARGET)
	ln -s $(LIBDIR)/$(LIBNAME) $(PREFIXPATH)/$(TARGET)
