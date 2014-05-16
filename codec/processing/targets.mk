PROCESSING_SRCDIR=codec/processing
PROCESSING_CPP_SRCS=\
	$(PROCESSING_SRCDIR)/src/adaptivequantization/AdaptiveQuantization.cpp\
	$(PROCESSING_SRCDIR)/src/backgrounddetection/BackgroundDetection.cpp\
	$(PROCESSING_SRCDIR)/src/common/common.cpp\
	$(PROCESSING_SRCDIR)/src/common/memory.cpp\
	$(PROCESSING_SRCDIR)/src/common/WelsFrameWork.cpp\
	$(PROCESSING_SRCDIR)/src/common/WelsFrameWorkEx.cpp\
	$(PROCESSING_SRCDIR)/src/complexityanalysis/ComplexityAnalysis.cpp\
	$(PROCESSING_SRCDIR)/src/denoise/denoise.cpp\
	$(PROCESSING_SRCDIR)/src/denoise/denoise_filter.cpp\
	$(PROCESSING_SRCDIR)/src/downsample/downsample.cpp\
	$(PROCESSING_SRCDIR)/src/downsample/downsamplefuncs.cpp\
	$(PROCESSING_SRCDIR)/src/imagerotate/imagerotate.cpp\
	$(PROCESSING_SRCDIR)/src/imagerotate/imagerotatefuncs.cpp\
	$(PROCESSING_SRCDIR)/src/scenechangedetection/SceneChangeDetection.cpp\
	$(PROCESSING_SRCDIR)/src/scrolldetection/ScrollDetection.cpp\
	$(PROCESSING_SRCDIR)/src/scrolldetection/ScrollDetectionFuncs.cpp\
	$(PROCESSING_SRCDIR)/src/vaacalc/vaacalcfuncs.cpp\
	$(PROCESSING_SRCDIR)/src/vaacalc/vaacalculation.cpp\

PROCESSING_OBJS += $(PROCESSING_CPP_SRCS:.cpp=.$(OBJ))

ifeq ($(ASM_ARCH), x86)
PROCESSING_ASM_SRCS=\
	$(PROCESSING_SRCDIR)/src/x86/denoisefilter.asm\
	$(PROCESSING_SRCDIR)/src/x86/downsample_bilinear.asm\
	$(PROCESSING_SRCDIR)/src/x86/vaa.asm\

PROCESSING_OBJS += $(PROCESSING_ASM_SRCS:.asm=.$(OBJ))
endif

ifeq ($(ASM_ARCH), arm)
PROCESSING_ASM_ARM_SRCS=\
	$(PROCESSING_SRCDIR)/src/arm/adaptive_quantization.S\
	$(PROCESSING_SRCDIR)/src/arm/down_sample_neon.S\
	$(PROCESSING_SRCDIR)/src/arm/pixel_sad_neon.S\
	$(PROCESSING_SRCDIR)/src/arm/vaa_calc_neon.S\

PROCESSING_OBJS += $(PROCESSING_ASM_ARM_SRCS:.S=.$(OBJ))
endif

OBJS += $(PROCESSING_OBJS)
$(PROCESSING_SRCDIR)/%.$(OBJ): $(PROCESSING_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c $(CXX_O) $<

$(PROCESSING_SRCDIR)/%.$(OBJ): $(PROCESSING_SRCDIR)/%.asm
	$(QUIET_ASM)$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(PROCESSING_ASMFLAGS) $(PROCESSING_ASM_INCLUDES) -o $@ $<

$(PROCESSING_SRCDIR)/%.$(OBJ): $(PROCESSING_SRCDIR)/%.S
	$(QUIET_CCAS)$(CCAS) $(CCASFLAGS) $(ASMFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $@ $<

$(LIBPREFIX)processing.$(LIBSUFFIX): $(PROCESSING_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

libraries: $(LIBPREFIX)processing.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)processing.$(LIBSUFFIX)
