CC = gcc 

.PHONY: all debug clean build

AUXFILES := README

depth := 1

PROJDIRS := src
SRCFILES := $(shell find $(PROJDIRS) -maxdepth $(depth)  -type f ! -name "tests.c" -name "*.c")

HDRFILES := $(shell find $(PROJDIRS) -maxdepth $(depth) -type f -name "*.h")

OBJFILES := $(patsubst %.c,%.o,$(SRCFILES))

DEPFILES := $(patsubst %.c,%.d, $(SRCFILES))

ALLFILES := $(SRCFILES) $(HDRFILES)

WARNINGS := -Wall

DBG_FLAGS := -g

CCFLAGS :=


LDFLAGS :=
LDLIBS :=


all: CCFLAGS := $(CCFLAGS)
all: build

-include $(DEPFILES)


%.o : %.c Makefile
	$(CC) $(CCFLAGS) -MMD -MP -c $< -o $@
%.o : %.cc Makefile
	$(CCXX) $(CCXXFLAGS) -MMD -MP -c $< -o $@



debug: CCFLAGS := $(CCFLAGS) $(DBG_FLAGS)
debug: build

debug_test: CCFLAGS := $(CCFLAGS) $(DBG_FLAGS)
debug_test: test build

test.o: src/tests.c
	$(CC) $(CCFLAGS) -c -MMD -MP  $^ -o $@

test: test.o build
	$(CC) $(CCFLAGS) -L. test.o -lgex -o $@

build: libgex.a

libgex.a: $(OBJFILES)
	-rm -f $@;\
	ar rcs $@ $^


clean:
	-rm -f $(OBJFILES)
	-rm -f $(DEPFILES) 
	-rm -f test.d
	-rm -f test tests.o test.o
