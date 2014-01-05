DECODER_PREFIX=DECODER
DECODER_SRCDIR=codec/decoder
DECODER_CPP_SRCS=\
	$(DECODER_SRCDIR)/./core/src/au_parser.cpp\
	$(DECODER_SRCDIR)/./core/src/bit_stream.cpp\
	$(DECODER_SRCDIR)/./core/src/cpu.cpp\
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

OBJS += $(DECODER_OBJS)
$(DECODER_SRCDIR)/./core/src/au_parser.o: $(DECODER_SRCDIR)/./core/src/au_parser.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/au_parser.o $(DECODER_SRCDIR)/./core/src/au_parser.cpp

$(DECODER_SRCDIR)/./core/src/bit_stream.o: $(DECODER_SRCDIR)/./core/src/bit_stream.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/bit_stream.o $(DECODER_SRCDIR)/./core/src/bit_stream.cpp

$(DECODER_SRCDIR)/./core/src/cpu.o: $(DECODER_SRCDIR)/./core/src/cpu.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/cpu.o $(DECODER_SRCDIR)/./core/src/cpu.cpp

$(DECODER_SRCDIR)/./core/src/deblocking.o: $(DECODER_SRCDIR)/./core/src/deblocking.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/deblocking.o $(DECODER_SRCDIR)/./core/src/deblocking.cpp

$(DECODER_SRCDIR)/./core/src/decode_mb_aux.o: $(DECODER_SRCDIR)/./core/src/decode_mb_aux.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/decode_mb_aux.o $(DECODER_SRCDIR)/./core/src/decode_mb_aux.cpp

$(DECODER_SRCDIR)/./core/src/decode_slice.o: $(DECODER_SRCDIR)/./core/src/decode_slice.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/decode_slice.o $(DECODER_SRCDIR)/./core/src/decode_slice.cpp

$(DECODER_SRCDIR)/./core/src/decoder.o: $(DECODER_SRCDIR)/./core/src/decoder.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/decoder.o $(DECODER_SRCDIR)/./core/src/decoder.cpp

$(DECODER_SRCDIR)/./core/src/decoder_core.o: $(DECODER_SRCDIR)/./core/src/decoder_core.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/decoder_core.o $(DECODER_SRCDIR)/./core/src/decoder_core.cpp

$(DECODER_SRCDIR)/./core/src/decoder_data_tables.o: $(DECODER_SRCDIR)/./core/src/decoder_data_tables.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/decoder_data_tables.o $(DECODER_SRCDIR)/./core/src/decoder_data_tables.cpp

$(DECODER_SRCDIR)/./core/src/expand_pic.o: $(DECODER_SRCDIR)/./core/src/expand_pic.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/expand_pic.o $(DECODER_SRCDIR)/./core/src/expand_pic.cpp

$(DECODER_SRCDIR)/./core/src/fmo.o: $(DECODER_SRCDIR)/./core/src/fmo.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/fmo.o $(DECODER_SRCDIR)/./core/src/fmo.cpp

$(DECODER_SRCDIR)/./core/src/get_intra_predictor.o: $(DECODER_SRCDIR)/./core/src/get_intra_predictor.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/get_intra_predictor.o $(DECODER_SRCDIR)/./core/src/get_intra_predictor.cpp

$(DECODER_SRCDIR)/./core/src/manage_dec_ref.o: $(DECODER_SRCDIR)/./core/src/manage_dec_ref.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/manage_dec_ref.o $(DECODER_SRCDIR)/./core/src/manage_dec_ref.cpp

$(DECODER_SRCDIR)/./core/src/mc.o: $(DECODER_SRCDIR)/./core/src/mc.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/mc.o $(DECODER_SRCDIR)/./core/src/mc.cpp

$(DECODER_SRCDIR)/./core/src/mem_align.o: $(DECODER_SRCDIR)/./core/src/mem_align.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/mem_align.o $(DECODER_SRCDIR)/./core/src/mem_align.cpp

$(DECODER_SRCDIR)/./core/src/memmgr_nal_unit.o: $(DECODER_SRCDIR)/./core/src/memmgr_nal_unit.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/memmgr_nal_unit.o $(DECODER_SRCDIR)/./core/src/memmgr_nal_unit.cpp

$(DECODER_SRCDIR)/./core/src/mv_pred.o: $(DECODER_SRCDIR)/./core/src/mv_pred.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/mv_pred.o $(DECODER_SRCDIR)/./core/src/mv_pred.cpp

$(DECODER_SRCDIR)/./core/src/parse_mb_syn_cavlc.o: $(DECODER_SRCDIR)/./core/src/parse_mb_syn_cavlc.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/parse_mb_syn_cavlc.o $(DECODER_SRCDIR)/./core/src/parse_mb_syn_cavlc.cpp

$(DECODER_SRCDIR)/./core/src/pic_queue.o: $(DECODER_SRCDIR)/./core/src/pic_queue.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/pic_queue.o $(DECODER_SRCDIR)/./core/src/pic_queue.cpp

$(DECODER_SRCDIR)/./core/src/rec_mb.o: $(DECODER_SRCDIR)/./core/src/rec_mb.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/rec_mb.o $(DECODER_SRCDIR)/./core/src/rec_mb.cpp

$(DECODER_SRCDIR)/./core/src/utils.o: $(DECODER_SRCDIR)/./core/src/utils.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./core/src/utils.o $(DECODER_SRCDIR)/./core/src/utils.cpp

$(DECODER_SRCDIR)/./plus/src/welsCodecTrace.o: $(DECODER_SRCDIR)/./plus/src/welsCodecTrace.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./plus/src/welsCodecTrace.o $(DECODER_SRCDIR)/./plus/src/welsCodecTrace.cpp

$(DECODER_SRCDIR)/./plus/src/welsDecoderExt.o: $(DECODER_SRCDIR)/./plus/src/welsDecoderExt.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(DECODER_CFLAGS) $(DECODER_INCLUDES) -c -o $(DECODER_SRCDIR)/./plus/src/welsDecoderExt.o $(DECODER_SRCDIR)/./plus/src/welsDecoderExt.cpp

$(DECODER_SRCDIR)/./core/asm/block_add.o: $(DECODER_SRCDIR)/./core/asm/block_add.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(DECODER_ASMFLAGS) $(DECODER_ASM_INCLUDES) -o $(DECODER_SRCDIR)/./core/asm/block_add.o $(DECODER_SRCDIR)/./core/asm/block_add.asm

$(DECODER_SRCDIR)/./core/asm/dct.o: $(DECODER_SRCDIR)/./core/asm/dct.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(DECODER_ASMFLAGS) $(DECODER_ASM_INCLUDES) -o $(DECODER_SRCDIR)/./core/asm/dct.o $(DECODER_SRCDIR)/./core/asm/dct.asm

$(DECODER_SRCDIR)/./core/asm/intra_pred.o: $(DECODER_SRCDIR)/./core/asm/intra_pred.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(DECODER_ASMFLAGS) $(DECODER_ASM_INCLUDES) -o $(DECODER_SRCDIR)/./core/asm/intra_pred.o $(DECODER_SRCDIR)/./core/asm/intra_pred.asm

$(LIBPREFIX)decoder.$(LIBSUFFIX): $(DECODER_OBJS)
	rm -f $(LIBPREFIX)decoder.$(LIBSUFFIX)
	$(AR) cr $@ $(DECODER_OBJS)

libraries: $(LIBPREFIX)decoder.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)decoder.$(LIBSUFFIX)
