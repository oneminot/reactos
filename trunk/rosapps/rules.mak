#
# Select your host
#
#HOST = mingw32-linux
HOST = mingw32-windows


#
# Important
#
.EXPORT_ALL_VARIABLES:

ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
PREFIX = i586-mingw32-
EXE_POSTFIX = 
CP = cp
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
endif

ifeq ($(HOST),mingw32-windows)
PREFIX = 
EXE_POSTFIX = .exe
CP = copy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
RM = del
DOSCLI = yes
endif

#
# Create variables for all the compiler tools 
#
ifeq ($(WITH_DEBUGGING),yes)
DEBUGGING_CFLAGS = -g
else
DEBUGGING_CFLAGS = 
endif

ifeq ($(WARNINGS_ARE_ERRORS),yes)
EXTRA_CFLAGS = -Werror
endif

DEFINES = -DDBG

ifeq ($(WIN32_LEAN_AND_MEAN),yes)
LEAN_AND_MEAN_DEFINE = -DWIN32_LEAN_AND_MEAN
else
LEAN_AND_MEAN_DEFINE = 
endif 

CC = $(PREFIX)gcc
NATIVE_CC = gcc
CFLAGS = -O2 -Wall -Wstrict-prototypes -fno-builtin \
         $(LEAN_AND_MEAN_DEFINE) $(DEFINES) $(DEBUGGING_CFLAGS) \
         $(EXTRA_CFLAGS)
CXXFLAGS = $(CFLAGS)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS = $(PREFIX)gcc -c -x assembler-with-cpp
CPP = $(PREFIX)cpp
AR = $(PREFIX)ar
RC = $(PREFIX)windres

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


RULES_MAK_INCLUDED = 1
