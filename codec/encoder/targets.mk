ENCODER_PREFIX=ENCODER
ENCODER_SRCDIR=codec/encoder
ENCODER_CPP_SRCS=\
	$(ENCODER_SRCDIR)/./core/src/mc.cpp\
	$(ENCODER_SRCDIR)/./core/src/slice_multi_threading.cpp\
	$(ENCODER_SRCDIR)/./core/src/wels_preprocess.cpp\
	$(ENCODER_SRCDIR)/./core/src/ratectl.cpp\
	$(ENCODER_SRCDIR)/./core/src/get_intra_predictor.cpp\
	$(ENCODER_SRCDIR)/./core/src/encoder_ext.cpp\
	$(ENCODER_SRCDIR)/./core/src/ref_list_mgr_svc.cpp\
	$(ENCODER_SRCDIR)/./core/src/deblocking.cpp\
	$(ENCODER_SRCDIR)/./core/src/picture_handle.cpp\
	$(ENCODER_SRCDIR)/./core/src/encoder.cpp\
	$(ENCODER_SRCDIR)/./core/src/encoder_data_tables.cpp\
	$(ENCODER_SRCDIR)/./core/src/property.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_set_mb_syn_cavlc.cpp\
	$(ENCODER_SRCDIR)/./core/src/encode_mb_aux.cpp\
	$(ENCODER_SRCDIR)/./core/src/memory_align.cpp\
	$(ENCODER_SRCDIR)/./core/src/set_mb_syn_cavlc.cpp\
	$(ENCODER_SRCDIR)/./core/src/utils.cpp\
	$(ENCODER_SRCDIR)/./core/src/mv_pred.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_encode_slice.cpp\
	$(ENCODER_SRCDIR)/./core/src/au_set.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_base_layer_md.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_encode_mb.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_motion_estimate.cpp\
	$(ENCODER_SRCDIR)/./core/src/md.cpp\
	$(ENCODER_SRCDIR)/./core/src/sample.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_enc_slice_segment.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_mode_decision.cpp\
	$(ENCODER_SRCDIR)/./core/src/nal_encap.cpp\
	$(ENCODER_SRCDIR)/./core/src/expand_pic.cpp\
	$(ENCODER_SRCDIR)/./core/src/decode_mb_aux.cpp\
	$(ENCODER_SRCDIR)/./plus/src/welsEncoderExt.cpp\
	$(ENCODER_SRCDIR)/./plus/src/welsCodecTrace.cpp\

ENCODER_OBJS += $(ENCODER_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
ENCODER_ASM_SRCS=\
	$(ENCODER_SRCDIR)/./core/asm/quant.asm\
	$(ENCODER_SRCDIR)/./core/asm/score.asm\
	$(ENCODER_SRCDIR)/./core/asm/coeff.asm\
	$(ENCODER_SRCDIR)/./core/asm/dct.asm\
	$(ENCODER_SRCDIR)/./core/asm/satd_sad.asm\
	$(ENCODER_SRCDIR)/./core/asm/memzero.asm\
	$(ENCODER_SRCDIR)/./core/asm/intra_pred.asm\

ENCODER_OBJS += $(ENCODER_ASM_SRCS:.asm=.o)
endif

OBJS += $(ENCODER_OBJS)
$(ENCODER_SRCDIR)/./core/src/mc.o: $(ENCODER_SRCDIR)/./core/src/mc.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/mc.o $(ENCODER_SRCDIR)/./core/src/mc.cpp

$(ENCODER_SRCDIR)/./core/src/slice_multi_threading.o: $(ENCODER_SRCDIR)/./core/src/slice_multi_threading.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/slice_multi_threading.o $(ENCODER_SRCDIR)/./core/src/slice_multi_threading.cpp

$(ENCODER_SRCDIR)/./core/src/wels_preprocess.o: $(ENCODER_SRCDIR)/./core/src/wels_preprocess.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/wels_preprocess.o $(ENCODER_SRCDIR)/./core/src/wels_preprocess.cpp

$(ENCODER_SRCDIR)/./core/src/ratectl.o: $(ENCODER_SRCDIR)/./core/src/ratectl.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/ratectl.o $(ENCODER_SRCDIR)/./core/src/ratectl.cpp

$(ENCODER_SRCDIR)/./core/src/get_intra_predictor.o: $(ENCODER_SRCDIR)/./core/src/get_intra_predictor.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/get_intra_predictor.o $(ENCODER_SRCDIR)/./core/src/get_intra_predictor.cpp

$(ENCODER_SRCDIR)/./core/src/encoder_ext.o: $(ENCODER_SRCDIR)/./core/src/encoder_ext.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/encoder_ext.o $(ENCODER_SRCDIR)/./core/src/encoder_ext.cpp

$(ENCODER_SRCDIR)/./core/src/ref_list_mgr_svc.o: $(ENCODER_SRCDIR)/./core/src/ref_list_mgr_svc.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/ref_list_mgr_svc.o $(ENCODER_SRCDIR)/./core/src/ref_list_mgr_svc.cpp

$(ENCODER_SRCDIR)/./core/src/deblocking.o: $(ENCODER_SRCDIR)/./core/src/deblocking.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/deblocking.o $(ENCODER_SRCDIR)/./core/src/deblocking.cpp

$(ENCODER_SRCDIR)/./core/src/picture_handle.o: $(ENCODER_SRCDIR)/./core/src/picture_handle.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/picture_handle.o $(ENCODER_SRCDIR)/./core/src/picture_handle.cpp

$(ENCODER_SRCDIR)/./core/src/encoder.o: $(ENCODER_SRCDIR)/./core/src/encoder.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/encoder.o $(ENCODER_SRCDIR)/./core/src/encoder.cpp

$(ENCODER_SRCDIR)/./core/src/encoder_data_tables.o: $(ENCODER_SRCDIR)/./core/src/encoder_data_tables.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/encoder_data_tables.o $(ENCODER_SRCDIR)/./core/src/encoder_data_tables.cpp

$(ENCODER_SRCDIR)/./core/src/property.o: $(ENCODER_SRCDIR)/./core/src/property.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/property.o $(ENCODER_SRCDIR)/./core/src/property.cpp

$(ENCODER_SRCDIR)/./core/src/svc_set_mb_syn_cavlc.o: $(ENCODER_SRCDIR)/./core/src/svc_set_mb_syn_cavlc.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_set_mb_syn_cavlc.o $(ENCODER_SRCDIR)/./core/src/svc_set_mb_syn_cavlc.cpp

$(ENCODER_SRCDIR)/./core/src/encode_mb_aux.o: $(ENCODER_SRCDIR)/./core/src/encode_mb_aux.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/encode_mb_aux.o $(ENCODER_SRCDIR)/./core/src/encode_mb_aux.cpp

$(ENCODER_SRCDIR)/./core/src/memory_align.o: $(ENCODER_SRCDIR)/./core/src/memory_align.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/memory_align.o $(ENCODER_SRCDIR)/./core/src/memory_align.cpp

$(ENCODER_SRCDIR)/./core/src/set_mb_syn_cavlc.o: $(ENCODER_SRCDIR)/./core/src/set_mb_syn_cavlc.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/set_mb_syn_cavlc.o $(ENCODER_SRCDIR)/./core/src/set_mb_syn_cavlc.cpp

$(ENCODER_SRCDIR)/./core/src/utils.o: $(ENCODER_SRCDIR)/./core/src/utils.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/utils.o $(ENCODER_SRCDIR)/./core/src/utils.cpp

$(ENCODER_SRCDIR)/./core/src/mv_pred.o: $(ENCODER_SRCDIR)/./core/src/mv_pred.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/mv_pred.o $(ENCODER_SRCDIR)/./core/src/mv_pred.cpp

$(ENCODER_SRCDIR)/./core/src/svc_encode_slice.o: $(ENCODER_SRCDIR)/./core/src/svc_encode_slice.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_encode_slice.o $(ENCODER_SRCDIR)/./core/src/svc_encode_slice.cpp

$(ENCODER_SRCDIR)/./core/src/au_set.o: $(ENCODER_SRCDIR)/./core/src/au_set.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/au_set.o $(ENCODER_SRCDIR)/./core/src/au_set.cpp

$(ENCODER_SRCDIR)/./core/src/svc_base_layer_md.o: $(ENCODER_SRCDIR)/./core/src/svc_base_layer_md.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_base_layer_md.o $(ENCODER_SRCDIR)/./core/src/svc_base_layer_md.cpp

$(ENCODER_SRCDIR)/./core/src/svc_encode_mb.o: $(ENCODER_SRCDIR)/./core/src/svc_encode_mb.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_encode_mb.o $(ENCODER_SRCDIR)/./core/src/svc_encode_mb.cpp

$(ENCODER_SRCDIR)/./core/src/svc_motion_estimate.o: $(ENCODER_SRCDIR)/./core/src/svc_motion_estimate.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_motion_estimate.o $(ENCODER_SRCDIR)/./core/src/svc_motion_estimate.cpp

$(ENCODER_SRCDIR)/./core/src/md.o: $(ENCODER_SRCDIR)/./core/src/md.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/md.o $(ENCODER_SRCDIR)/./core/src/md.cpp

$(ENCODER_SRCDIR)/./core/src/sample.o: $(ENCODER_SRCDIR)/./core/src/sample.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/sample.o $(ENCODER_SRCDIR)/./core/src/sample.cpp

$(ENCODER_SRCDIR)/./core/src/svc_enc_slice_segment.o: $(ENCODER_SRCDIR)/./core/src/svc_enc_slice_segment.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_enc_slice_segment.o $(ENCODER_SRCDIR)/./core/src/svc_enc_slice_segment.cpp

$(ENCODER_SRCDIR)/./core/src/svc_mode_decision.o: $(ENCODER_SRCDIR)/./core/src/svc_mode_decision.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/svc_mode_decision.o $(ENCODER_SRCDIR)/./core/src/svc_mode_decision.cpp

$(ENCODER_SRCDIR)/./core/src/nal_encap.o: $(ENCODER_SRCDIR)/./core/src/nal_encap.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/nal_encap.o $(ENCODER_SRCDIR)/./core/src/nal_encap.cpp

$(ENCODER_SRCDIR)/./core/src/expand_pic.o: $(ENCODER_SRCDIR)/./core/src/expand_pic.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/expand_pic.o $(ENCODER_SRCDIR)/./core/src/expand_pic.cpp

$(ENCODER_SRCDIR)/./core/src/decode_mb_aux.o: $(ENCODER_SRCDIR)/./core/src/decode_mb_aux.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./core/src/decode_mb_aux.o $(ENCODER_SRCDIR)/./core/src/decode_mb_aux.cpp

$(ENCODER_SRCDIR)/./plus/src/welsEncoderExt.o: $(ENCODER_SRCDIR)/./plus/src/welsEncoderExt.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./plus/src/welsEncoderExt.o $(ENCODER_SRCDIR)/./plus/src/welsEncoderExt.cpp

$(ENCODER_SRCDIR)/./plus/src/welsCodecTrace.o: $(ENCODER_SRCDIR)/./plus/src/welsCodecTrace.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $(ENCODER_SRCDIR)/./plus/src/welsCodecTrace.o $(ENCODER_SRCDIR)/./plus/src/welsCodecTrace.cpp

$(ENCODER_SRCDIR)/./core/asm/quant.o: $(ENCODER_SRCDIR)/./core/asm/quant.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/quant.o $(ENCODER_SRCDIR)/./core/asm/quant.asm

$(ENCODER_SRCDIR)/./core/asm/score.o: $(ENCODER_SRCDIR)/./core/asm/score.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/score.o $(ENCODER_SRCDIR)/./core/asm/score.asm

$(ENCODER_SRCDIR)/./core/asm/coeff.o: $(ENCODER_SRCDIR)/./core/asm/coeff.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/coeff.o $(ENCODER_SRCDIR)/./core/asm/coeff.asm

$(ENCODER_SRCDIR)/./core/asm/dct.o: $(ENCODER_SRCDIR)/./core/asm/dct.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/dct.o $(ENCODER_SRCDIR)/./core/asm/dct.asm

$(ENCODER_SRCDIR)/./core/asm/satd_sad.o: $(ENCODER_SRCDIR)/./core/asm/satd_sad.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/satd_sad.o $(ENCODER_SRCDIR)/./core/asm/satd_sad.asm

$(ENCODER_SRCDIR)/./core/asm/memzero.o: $(ENCODER_SRCDIR)/./core/asm/memzero.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/memzero.o $(ENCODER_SRCDIR)/./core/asm/memzero.asm

$(ENCODER_SRCDIR)/./core/asm/intra_pred.o: $(ENCODER_SRCDIR)/./core/asm/intra_pred.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $(ENCODER_SRCDIR)/./core/asm/intra_pred.o $(ENCODER_SRCDIR)/./core/asm/intra_pred.asm

$(LIBPREFIX)encoder.$(LIBSUFFIX): $(ENCODER_OBJS)
	rm -f $(LIBPREFIX)encoder.$(LIBSUFFIX)
	$(AR) cr $@ $(ENCODER_OBJS)

libraries: $(LIBPREFIX)encoder.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)encoder.$(LIBSUFFIX)
