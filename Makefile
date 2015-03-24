#
#change this makefile for your target...
#

BINDIR_PREFIX := /home/$(USER)/targetfs-c6x-abc
BARDY_PREFIX := ../BARDY_C6X
DIRS := $(BARDY_PREFIX)/gipcy/include
LIBPATH  = $(BARDY_PREFIX)/bin

ifeq "$(findstring c6x-uclinux-gcc, $(CC))" "c6x-uclinux-gcc"
    OS := __LINUX__
    CSTOOL_PREFIX ?= c6x-uclinux-
    IPC := __IPC_LINUX__
    TARGET := -march=c674x
    PLATFORM := _c6x_
else
    PLATFORM := _x86_
endif

PHONY = clean
TARGET_NAME = azbuka

all: $(TARGET_NAME)

ROOT_DIR = $(shell pwd)

INC := $(addprefix -I, $(DIRS))

CC = $(CSTOOL_PREFIX)g++
LD = $(CSTOOL_PREFIX)g++

CFLAGS += $(TARGET) -D$(PLATFORM) -D__LINUX__ -g -Wall $(INC)
#LFLAGS = -Wl,-rpath $(LIBPATH) -L"$(LIBPATH)"
LFLAGS += $(TARGET)

$(TARGET_NAME): $(patsubst %.cpp,%.o, $(wildcard *.cpp))
	$(LD) -o $(TARGET_NAME) $^ $(LIBPATH)/libgipcy.a $(LFLAGS)
ifeq "$(findstring c6x, $(CC))" "c6x"
	c6x-uclinux-strip $(TARGET_NAME)
	cp $(TARGET_NAME) $(BINDIR_PREFIX)/home/embedded/examples
endif
#	rm -f *.o *~ core

%.o: %.cpp
	$(CC) $(CFLAGS) -c -MD $<
	
include $(wildcard *.d)


clean:
	rm -f *.o *~ core
	rm -f *.d *~ core
	rm -f $(TARGET_NAME)
	
distclean:
	rm -f *.o *~ core
	rm -f *.d *~ core
	rm -f $(TARGET_NAME)
