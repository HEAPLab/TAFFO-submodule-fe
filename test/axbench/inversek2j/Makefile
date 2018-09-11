CC			:= clang++
LD			:= clang++
LLVM_DIR			?= $(shell llvm-config --obj-root)

ifeq ($(shell $(LLVM_DIR)/bin/llvm-config --build-mode),Debug)
ISDEBUG = 1
$(info debug mode on)
endif
comma :=,

OX          := #-O3

CFLAGS		:= -Wall -Wnarrowing -std=c++11
LFLAGS		:= -lboost_regex
HEADERS     := src
INCLUDE 	+= -I${HEADERS} -I../common/src

MODULE		:= inversek2j.out

CPP_FILES := $(wildcard src/*.cpp)
LL_FILES  := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.ll))) 
OBJ_FLOAT := obj/main.o
OBJ_FIX   := obj/main.fixp.o

.PHONY: all clean

all: DIR $(MODULE)

.PRECIOUS: obj/%.ll obj/%.o obj/%.s

DIR:
	@echo ${CPP_FILES}
	if [ ! -d "./bin" ];then 	\
		mkdir bin;				\
	fi
	if [ ! -d "./obj" ];then 	\
		mkdir obj;				\
	fi

$(MODULE): $(OBJ_FLOAT) $(OBJ_FIX) obj/main.err.ll
	$(LD) $(OBJ_FLOAT) $(LFLAGS) -o bin/$@
	$(LD) $(OBJ_FIX) $(LFLAGS) -o bin/$@.fixp

obj/%.ll : src/%.cpp
	$(CC) -S -emit-llvm $(CFLAGS) $(INCLUDE) $< -o $@

obj/main.ll : $(LL_FILES)
	$(LLVM_DIR)/bin/llvm-link -S $^ -o $@

$(OBJ_FLOAT) : obj/main.ll
	$(CC) -S $^ -o obj/main.s $(OX)
	$(CC) -c obj/main.s -o $@

obj/main.init.ll : obj/main.ll
	$(LLVM_DIR)/bin/opt -load=$(INITLIB) -S -taffoinit $(if $(ISDEBUG), -debug-only=flttofix$(comma)annotation) -stats $< -o $@

obj/main.fixp.ll : obj/main.init.ll
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce $(if $(ISDEBUG), -debug-only=flttofix) -stats $< -o $@

$(OBJ_FIX) : obj/main.fixp.ll
	$(CC) -S $^ -o obj/main.fixp.s $(OX)
	$(CC) -c obj/main.fixp.s -o $@

obj/main.err.ll : obj/main.fixp.ll
	$(LLVM_DIR)/bin/opt -load=$(ERRORLIB) -S -errorprop -startonly $(if $(ISDEBUG), -debug-only=errorprop) $< -o $@

clean:
	@rm -rf *.o
	@rm -rf *.d
	@rm -rf *.out
	@rm -rf bin
	@rm -rf obj
