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

MIOSIX_EABI     := /opt/arm-miosix-eabi/arm-miosix-eabi
MIOSIX_CFLAGS   := -target arm-none-linux-eabi --sysroot=$(MIOSIX_EABI) -I$(MIOSIX_EABI)/include/ -I$(MIOSIX_EABI)/include/c++/4.7.3/ -I$(MIOSIX_EABI)/include/c++/4.7.3/arm-miosix-eabi/ -D_MIOSIX -D_MIOSIX_GCC_PATCH_VERSION=1 -mcpu=cortex-m3 -mthumb -ffunction-sections -Wall

MODULE		:= kmeans.out

CPP_FILES := $(wildcard src/*.c)
LL_FILES  := $(addprefix obj/,$(notdir $(CPP_FILES:.c=.ll))) 
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

obj/%.ll : src/%.c
	$(CC) -S -emit-llvm $(MIOSIX_FLAGS) $(CFLAGS) $(INCLUDE) $< -o $@

obj/main.ll : $(LL_FILES)
	$(LLVM_DIR)/bin/llvm-link -S $^ -o $@

$(OBJ_FLOAT) : obj/main.ll
	$(CC) -S $(MIOSIX_FLAGS) $^ -o obj/main.s $(OX)
	$(CC) -c $(MIOSIX_FLAGS) obj/main.s -o $@

obj/main.init.ll : obj/main.ll
	$(LLVM_DIR)/bin/opt -load=$(INITLIB) -S -taffoinit $(if $(ISDEBUG), -debug-only=flttofix$(comma)annotation) -stats $< -o $@ 2> stats/init.log

obj/main.fixp.ll : obj/main.init.ll
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce -mem2reg -stats $< -o $@ 2> stats/flttofix.log

obj/main.err.ll : obj/main.fixp.ll
	$(LLVM_DIR)/bin/opt -load=$(ERRORLIB) -S -errorprop -startonly $(if $(ISDEBUG), -debug-only=errorprop) $< -o $@ 2> stats/errorprop.log

$(OBJ_FIX) : obj/main.fixp.ll
	$(CC) -S $(MIOSIX_FLAGS) $^ -o obj/main.fixp.s $(OX)
	$(CC) -c $(MIOSIX_FLAGS) obj/main.fixp.s -o $@

stats/%.mix.txt: obj/%.ll
	$(INSTMIX) $< > $@

stats/%.llvm.txt: obj/%.ll
	$(LLVM_DIR)/bin/opt -load=$(PASSLIB) -S -flttofix -dce -stats $< -o /dev/null 2> $@

miosix_cflags:
	$(eval MIOSIX_FLAGS += $(MIOSIX_CFLAGS))

miosix_o: miosix_cflags DIR $(OBJ_FLOAT) $(OBJ_FIX) obj/main.err.ll stats/main.fixp.mix.txt stats/main.mix.txt stats/main.llvm.txt
	cp obj/main.o $(MIOSIX_ROOT)/$(basename $(MODULE)).o
	cp obj/main.fixp.o $(MIOSIX_ROOT)/$(basename $(MODULE)).fixp.o

miosix_float: miosix_o
	make clean -C $(MIOSIX_ROOT)
	make BENCH=$(basename $(MODULE)).o -j 8 -C $(MIOSIX_ROOT)
	#make program -C $(MIOSIX_ROOT)

miosix_fix: miosix_o
	make clean -C $(MIOSIX_ROOT)
	make BENCH=$(basename $(MODULE)).fixp.o -j 8 -C $(MIOSIX_ROOT)
	#make program -C $(MIOSIX_ROOT)

clean:
	rm -rf *.o
	rm -rf *.d
	rm -rf *.out
	rm -rf bin
	rm -rf obj
	rm -rf stats
