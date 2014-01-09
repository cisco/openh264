ENCODER_PREFIX=ENCODER
ENCODER_SRCDIR=codec/encoder
ENCODER_CPP_SRCS=\
	$(ENCODER_SRCDIR)/./core/src/au_set.cpp\
	$(ENCODER_SRCDIR)/./core/src/deblocking.cpp\
	$(ENCODER_SRCDIR)/./core/src/decode_mb_aux.cpp\
	$(ENCODER_SRCDIR)/./core/src/encode_mb_aux.cpp\
	$(ENCODER_SRCDIR)/./core/src/encoder.cpp\
	$(ENCODER_SRCDIR)/./core/src/encoder_data_tables.cpp\
	$(ENCODER_SRCDIR)/./core/src/encoder_ext.cpp\
	$(ENCODER_SRCDIR)/./core/src/expand_pic.cpp\
	$(ENCODER_SRCDIR)/./core/src/get_intra_predictor.cpp\
	$(ENCODER_SRCDIR)/./core/src/mc.cpp\
	$(ENCODER_SRCDIR)/./core/src/md.cpp\
	$(ENCODER_SRCDIR)/./core/src/memory_align.cpp\
	$(ENCODER_SRCDIR)/./core/src/mv_pred.cpp\
	$(ENCODER_SRCDIR)/./core/src/nal_encap.cpp\
	$(ENCODER_SRCDIR)/./core/src/picture_handle.cpp\
	$(ENCODER_SRCDIR)/./core/src/property.cpp\
	$(ENCODER_SRCDIR)/./core/src/ratectl.cpp\
	$(ENCODER_SRCDIR)/./core/src/ref_list_mgr_svc.cpp\
	$(ENCODER_SRCDIR)/./core/src/sample.cpp\
	$(ENCODER_SRCDIR)/./core/src/set_mb_syn_cavlc.cpp\
	$(ENCODER_SRCDIR)/./core/src/slice_multi_threading.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_base_layer_md.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_enc_slice_segment.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_encode_mb.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_encode_slice.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_mode_decision.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_motion_estimate.cpp\
	$(ENCODER_SRCDIR)/./core/src/svc_set_mb_syn_cavlc.cpp\
	$(ENCODER_SRCDIR)/./core/src/utils.cpp\
	$(ENCODER_SRCDIR)/./core/src/wels_preprocess.cpp\
	$(ENCODER_SRCDIR)/./plus/src/welsCodecTrace.cpp\
	$(ENCODER_SRCDIR)/./plus/src/welsEncoderExt.cpp\

ENCODER_OBJS += $(ENCODER_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
ENCODER_ASM_SRCS=\
	$(ENCODER_SRCDIR)/./core/asm/coeff.asm\
	$(ENCODER_SRCDIR)/./core/asm/dct.asm\
	$(ENCODER_SRCDIR)/./core/asm/intra_pred.asm\
	$(ENCODER_SRCDIR)/./core/asm/memzero.asm\
	$(ENCODER_SRCDIR)/./core/asm/quant.asm\
	$(ENCODER_SRCDIR)/./core/asm/satd_sad.asm\
	$(ENCODER_SRCDIR)/./core/asm/score.asm\

ENCODER_OBJS += $(ENCODER_ASM_SRCS:.asm=.o)
endif

OBJS += $(ENCODER_OBJS)
$(ENCODER_SRCDIR)/%.o: $(ENCODER_SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $@ $<

$(ENCODER_SRCDIR)/%.o: $(ENCODER_SRCDIR)/%.asm
	$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $@ $<

$(LIBPREFIX)encoder.$(LIBSUFFIX): $(ENCODER_OBJS)
	rm -f $(LIBPREFIX)encoder.$(LIBSUFFIX)
	$(AR) cr $@ $(ENCODER_OBJS)

libraries: $(LIBPREFIX)encoder.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)encoder.$(LIBSUFFIX)
