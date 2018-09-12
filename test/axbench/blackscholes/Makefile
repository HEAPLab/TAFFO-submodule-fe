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
LFLAGS		+= #-lboost_regex 
HEADERS		:= src
INCLUDE		+= -I${HEADERS} -I../common/src
LIB		:= 

MODULE		:= blackscholes.out

CPP_FILES       := blackscholes.c
OBJ_FILES       := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.o))) 
OBJ_FILES_FIX   := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.fixp.o)))
LL_FILES_ERR    := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.err.ll)))
FLT_IMIX_STAT   := $(addprefix stats/,$(notdir $(CPP_FILES:.c=.mix.txt)))
FIX_IMIX_STAT   := $(addprefix stats/,$(notdir $(CPP_FILES:.c=.fixp.mix.txt)))
LLVM_STAT       := $(addprefix stats/,$(notdir $(CPP_FILES:.c=.llvm.txt)))

.PHONY: all clean

all: DIR $(MODULE)

.PRECIOUS: obj/%.ll obj/%.o obj/%.s obj/%.init.ll obj/%.fixp.ll

DIR:
	@echo ${CPP_FILES}
	@echo ${OBJ_FILES}
	mkdir -p bin
	mkdir -p obj
	mkdir -p stats


$(MODULE): $(OBJ_FILES) $(OBJ_FILES_FIX) $(LL_FILES_ERR) $(FLT_IMIX_STAT) $(FIX_IMIX_STAT) $(LLVM_STAT)
	$(LD) $(OBJ_FILES) $(LIB) $(LFLAGS) -o bin/$@
	$(LD) $(OBJ_FILES_FIX) $(LIB) $(LFLAGS) -o bin/$@.fixp


obj/%.ll: src/%.c
	$(CC) -S -emit-llvm $(CFLAGS) $(INCLUDE) $< -o $@

obj/%.init.ll: obj/%.ll
	$(LLVM_DIR)/bin/opt -load=$(INITLIB) -S -taffoinit $(if $(ISDEBUG), -debug-only=flttofix$(comma)annotation) -stats $< -o $@ 2> stats/init.log

obj/%.fixp.ll: obj/%.init.ll
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce -mem2reg $(if $(ISDEBUG), -debug-only=flttofix) -stats $< -o $@ 2> stats/flttofix.log

obj/%.err.ll: obj/%.fixp.ll
	$(LLVM_DIR)/bin/opt -load=$(ERRORLIB) -S -errorprop -startonly $(if $(ISDEBUG), -debug-only=errorprop) $< -o $@ 2> stats/errorprop.log

obj/%.s: obj/%.ll
	$(CC) -S  $< -o $@ $(OX)

obj/%.o: obj/%.s
	$(CC) -c  $< -o $@

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
	rm -rf *.ll
	rm -rf *.s
	rm -rf stats
