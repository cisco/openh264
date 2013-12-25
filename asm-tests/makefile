NASM = Yes
TARGET = asm_tests
OUTDIR    = .
BINDIR    = ../../bin
OBJDIR    = .
SRCDIRS  = .



CC        = $(shell which gcc)
AS        = $(shell which nasm)
GCC       = gcc 
LDFLAGS   = -lstdc++ -ldl
ifeq ($(ENABLE64BIT), Yes)
GCC += -m64
ASMFLAGS = -f macho64 -DUNIX64 -DPREFIX
LDFLAGS += libgtest_64.a
else
GCC += -m32
ASMFLAGS = -f macho32  -DX86_32 -DPREFIX
LDFLAGS += libgtest_32.a
endif

CPPFLAGS  = -Wall -g -O3 -fpic -I ../gtest/include
ifeq ($(NASM), Yes)
CPPFLAGS += -DX86_ASM
endif

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
	$(LINK) $(OBJS) -o $@

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f $(DEPS) TAGS

