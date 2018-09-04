CC			:= clang++
LD			:= clang++

OX          :=# -O3

CFLAGS		:= -Wall -Wnarrowing -std=c++11
LFLAGS		:= -lboost_regex
HEADERS     := src
INCLUDE 	:= -I${HEADERS} 

MODULE		:= jpeg.out

CPP_FILES := $(wildcard src/*.c)
LL_FILES  := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.ll))) 
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

obj/%.ll : src/%.c
	$(CC) -S -emit-llvm $(CFLAGS) $(INCLUDE) $< -o $@

obj/main.ll : $(LL_FILES)
	llvm-link -S $^ -o $@

$(OBJ_FLOAT) : obj/main.ll
	$(CC) -S $^ -o obj/main.s $(OX)
	$(CC) -c obj/main.s -o $@

obj/main.init.ll : obj/main.ll
	opt -load=$(INITLIB) -S -taffoinit -debug-only=flttofix,annotation -stats $< -o $@

obj/main.fixp.ll : obj/main.init.ll
	opt -load=$(PASSLIB) -S -flttofix -dce -mem2reg -debug-only=flttofix -stats $(OX) $< -o $@

obj/main.err.ll : obj/main.fixp.ll
	opt -load=$(ERRORLIB) -S -errorprop -nounroll -startonly -debug-only=errorprop $< -o $@

$(OBJ_FIX) : obj/main.fixp.ll
	$(CC) -S $^ -o obj/main.fixp.s $(OX)
	$(CC) -c obj/main.fixp.s -o $@

clean:
	@rm -rf *.o
	@rm -rf *.d
	@rm -rf *.out
	@rm -rf bin
	@rm -rf obj
