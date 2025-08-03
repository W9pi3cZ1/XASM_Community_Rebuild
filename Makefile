ifeq ($(VERBOSE), 1)
  V=
  Q=
else
  V=@printf "\033[1;32m[Build]\033[0m $@ ...\n";
  Q=@
endif

C_SOURCES      := $(shell find ./src/include -name "*.c")
ALL_C_SOURCES  := $(shell find ./src -name "*.c")
S_SOURCES      := $(shell find ./src/include -name "*.s")
HEADERS        := $(shell find ./src/include -name "*.h")
OBJS           := $(C_SOURCES:%.c=%.o) $(S_SOURCES:%.s=%.o)
DEPS           := $(OBJS:%.o=%.d)
LIBS           := $(wildcard libs/lib*.a)
PREFIX		 := build
PWD            := $(shell pwd)

CHECKS         := -quiet -checks=-*,clang-analyzer-*,bugprone-*,cert-*,misc-*,performance-*,portability-*,-misc-include-cleaner,-clang-analyzer-security.insecureAPI.*

C_FLAGS        := -Wall -Wextra -O2 -Isrc/include -MMD
LD_FLAGS       := 
AS_FLAGS       := 

all: build

%.o: %.c
	$(V)$(CC) $(C_FLAGS) -c -o $@ $<

%.o: %.s
	$(V)$(AS) $(AS_FLAGS) -o $@ $<

%.fmt: %
	$(Q)printf "\033[1;32m[Format]\033[0m $< ...\n"
	$(Q)clang-format -i $<

compiler:$(OBJS) $(LIBS)
	$(V)$(CC) $(C_FLAGS) -c -o src/$@.o src/$@.c
	$(Q)$(CC) $(LD_FLAGS) -o $(PREFIX)/$@ $^ src/$@.o

runner: $(OBJS) $(LIBS)
	$(V)$(CC) $(C_FLAGS) -c -o src/$@.o src/$@.c
	$(Q)$(CC) $(LD_FLAGS) -o $(PREFIX)/$@ $^ src/$@.o

build: compiler runner
	$(V)printf "\033[1;32m[Done]\033[0m Build completed.\n\n"

format: $(ALL_C_SOURCES:%=%.fmt) $(S_SOURCES:%=%.fmt) $(HEADERS:%=%.fmt)
	$(Q)printf "\033[1;32m[Done]\033[0m Code Format complete.\n\n"

clean:
	$(Q)$(RM) $(OBJS) $(DEPS) $(shell find ./src/include -name "*.o") $(shell find ./src/include -name "*.d")
	$(Q)printf "\033[1;32m[Done]\033[0m Clean completed.\n\n"

-include $(DEPS)
