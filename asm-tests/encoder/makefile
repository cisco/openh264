NASM = Yes
TARGET = asm_tests
OUTDIR    = .
BINDIR    = ../../bin
OBJDIR    = .
SRCDIRS  = .

SYSTEM = $(shell uname)
CC        = $(shell which gcc)
AS        = $(shell which nasm)
GCC       = gcc 
ifeq ($(ENABLE64BIT), Yes)
GCC += -m64
ASMFLAGS = -g -DUNIX64 -I ../
LDFLAGS += ../libgtest_64.a

ifeq ($(SYSTEM), Darwin)
ASMFLAGS += -f macho64
else #Linux
ASMFLAGS += -f elf64
endif

else
GCC += -m32
ASMFLAGS =  -g -DX86_32  -I ../

ifeq ($(SYSTEM), Darwin)
ASMFLAGS += -f macho32
else # Linux
ASMFLAGS += -f elf
endif

LDFLAGS += ../libgtest_32.a
endif

LDFLAGS   += -lstdc++ -ldl -lpthread
ifeq ($(SYSTEM), Darwin)
ASMFLAGS += -DPREFIX
endif
CPPFLAGS  = -Wall -g -fpic -I ../gtest/include -I ../../codec/encoder/core/inc -I ../../codec/api/svc
ifeq ($(NASM), Yes)
CPPFLAGS += -DX86_ASM
endif
CPPFLAGS += -msse2 -O2
SRCEXTS  = .cpp
ifeq ($(NASM), Yes)
SRCEXTS += .asm
endif
HDREXTS  = .h
SOURCES  = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
HEADERS  = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(HDREXTS))))
SRC_CPP  = $(filter %.cpp,$(SOURCES))
SRC_ASM  = $(filter %.asm,$(SOURCES))
OBJS     = $(addsuffix .o, $(basename $(SOURCES)))
DEPS     = $(OBJS:.o=.d)

DEP_OPT  = $(shell if `$(CC) --version | grep "GCC" >/dev/null`; then \
                  echo "-MM -MP"; else echo "-M"; fi )
DEPEND_cpp.d  = $(subst -g ,,$(CC) $(DEP_OPT) $(CPPFLAGS))
DEPEND_asm.d  = $(subst -g ,,$(AS) $(DEP_OPT) $(ASMFLAGS))
COMPILE.cpp   = $(GCC) $(CPPFLAGS) -c
COMPILE.asm   = $(AS)  $(ASMFLAGS)
LINK          = $(GCC) $(LDFLAGS)

.PHONY: all objs tags ctags clean distclean

.SUFFIXES:

all: $(TARGET)

%.d:%.cpp
	@echo -n $(dir $<) > $@
	@$(DEPEND_cpp.d) $< >> $@

%.d:%.asm
	@echo -n $(dir $<) > $@
	@$(DEPEND_asm.d) $< >> $@

objs:$(OBJS)

%.o:%.cpp
	$(COMPILE.cpp) $< -o $@

%.o:%.asm
	$(COMPILE.asm) $< -o $@

$(TARGET):$(OBJS)
	$(GCC) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f $(DEPS) TAGS

