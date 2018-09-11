CC			:= clang++
LD			:= clang++
LLVM_DIR			?= $(shell llvm-config --obj-root)

ifeq ($(shell $(LLVM_DIR)/bin/llvm-config --build-mode),Debug)
ISDEBUG = 1
$(info debug mode on)
endif
comma :=,

OX          := -O3

CFLAGS		:= -Wall -Wnarrowing -std=c++11 -MD
LFLAGS		:= -lboost_regex 
HEADERS     := src
INCLUDE 	:= -I${ANALOG_INC} -I${HEADERS}
LIB			:= -L${ANALOG_LIB} 

MODULE		:= blackscholes.out

CPP_FILES       := blackscholes.c
OBJ_FILES       := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.o))) 
OBJ_FILES_FIX   := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.fixp.o)))
LL_FILES_ERR    := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.err.ll)))

.PHONY: all clean

all: DIR $(MODULE)

.PRECIOUS: obj/%.ll obj/%.o obj/%.s obj/%.init.ll obj/%.fixp.ll

DIR:
	@echo ${CPP_FILES}
	@echo ${OBJ_FILES}
	if [ ! -d "./bin" ];then 	\
		mkdir bin;				\
	fi
	if [ ! -d "./obj" ];then 	\
		mkdir obj;				\
	fi

$(MODULE): $(OBJ_FILES) $(OBJ_FILES_FIX) $(LL_FILES_ERR)
	$(LD) $(OBJ_FILES) $(LIB) $(LFLAGS) -o bin/$@
	$(LD) $(OBJ_FILES_FIX) $(LIB) $(LFLAGS) -o bin/$@.fixp


obj/%.ll: src/%.c
	$(CC) -S -emit-llvm $< -o $@

obj/%.init.ll: obj/%.ll
	$(LLVM_DIR)/bin/opt -load=$(INITLIB) -S -taffoinit $(if $(ISDEBUG), -debug-only=flttofix$(comma)annotation) -stats $< -o $@

obj/%.fixp.ll: obj/%.init.ll
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce -mem2reg $(if $(ISDEBUG), -debug-only=flttofix) -stats $< -o $@

obj/%.err.ll: obj/%.fixp.ll
	$(LLVM_DIR)/bin/opt -load=$(ERRORLIB) -S -errorprop -startonly $(if $(ISDEBUG), -debug-only=errorprop) $< -o $@

obj/%.s: obj/%.ll
	$(CC) -S  $< -o $@ $(OX)

obj/%.o: obj/%.s
	$(CC) -c  $< -o $@

clean:
	@rm -rf *.o
	@rm -rf *.d
	@rm -rf *.out
	@rm -rf bin
	@rm -rf obj
	@rm -rf *.ll
	@rm -rf *.s
