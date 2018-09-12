CC			:= clang++
LD			:= clang++
LLVM_DIR			?= $(shell llvm-config --obj-root)

ifeq ($(shell $(LLVM_DIR)/bin/llvm-config --build-mode),Debug)
ISDEBUG = 1
$(info debug mode on)
endif
comma :=,

OX          := -O3

CFLAGS		+= -Wall -Wnarrowing -std=c++11
LFLAGS		+= #-lboost_regex
HEADERS		:= src
INCLUDE 		+= -I${HEADERS} -I../common/src

MODULE		:= jmeint.out

CPP_FILES := $(wildcard src/*.cpp)
LL_FILES  := $(addprefix obj/,$(notdir $(CPP_FILES:.cpp=.ll))) 
OBJ_FLOAT := obj/main.o
OBJ_FIX   := obj/main.fixp.o

.PHONY: all clean

all: DIR $(MODULE)

.PRECIOUS: obj/%.ll obj/%.o obj/%.s

DIR:
	@echo ${CPP_FILES}
	mkdir -p bin
	mkdir -p obj
	mkdir -p stats

$(MODULE): $(OBJ_FLOAT) $(OBJ_FIX) obj/main.err.ll stats/main.fixp.mix.txt stats/main.mix.txt stats/main.llvm.txt
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
	$(LLVM_DIR)/bin/opt -load=$(INITLIB) -S -taffoinit $(if $(ISDEBUG), -debug-only=flttofix$(comma)annotation) -stats $< -o $@ 2> stats/init.log

obj/main.fixp.ll : obj/main.init.ll
	# no debug prints to reduce compile times
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce -mem2reg -stats $< -o $@  2> stats/flttofix.log

obj/main.err.ll : obj/main.fixp.ll
	$(LLVM_DIR)/bin/opt -load=$(ERRORLIB) -S -errorprop -startonly $(if $(ISDEBUG), -debug-only=errorprop$(comma)loop-unroll) $< -o $@ 2> stats/errorprop.log

$(OBJ_FIX) : obj/main.fixp.ll
	$(CC) -S $^ -o obj/main.fixp.s $(OX)
	$(CC) -c obj/main.fixp.s -o $@

stats/%.mix.txt: obj/%.ll
	$(INSTMIX) $< > $@

stats/%.llvm.txt: obj/%.ll
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce -stats $< -o /dev/null 2> $@


clean:
	rm -rf *.o
	rm -rf *.d
	rm -rf *.out
	rm -rf bin
	rm -rf obj
	rm -rf stats
