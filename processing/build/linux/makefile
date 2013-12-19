NASM = 1
NAME      = libwelsvp

OUTDIR    = ../../../bin/linux
BINDIR    = ../../bin
OBJDIR    = ../../obj
SRCDIRS   = ../../src/asm \
            ../../src/common \
            ../../src/adaptivequantization \
            ../../src/backgounddetection \
            ../../src/denoise \
            ../../src/downsample \
            ../../src/scenechangedetection \
            ../../src/vaacalc \
            ../../src/complexityanalysis
SRCDIRS  += ../../src/imagerotate


TARGETLIB =  $(BINDIR)/$(NAME).so

CC        = $(shell which gcc)
AS        = $(shell which nasm)
GCC       = gcc 
ifeq ($(ENABLE64BIT), Yes)
GCC += -m64
NASM = 0
else
GCC += -m32
endif

CPPFLAGS  = -Wall -g -O3 -fpic
ifeq ($(NASM), 1)
CPPFLAGS += -DX86_ASM
endif
ASMFLAGS  = -f elf -DNOPREFIX  -I ../../src/asm/
LDFLAGS   = -lstdc++ -ldl

SRCEXTS  = .cpp
ifeq ($(NASM), 1)
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

all: $(TARGETLIB)

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

tags: $(HEADERS) $(SOURCES)
	etags $(HEADERS) $(SOURCES)

ctags: $(HEADERS) $(SOURCES)
	ctags $(HEADERS) $(SOURCES)

$(TARGETLIB):$(OBJS)
	@if test ! -d $(BINDIR) ; then mkdir -p $(BINDIR) ; fi
	$(LINK) $(OBJS) -shared -Wl,-Bsymbolic -o $@
	@echo produce the lib to $(TARGETLIB).
	@if test ! -d $(OUTDIR) ; then mkdir -p $(OUTDIR) ; fi
	@cp -f $(TARGETLIB) $(OUTDIR)
	@cp -f $(TARGETLIB) ../../../testbin
	@echo copy the lib to $(OUTDIR).

clean:
	rm -f $(OBJS) $(TARGETLIB)

distclean: clean
	rm -f $(DEPS) TAGS

