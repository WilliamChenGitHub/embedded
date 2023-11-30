PLATFORM_LINUX := linux
PLATFORM_WINDOWS := windows
PLATFORM_FREE_RTOS := freeRtos

CCFLAGS := -fpic

ERRFLAGS := -Wall -Wextra -Wcast-align -Wcast-qual -Waddress -Warray-bounds

CCFLAGS += $(ERRFLAGS)

ARCH := $(shell uname -m)

VERSION_BCD := 0x100

ifeq ($(findstring $(PLATFORM_LINUX),$(MAKECMDGOALS)),$(PLATFORM_LINUX))
    PLATFORM := $(PLATFORM_LINUX)
    WORDSIZE := __WORDSIZE=64
    CROSS := 
    CCFLAGS += -std=gnu11
else

ifeq ($(findstring $(PLATFORM_WINDOWS),$(MAKECMDGOALS)),$(PLATFORM_WINDOWS))
    PLATFORM := $(PLATFORM_WINDOWS)
    WORDSIZE := __WORDSIZE=64
    CROSS :=
    CCFLAGS += 
else

ifeq ($(findstring $(PLATFORM_FREE_RTOS),$(MAKECMDGOALS)),$(PLATFORM_FREE_RTOS))
    PLATFORM := $(PLATFORM_FREE_RTOS)
    WORDSIZE := __WORDSIZE=32
    CROSS := 
    CCFLAGS += 
    MACROFLAGS := -U__unix__ -U__linux__
else
    PLATFORM := $(PLATFORM_LINUX)
    WORDSIZE := __WORDSIZE=64
    CROSS :=
    CCFLAGS += -std=gnu11 -fpic
endif

endif

endif


DEBUG := debug
ifeq ($(findstring $(DEBUG),$(MAKECMDGOALS)),$(DEBUG))
DBGSTR := debug
DBGFLAG := -g
endif

OPTIMIZATION := -O0

MACRODEF += -D$(WORDSIZE) $(MACROFLAGS)

__VERSION_HEX := $(shell echo $(VERSION_BCD)) 
__VERSION_DEF := __LIBEMBEDDEDVER=$(__VERSION_HEX)
MACRODEF += -D$(__VERSION_DEF)


CC := $(CROSS)gcc
AR := $(CROSS)ar


INC_PATH += -I Common
INC_PATH += -I Algorithm/Sort 
INC_PATH += -I DataStruct/List -I DataStruct/Queue -I DataStruct/Stack -I DataStruct/Tree -I DataStruct/Graph
INC_PATH += -I Tools/Communication -I Tools/JSON

LIB_PATH += -L ./
LIBS += -lc -lpthread

SRC_DIRS += $(INC_PATH)

OUT := embedded
SRCS := $(foreach n,$(SRC_DIRS),$(wildcard $(n)/*.c))
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.dep)

HEAD_FILES := $(foreach n,$(INC_PATH),$(wildcard $(n)/*.h))

#include $(DEPS)

.PHONY : all clean distclean $(PLATFORM_LINUX) $(PLATFORM_WINDOWS) $(PLATFORM_FREE_RTOS) $(DEBUG) install test release

$(PLATFORM_LINUX) : clean all

$(PLATFORM_FREE_RTOS) : clean all

VERSION_STR := $(subst 0x,V,$(VERSION_BCD))
RELEASE_NAME := $(OUT)-$(PLATFORM)-$(ARCH)-$(VERSION_STR)$(DBGSTR)
RELEASE_DIR := release

$(DEBUG) : all
	@echo debug version was builded

all : $(DEPS) $(OBJS) $(OUT)
	@echo make success The platform is $(PLATFORM), arch is $(ARCH), version is $(VERSION_STR)

release : all
	-mkdir $(RELEASE_DIR)
	-mkdir $(RELEASE_DIR)/$(RELEASE_NAME)
	-mkdir $(RELEASE_DIR)/$(RELEASE_NAME)/$(OUT)Inc
	@cp $(HEAD_FILES) $(RELEASE_DIR)/$(RELEASE_NAME)/$(OUT)Inc
	@cp lib$(OUT).a lib$(OUT).so ChangeLog.log $(RELEASE_DIR)/$(RELEASE_NAME)
	@tar -czf $(RELEASE_DIR)/lib$(RELEASE_NAME).tar.gz $(RELEASE_DIR)/$(RELEASE_NAME)

install : 
	cp lib$(OUT).so /lib

test : $(OUT)
	$(CC) -o Test/test Test/test.c $(DBGFLAG) $(ERRFLAGS) $(INC_PATH) $(MACRODEF) $(LIB_PATH) -l:lib$(OUT).a $(LIBS)

$(OBJS):%.o : %.c
	$(CC) -o $@ -c $(filter %.c,$^) $(DBGFLAG) $(CCFLAGS) $(OPTIMIZATION) $(MACRODEF) $(INC_PATH) $(LIB_PATH) $(LIBS)

$(OUT) : $(OBJS)
	$(AR) -rc lib$@.a $^ 
	$(CC) -shared -fpic -o lib$@.so $^ $(LIBS)

$(DEPS):%.dep : %.c
	@echo "Creating $@ ..."
	@set -e;\
	$(CC) -MM $(INC_PATH) $< | \
	sed 's,.*\.o[ :]*,$*.o $@ : ,g' > $@;

clean :
	-rm $(OBJS) $(DEPS) lib$(OUT).* Test/test

distclean: clean
	-rm $(RELEASE_DIR) -rf
