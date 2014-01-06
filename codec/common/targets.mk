COMMON_PREFIX=COMMON
COMMON_SRCDIR=codec/common
COMMON_CPP_SRCS=\
	$(COMMON_SRCDIR)/./logging.cpp\
	$(COMMON_SRCDIR)/./cpu.cpp\

COMMON_OBJS += $(COMMON_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
COMMON_ASM_SRCS=\
	$(COMMON_SRCDIR)/./deblock.asm\
	$(COMMON_SRCDIR)/./vaa.asm\
	$(COMMON_SRCDIR)/./asm_inc.asm\
	$(COMMON_SRCDIR)/./mc_luma.asm\
	$(COMMON_SRCDIR)/./cpuid.asm\
	$(COMMON_SRCDIR)/./mc_chroma.asm\
	$(COMMON_SRCDIR)/./mb_copy.asm\
	$(COMMON_SRCDIR)/./expand_picture.asm\

COMMON_OBJS += $(COMMON_ASM_SRCS:.asm=.o)
endif

OBJS += $(COMMON_OBJS)
$(COMMON_SRCDIR)/./logging.o: $(COMMON_SRCDIR)/./logging.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c -o $(COMMON_SRCDIR)/./logging.o $(COMMON_SRCDIR)/./logging.cpp

$(COMMON_SRCDIR)/./cpu.o: $(COMMON_SRCDIR)/./cpu.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c -o $(COMMON_SRCDIR)/./cpu.o $(COMMON_SRCDIR)/./cpu.cpp

$(COMMON_SRCDIR)/./deblock.o: $(COMMON_SRCDIR)/./deblock.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./deblock.o $(COMMON_SRCDIR)/./deblock.asm

$(COMMON_SRCDIR)/./vaa.o: $(COMMON_SRCDIR)/./vaa.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./vaa.o $(COMMON_SRCDIR)/./vaa.asm

$(COMMON_SRCDIR)/./asm_inc.o: $(COMMON_SRCDIR)/./asm_inc.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./asm_inc.o $(COMMON_SRCDIR)/./asm_inc.asm

$(COMMON_SRCDIR)/./mc_luma.o: $(COMMON_SRCDIR)/./mc_luma.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./mc_luma.o $(COMMON_SRCDIR)/./mc_luma.asm

$(COMMON_SRCDIR)/./cpuid.o: $(COMMON_SRCDIR)/./cpuid.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./cpuid.o $(COMMON_SRCDIR)/./cpuid.asm

$(COMMON_SRCDIR)/./mc_chroma.o: $(COMMON_SRCDIR)/./mc_chroma.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./mc_chroma.o $(COMMON_SRCDIR)/./mc_chroma.asm

$(COMMON_SRCDIR)/./mb_copy.o: $(COMMON_SRCDIR)/./mb_copy.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./mb_copy.o $(COMMON_SRCDIR)/./mb_copy.asm

$(COMMON_SRCDIR)/./expand_picture.o: $(COMMON_SRCDIR)/./expand_picture.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(COMMON_ASMFLAGS) $(COMMON_ASM_INCLUDES) -o $(COMMON_SRCDIR)/./expand_picture.o $(COMMON_SRCDIR)/./expand_picture.asm

$(LIBPREFIX)common.$(LIBSUFFIX): $(COMMON_OBJS)
	rm -f $(LIBPREFIX)common.$(LIBSUFFIX)
	$(AR) cr $@ $(COMMON_OBJS)

libraries: $(LIBPREFIX)common.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)common.$(LIBSUFFIX)
