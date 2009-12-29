# The makefile can be invoked with
# ARCH = x86_64 or x64 for EM64T build
# ARCH = ia32 or IA32 for IA32 build
# ARCH = ia64 or IA64 for IA64 build
#
ARCH ?= IA32

CYGWIN:=$(findstring CYGWIN, $(shell uname -s))
LINUX:=$(findstring Linux, $(shell uname -s))
DARWIN:=$(findstring Darwin, $(shell uname -s))

CC = gcc
CXX = g++
AS = gcc
AR = ar
LD = ld
LINKER ?= $(CC)
ifeq ($(ARCH), IA32)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/Ia32/
endif

ifeq ($(ARCH), X64)
ARCH_INCLUDE = -I $(MAKEROOT)/Include/X64/
endif

INCLUDE = $(TOOL_INCLUDE) -I $(MAKEROOT) -I $(MAKEROOT)/Include/Common -I $(MAKEROOT)/Include/ -I $(MAKEROOT)/Include/IndustryStandard -I $(MAKEROOT)/Common/ -I .. -I . $(ARCH_INCLUDE) 
CPPFLAGS = $(INCLUDE)
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fno-merge-constants -nostdlib -Wall -Werror -c -g
LFLAGS =

#
# Snow Leopard  is a 32-bit and 64-bit environment. uname -m returns -i386, but gcc defaults 
#  to x86_64. So make sure tools match uname -m
#
uname_s = $(shell uname -s)
ifeq ($(uname_s),Darwin)
  CFLAGS   += -arch i386
  CPPFLAGS += -arch i386
  LFLAGS   += -arch i386
endif
  
.PHONY: all
.PHONY: install
.PHONY: clean

all:

$(MAKEROOT)/libs:
	mkdir $(MAKEROOT)/libs 

$(MAKEROOT)/bin:
	mkdir $(MAKEROOT)/bin
