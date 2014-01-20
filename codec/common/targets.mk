COMMON_PREFIX=COMMON
COMMON_SRCDIR=codec/common
COMMON_CPP_SRCS=\
	$(COMMON_SRCDIR)/./cpu.cpp\
	$(COMMON_SRCDIR)/./deblocking_common.cpp\
	$(COMMON_SRCDIR)/./logging.cpp\

COMMON_OBJS += $(COMMON_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
COMMON_ASM_SRCS=\
	$(COMMON_SRCDIR)/./asm_inc.asm\
	$(COMMON_SRCDIR)/./cpuid.asm\
	$(COMMON_SRCDIR)/./deblock.asm\
	$(COMMON_SRCDIR)/./expand_picture.asm\
	$(COMMON_SRCDIR)/./mb_copy.asm\
	$(COMMON_SRCDIR)/./mc_chroma.asm\
	$(COMMON_SRCDIR)/./mc_luma.asm\
	$(COMMON_SRCDIR)/./vaa.asm\

COMMON_OBJS += $(COMMON_ASM_SRCS:.asm=.o)
endif

$(COMMON_SRCDIR)/%.o: $(COMMON_SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c $(CXX_O) $<

$(COMMON_SRCDIR)/%.o: $(COMMON_SRCDIR)/%.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $@ $<

$(LIBPREFIX)common.$(LIBSUFFIX): $(COMMON_OBJS)
	rm -f $(LIBPREFIX)common.$(LIBSUFFIX)
	$(AR) $(AR_OPTS) $(COMMON_OBJS)

libraries: $(LIBPREFIX)common.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)common.$(LIBSUFFIX)
