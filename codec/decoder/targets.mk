DECODER_SRCDIR=codec/decoder
DECODER_CPP_SRCS=\
	$(DECODER_SRCDIR)/core/src/au_parser.cpp\
	$(DECODER_SRCDIR)/core/src/bit_stream.cpp\
	$(DECODER_SRCDIR)/core/src/deblocking.cpp\
	$(DECODER_SRCDIR)/core/src/decode_mb_aux.cpp\
	$(DECODER_SRCDIR)/core/src/decode_slice.cpp\
	$(DECODER_SRCDIR)/core/src/decoder.cpp\
	$(DECODER_SRCDIR)/core/src/decoder_core.cpp\
	$(DECODER_SRCDIR)/core/src/decoder_data_tables.cpp\
	$(DECODER_SRCDIR)/core/src/error_concealment.cpp\
	$(DECODER_SRCDIR)/core/src/expand_pic.cpp\
	$(DECODER_SRCDIR)/core/src/fmo.cpp\
	$(DECODER_SRCDIR)/core/src/get_intra_predictor.cpp\
	$(DECODER_SRCDIR)/core/src/manage_dec_ref.cpp\
	$(DECODER_SRCDIR)/core/src/mc.cpp\
	$(DECODER_SRCDIR)/core/src/mem_align.cpp\
	$(DECODER_SRCDIR)/core/src/memmgr_nal_unit.cpp\
	$(DECODER_SRCDIR)/core/src/mv_pred.cpp\
	$(DECODER_SRCDIR)/core/src/parse_mb_syn_cavlc.cpp\
	$(DECODER_SRCDIR)/core/src/pic_queue.cpp\
	$(DECODER_SRCDIR)/core/src/rec_mb.cpp\
	$(DECODER_SRCDIR)/core/src/utils.cpp\
	$(DECODER_SRCDIR)/plus/src/welsCodecTrace.cpp\
	$(DECODER_SRCDIR)/plus/src/welsDecoderExt.cpp\

DECODER_OBJS += $(DECODER_CPP_SRCS:.cpp=.$(OBJ))

ifeq ($(ASM_ARCH), x86)
DECODER_ASM_SRCS=\
	$(DECODER_SRCDIR)/core/x86/dct.asm\
	$(DECODER_SRCDIR)/core/x86/intra_pred.asm\

DECODER_OBJS += $(DECODER_ASM_SRCS:.asm=.$(OBJ))
endif

ifeq ($(ASM_ARCH), arm)
DECODER_ASM_S_SRCS=\
	$(DECODER_SRCDIR)/core/arm/block_add_neon.S\
	$(DECODER_SRCDIR)/core/arm/intra_pred_neon.S\

DECODER_OBJS += $(DECODER_ASM_S_SRCS:.S=.$(OBJ))
endif

OBJS += $(DECODER_OBJS)
$(DECODER_SRCDIR)/%.$(OBJ): $(DECODER_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c $(CXX_O) $<

$(DECODER_SRCDIR)/%.$(OBJ): $(DECODER_SRCDIR)/%.asm
	$(QUIET_ASM)$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(DECODER_ASMFLAGS) $(DECODER_ASM_INCLUDES) -o $@ $<

$(DECODER_SRCDIR)/%.$(OBJ): $(DECODER_SRCDIR)/%.S
	$(QUIET_CCAS)$(CCAS) $(CFLAGS) $(ASMFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $@ $<

$(LIBPREFIX)decoder.$(LIBSUFFIX): $(DECODER_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

libraries: $(LIBPREFIX)decoder.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)decoder.$(LIBSUFFIX)
