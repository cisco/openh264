COMMON_SRCDIR=codec/common
COMMON_CPP_SRCS=\
	$(COMMON_SRCDIR)/src/cpu.cpp\
	$(COMMON_SRCDIR)/src/crt_util_safe_x.cpp\
	$(COMMON_SRCDIR)/src/deblocking_common.cpp\
	$(COMMON_SRCDIR)/src/logging.cpp\
	$(COMMON_SRCDIR)/src/sad_common.cpp\
	$(COMMON_SRCDIR)/src/WelsThreadLib.cpp\

COMMON_OBJS += $(COMMON_CPP_SRCS:.cpp=.$(OBJ))

ifeq ($(ASM_ARCH), x86)
COMMON_ASM_SRCS=\
	$(COMMON_SRCDIR)/x86/cpuid.asm\
	$(COMMON_SRCDIR)/x86/deblock.asm\
	$(COMMON_SRCDIR)/x86/expand_picture.asm\
	$(COMMON_SRCDIR)/x86/mb_copy.asm\
	$(COMMON_SRCDIR)/x86/mc_chroma.asm\
	$(COMMON_SRCDIR)/x86/mc_luma.asm\
	$(COMMON_SRCDIR)/x86/satd_sad.asm\
	$(COMMON_SRCDIR)/x86/vaa.asm\

COMMON_OBJS += $(COMMON_ASM_SRCS:.asm=.$(OBJ))
endif

ifeq ($(ASM_ARCH), arm)
COMMON_ASM_S_SRCS=\
	$(COMMON_SRCDIR)/arm/deblocking_neon.S\
	$(COMMON_SRCDIR)/arm/expand_picture_neon.S\
	$(COMMON_SRCDIR)/arm/mc_neon.S\

COMMON_OBJS += $(COMMON_ASM_S_SRCS:.S=.$(OBJ))
endif

OBJS += $(COMMON_OBJS)
$(COMMON_SRCDIR)/%.$(OBJ): $(COMMON_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c $(CXX_O) $<

$(COMMON_SRCDIR)/%.$(OBJ): $(COMMON_SRCDIR)/%.asm
	$(QUIET_ASM)$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $@ $<

$(COMMON_SRCDIR)/%.$(OBJ): $(COMMON_SRCDIR)/%.S
	$(QUIET_CCAS)$(CCAS) $(CFLAGS) $(ASMFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c -o $@ $<

$(LIBPREFIX)common.$(LIBSUFFIX): $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

libraries: $(LIBPREFIX)common.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)common.$(LIBSUFFIX)
