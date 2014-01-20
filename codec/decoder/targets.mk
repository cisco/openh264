DECODER_PREFIX=DECODER
DECODER_SRCDIR=codec/decoder
DECODER_CPP_SRCS=\
	$(DECODER_SRCDIR)/./core/src/au_parser.cpp\
	$(DECODER_SRCDIR)/./core/src/bit_stream.cpp\
	$(DECODER_SRCDIR)/./core/src/deblocking.cpp\
	$(DECODER_SRCDIR)/./core/src/decode_mb_aux.cpp\
	$(DECODER_SRCDIR)/./core/src/decode_slice.cpp\
	$(DECODER_SRCDIR)/./core/src/decoder.cpp\
	$(DECODER_SRCDIR)/./core/src/decoder_core.cpp\
	$(DECODER_SRCDIR)/./core/src/decoder_data_tables.cpp\
	$(DECODER_SRCDIR)/./core/src/expand_pic.cpp\
	$(DECODER_SRCDIR)/./core/src/fmo.cpp\
	$(DECODER_SRCDIR)/./core/src/get_intra_predictor.cpp\
	$(DECODER_SRCDIR)/./core/src/manage_dec_ref.cpp\
	$(DECODER_SRCDIR)/./core/src/mc.cpp\
	$(DECODER_SRCDIR)/./core/src/mem_align.cpp\
	$(DECODER_SRCDIR)/./core/src/memmgr_nal_unit.cpp\
	$(DECODER_SRCDIR)/./core/src/mv_pred.cpp\
	$(DECODER_SRCDIR)/./core/src/parse_mb_syn_cavlc.cpp\
	$(DECODER_SRCDIR)/./core/src/pic_queue.cpp\
	$(DECODER_SRCDIR)/./core/src/rec_mb.cpp\
	$(DECODER_SRCDIR)/./core/src/utils.cpp\
	$(DECODER_SRCDIR)/./plus/src/welsCodecTrace.cpp\
	$(DECODER_SRCDIR)/./plus/src/welsDecoderExt.cpp\

DECODER_OBJS += $(DECODER_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
DECODER_ASM_SRCS=\
	$(DECODER_SRCDIR)/./core/asm/block_add.asm\
	$(DECODER_SRCDIR)/./core/asm/dct.asm\
	$(DECODER_SRCDIR)/./core/asm/intra_pred.asm\

DECODER_OBJS += $(DECODER_ASM_SRCS:.asm=.o)
endif

$(DECODER_SRCDIR)/%.o: $(DECODER_SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c $(CXX_O) $<

$(DECODER_SRCDIR)/%.o: $(DECODER_SRCDIR)/%.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(DECODER_ASMFLAGS) $(DECODER_ASM_INCLUDES) -o $@ $<

$(LIBPREFIX)decoder.$(LIBSUFFIX): $(DECODER_OBJS)
	rm -f $(LIBPREFIX)decoder.$(LIBSUFFIX)
	$(AR) $(AR_OPTS) $(DECODER_OBJS)

libraries: $(LIBPREFIX)decoder.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)decoder.$(LIBSUFFIX)
