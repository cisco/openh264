COMMON_SRCDIR=codec/common
COMMON_CPP_SRCS=\
	$(COMMON_SRCDIR)/cpu.cpp\
	$(COMMON_SRCDIR)/crt_util_safe_x.cpp\
	$(COMMON_SRCDIR)/deblocking_common.cpp\
	$(COMMON_SRCDIR)/logging.cpp\
	$(COMMON_SRCDIR)/WelsThreadLib.cpp\

COMMON_OBJS += $(COMMON_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
COMMON_ASM_SRCS=\
	$(COMMON_SRCDIR)/asm_inc.asm\
	$(COMMON_SRCDIR)/cpuid.asm\
	$(COMMON_SRCDIR)/deblock.asm\
	$(COMMON_SRCDIR)/expand_picture.asm\
	$(COMMON_SRCDIR)/mb_copy.asm\
	$(COMMON_SRCDIR)/mc_chroma.asm\
	$(COMMON_SRCDIR)/mc_luma.asm\
	$(COMMON_SRCDIR)/satd_sad.asm\
	$(COMMON_SRCDIR)/vaa.asm\

COMMON_OBJS += $(COMMON_ASM_SRCS:.asm=.o)
endif

OBJS += $(COMMON_OBJS)
$(COMMON_SRCDIR)/%.o: $(COMMON_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c $(CXX_O) $<

$(COMMON_SRCDIR)/%.o: $(COMMON_SRCDIR)/%.asm
	$(QUIET_ASM)$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $@ $<

$(LIBPREFIX)common.$(LIBSUFFIX): $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

libraries: $(LIBPREFIX)common.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)common.$(LIBSUFFIX)
