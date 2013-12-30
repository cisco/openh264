H264DEC_PREFIX=H264DEC
H264DEC_SRCDIR=codec/console/dec
H264DEC_CPP_SRCS=\
	$(H264DEC_SRCDIR)/./src/d3d9_utils.cpp\
	$(H264DEC_SRCDIR)/./src/h264dec.cpp\
	$(H264DEC_SRCDIR)/./src/read_config.cpp\

H264DEC_OBJS += $(H264DEC_CPP_SRCS:.cpp=.o)
ifeq ($(USE_ASM), Yes)
H264DEC_ASM_SRCS=\

H264DEC_OBJS += $(H264DEC_ASM_SRCS:.asm=.o)
endif

OBJS += $(H264DEC_OBJS)
$(H264DEC_SRCDIR)/./src/d3d9_utils.o: $(H264DEC_SRCDIR)/./src/d3d9_utils.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(H264DEC_CFLAGS) $(H264DEC_INCLUDES) -c -o $(H264DEC_SRCDIR)/./src/d3d9_utils.o $(H264DEC_SRCDIR)/./src/d3d9_utils.cpp

$(H264DEC_SRCDIR)/./src/h264dec.o: $(H264DEC_SRCDIR)/./src/h264dec.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(H264DEC_CFLAGS) $(H264DEC_INCLUDES) -c -o $(H264DEC_SRCDIR)/./src/h264dec.o $(H264DEC_SRCDIR)/./src/h264dec.cpp

$(H264DEC_SRCDIR)/./src/read_config.o: $(H264DEC_SRCDIR)/./src/read_config.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(H264DEC_CFLAGS) $(H264DEC_INCLUDES) -c -o $(H264DEC_SRCDIR)/./src/read_config.o $(H264DEC_SRCDIR)/./src/read_config.cpp

h264dec: $(H264DEC_OBJS) $(LIBS) $(H264DEC_LIBS)
	$(CXX) -o $@  $(H264DEC_OBJS) $(H264DEC_LDFLAGS) $(H264DEC_LIBS) $(LDFLAGS) $(LIBS)

binaries: h264dec
BINARIES += h264dec
