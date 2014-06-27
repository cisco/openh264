ENCODER_SRCDIR=codec/encoder
ENCODER_CPP_SRCS=\
	$(ENCODER_SRCDIR)/core/src/au_set.cpp\
	$(ENCODER_SRCDIR)/core/src/deblocking.cpp\
	$(ENCODER_SRCDIR)/core/src/decode_mb_aux.cpp\
	$(ENCODER_SRCDIR)/core/src/encode_mb_aux.cpp\
	$(ENCODER_SRCDIR)/core/src/encoder.cpp\
	$(ENCODER_SRCDIR)/core/src/encoder_data_tables.cpp\
	$(ENCODER_SRCDIR)/core/src/encoder_ext.cpp\
	$(ENCODER_SRCDIR)/core/src/get_intra_predictor.cpp\
	$(ENCODER_SRCDIR)/core/src/mc.cpp\
	$(ENCODER_SRCDIR)/core/src/md.cpp\
	$(ENCODER_SRCDIR)/core/src/memory_align.cpp\
	$(ENCODER_SRCDIR)/core/src/mv_pred.cpp\
	$(ENCODER_SRCDIR)/core/src/nal_encap.cpp\
	$(ENCODER_SRCDIR)/core/src/picture_handle.cpp\
	$(ENCODER_SRCDIR)/core/src/property.cpp\
	$(ENCODER_SRCDIR)/core/src/ratectl.cpp\
	$(ENCODER_SRCDIR)/core/src/ref_list_mgr_svc.cpp\
	$(ENCODER_SRCDIR)/core/src/sample.cpp\
	$(ENCODER_SRCDIR)/core/src/set_mb_syn_cavlc.cpp\
	$(ENCODER_SRCDIR)/core/src/slice_multi_threading.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_base_layer_md.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_enc_slice_segment.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_encode_mb.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_encode_slice.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_mode_decision.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_motion_estimate.cpp\
	$(ENCODER_SRCDIR)/core/src/svc_set_mb_syn_cavlc.cpp\
	$(ENCODER_SRCDIR)/core/src/wels_preprocess.cpp\
	$(ENCODER_SRCDIR)/plus/src/welsEncoderExt.cpp\

ENCODER_OBJS += $(ENCODER_CPP_SRCS:.cpp=.$(OBJ))

ifeq ($(ASM_ARCH), x86)
ENCODER_ASM_SRCS=\
	$(ENCODER_SRCDIR)/core/x86/coeff.asm\
	$(ENCODER_SRCDIR)/core/x86/dct.asm\
	$(ENCODER_SRCDIR)/core/x86/intra_pred.asm\
	$(ENCODER_SRCDIR)/core/x86/matrix_transpose.asm\
	$(ENCODER_SRCDIR)/core/x86/memzero.asm\
	$(ENCODER_SRCDIR)/core/x86/quant.asm\
	$(ENCODER_SRCDIR)/core/x86/sample_sc.asm\
	$(ENCODER_SRCDIR)/core/x86/score.asm\

ENCODER_OBJS += $(ENCODER_ASM_SRCS:.asm=.$(OBJ))
endif

ifeq ($(ASM_ARCH), arm)
ENCODER_ASM_ARM_SRCS=\
	$(ENCODER_SRCDIR)/core/arm/intra_pred_neon.S\
	$(ENCODER_SRCDIR)/core/arm/intra_pred_sad_3_opt_neon.S\
	$(ENCODER_SRCDIR)/core/arm/memory_neon.S\
	$(ENCODER_SRCDIR)/core/arm/pixel_neon.S\
	$(ENCODER_SRCDIR)/core/arm/reconstruct_neon.S\

ENCODER_OBJS += $(ENCODER_ASM_ARM_SRCS:.S=.$(OBJ))
endif

ifeq ($(ASM_ARCH), arm64)
ENCODER_ASM_ARM64_SRCS=\
	$(ENCODER_SRCDIR)/core/arm64/intra_pred_aarch64_neon.S\
	$(ENCODER_SRCDIR)/core/arm64/intra_pred_sad_3_opt_aarch64_neon.S\
	$(ENCODER_SRCDIR)/core/arm64/pixel_aarch64_neon.S\

ENCODER_OBJS += $(ENCODER_ASM_ARM64_SRCS:.S=.$(OBJ))
endif

OBJS += $(ENCODER_OBJS)
$(ENCODER_SRCDIR)/%.$(OBJ): $(ENCODER_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c $(CXX_O) $<

$(ENCODER_SRCDIR)/%.$(OBJ): $(ENCODER_SRCDIR)/%.asm
	$(QUIET_ASM)$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(ENCODER_ASMFLAGS) $(ENCODER_ASM_INCLUDES) -o $@ $<

$(ENCODER_SRCDIR)/%.$(OBJ): $(ENCODER_SRCDIR)/%.S
	$(QUIET_CCAS)$(CCAS) $(CCASFLAGS) $(ASMFLAGS) $(INCLUDES) $(ENCODER_CFLAGS) $(ENCODER_INCLUDES) -c -o $@ $<

$(LIBPREFIX)encoder.$(LIBSUFFIX): $(ENCODER_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

libraries: $(LIBPREFIX)encoder.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)encoder.$(LIBSUFFIX)
