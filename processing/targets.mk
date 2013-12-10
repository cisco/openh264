PROCESSING_PREFIX=PROCESSING
PROCESSING_SRCDIR=processing
PROCESSING_CPP_SRCS=\
	$(PROCESSING_SRCDIR)/./src/adaptivequantization/AdaptiveQuantization.cpp\
	$(PROCESSING_SRCDIR)/./src/backgounddetection/BackgroundDetection.cpp\
	$(PROCESSING_SRCDIR)/./src/common/cpu.cpp\
	$(PROCESSING_SRCDIR)/./src/common/memory.cpp\
	$(PROCESSING_SRCDIR)/./src/common/thread.cpp\
	$(PROCESSING_SRCDIR)/./src/common/util.cpp\
	$(PROCESSING_SRCDIR)/./src/common/WelsFrameWork.cpp\
	$(PROCESSING_SRCDIR)/./src/common/WelsFrameWorkEx.cpp\
	$(PROCESSING_SRCDIR)/./src/complexityanalysis/ComplexityAnalysis.cpp\
	$(PROCESSING_SRCDIR)/./src/denoise/denoise.cpp\
	$(PROCESSING_SRCDIR)/./src/denoise/denoise_filter.cpp\
	$(PROCESSING_SRCDIR)/./src/downsample/downsample.cpp\
	$(PROCESSING_SRCDIR)/./src/downsample/downsamplefuncs.cpp\
	$(PROCESSING_SRCDIR)/./src/imagerotate/imagerotate.cpp\
	$(PROCESSING_SRCDIR)/./src/imagerotate/imagerotatefuncs.cpp\
	$(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetection.cpp\
	$(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetectionCommon.cpp\
	$(PROCESSING_SRCDIR)/./src/testbed/stdafx.cpp\
	$(PROCESSING_SRCDIR)/./src/vaacalc/vaacalcfuncs.cpp\
	$(PROCESSING_SRCDIR)/./src/vaacalc/vaacalculation.cpp\

PROCESSING_OBJS += $(PROCESSING_CPP_SRCS:.cpp=.o)
ifdef USE_ASM
PROCESSING_ASM_SRCS=\
	$(PROCESSING_SRCDIR)/./src/asm/asm_inc.asm\
	$(PROCESSING_SRCDIR)/./src/asm/cpuid.asm\
	$(PROCESSING_SRCDIR)/./src/asm/denoisefilter.asm\
	$(PROCESSING_SRCDIR)/./src/asm/downsample_bilinear.asm\
	$(PROCESSING_SRCDIR)/./src/asm/intra_pred.asm\
	$(PROCESSING_SRCDIR)/./src/asm/sad.asm\
	$(PROCESSING_SRCDIR)/./src/asm/vaa.asm\

PROCESSING_OBJS += $(PROCESSING_ASM_SRCS:.asm=.o)
endif

OBJS += $(PROCESSING_OBJS)
$(PROCESSING_SRCDIR)/./src/adaptivequantization/AdaptiveQuantization.o: $(PROCESSING_SRCDIR)/./src/adaptivequantization/AdaptiveQuantization.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/adaptivequantization/AdaptiveQuantization.o $(PROCESSING_SRCDIR)/./src/adaptivequantization/AdaptiveQuantization.cpp

$(PROCESSING_SRCDIR)/./src/backgounddetection/BackgroundDetection.o: $(PROCESSING_SRCDIR)/./src/backgounddetection/BackgroundDetection.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/backgounddetection/BackgroundDetection.o $(PROCESSING_SRCDIR)/./src/backgounddetection/BackgroundDetection.cpp

$(PROCESSING_SRCDIR)/./src/common/cpu.o: $(PROCESSING_SRCDIR)/./src/common/cpu.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/common/cpu.o $(PROCESSING_SRCDIR)/./src/common/cpu.cpp

$(PROCESSING_SRCDIR)/./src/common/memory.o: $(PROCESSING_SRCDIR)/./src/common/memory.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/common/memory.o $(PROCESSING_SRCDIR)/./src/common/memory.cpp

$(PROCESSING_SRCDIR)/./src/common/thread.o: $(PROCESSING_SRCDIR)/./src/common/thread.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/common/thread.o $(PROCESSING_SRCDIR)/./src/common/thread.cpp

$(PROCESSING_SRCDIR)/./src/common/util.o: $(PROCESSING_SRCDIR)/./src/common/util.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/common/util.o $(PROCESSING_SRCDIR)/./src/common/util.cpp

$(PROCESSING_SRCDIR)/./src/common/WelsFrameWork.o: $(PROCESSING_SRCDIR)/./src/common/WelsFrameWork.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/common/WelsFrameWork.o $(PROCESSING_SRCDIR)/./src/common/WelsFrameWork.cpp

$(PROCESSING_SRCDIR)/./src/common/WelsFrameWorkEx.o: $(PROCESSING_SRCDIR)/./src/common/WelsFrameWorkEx.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/common/WelsFrameWorkEx.o $(PROCESSING_SRCDIR)/./src/common/WelsFrameWorkEx.cpp

$(PROCESSING_SRCDIR)/./src/complexityanalysis/ComplexityAnalysis.o: $(PROCESSING_SRCDIR)/./src/complexityanalysis/ComplexityAnalysis.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/complexityanalysis/ComplexityAnalysis.o $(PROCESSING_SRCDIR)/./src/complexityanalysis/ComplexityAnalysis.cpp

$(PROCESSING_SRCDIR)/./src/denoise/denoise.o: $(PROCESSING_SRCDIR)/./src/denoise/denoise.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/denoise/denoise.o $(PROCESSING_SRCDIR)/./src/denoise/denoise.cpp

$(PROCESSING_SRCDIR)/./src/denoise/denoise_filter.o: $(PROCESSING_SRCDIR)/./src/denoise/denoise_filter.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/denoise/denoise_filter.o $(PROCESSING_SRCDIR)/./src/denoise/denoise_filter.cpp

$(PROCESSING_SRCDIR)/./src/downsample/downsample.o: $(PROCESSING_SRCDIR)/./src/downsample/downsample.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/downsample/downsample.o $(PROCESSING_SRCDIR)/./src/downsample/downsample.cpp

$(PROCESSING_SRCDIR)/./src/downsample/downsamplefuncs.o: $(PROCESSING_SRCDIR)/./src/downsample/downsamplefuncs.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/downsample/downsamplefuncs.o $(PROCESSING_SRCDIR)/./src/downsample/downsamplefuncs.cpp

$(PROCESSING_SRCDIR)/./src/imagerotate/imagerotate.o: $(PROCESSING_SRCDIR)/./src/imagerotate/imagerotate.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/imagerotate/imagerotate.o $(PROCESSING_SRCDIR)/./src/imagerotate/imagerotate.cpp

$(PROCESSING_SRCDIR)/./src/imagerotate/imagerotatefuncs.o: $(PROCESSING_SRCDIR)/./src/imagerotate/imagerotatefuncs.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/imagerotate/imagerotatefuncs.o $(PROCESSING_SRCDIR)/./src/imagerotate/imagerotatefuncs.cpp

$(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetection.o: $(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetection.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetection.o $(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetection.cpp

$(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetectionCommon.o: $(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetectionCommon.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetectionCommon.o $(PROCESSING_SRCDIR)/./src/scenechangedetection/SceneChangeDetectionCommon.cpp

$(PROCESSING_SRCDIR)/./src/testbed/stdafx.o: $(PROCESSING_SRCDIR)/./src/testbed/stdafx.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/testbed/stdafx.o $(PROCESSING_SRCDIR)/./src/testbed/stdafx.cpp

$(PROCESSING_SRCDIR)/./src/vaacalc/vaacalcfuncs.o: $(PROCESSING_SRCDIR)/./src/vaacalc/vaacalcfuncs.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/vaacalc/vaacalcfuncs.o $(PROCESSING_SRCDIR)/./src/vaacalc/vaacalcfuncs.cpp

$(PROCESSING_SRCDIR)/./src/vaacalc/vaacalculation.o: $(PROCESSING_SRCDIR)/./src/vaacalc/vaacalculation.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(PROCESSING_CFLAGS) $(PROCESSING_INCLUDES) -c -o $(PROCESSING_SRCDIR)/./src/vaacalc/vaacalculation.o $(PROCESSING_SRCDIR)/./src/vaacalc/vaacalculation.cpp

$(LIBPREFIX)processing.$(LIBSUFFIX): $(PROCESSING_OBJS)
	rm -f $(LIBPREFIX)processing.$(LIBSUFFIX)
	ar cr $@ $(PROCESSING_OBJS)

libraries: $(LIBPREFIX)processing.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)processing.$(LIBSUFFIX)
