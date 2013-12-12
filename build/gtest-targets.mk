GTEST_PREFIX=GTEST
GTEST_SRCDIR=gtest
GTEST_CPP_SRCS=\
	$(GTEST_SRCDIR)/src/gtest-all.cc

GTEST_OBJS += $(GTEST_CPP_SRCS:.cc=.o)

OBJS += $(GTEST_OBJS)
GTEST_INCLUDES += -Igtest

$(GTEST_SRCDIR)/src/gtest-all.o: $(GTEST_SRCDIR)/src/gtest-all.cc
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(GTEST_CFLAGS) $(GTEST_INCLUDES) -c -o $(GTEST_SRCDIR)/src/gtest-all.o $(GTEST_SRCDIR)/src/gtest-all.cc

$(LIBPREFIX)gtest.$(LIBSUFFIX): $(GTEST_OBJS)
	rm -f $(LIBPREFIX)gtest.$(LIBSUFFIX)
	ar cr $@ $(GTEST_OBJS)

libraries: $(LIBPREFIX)gtest.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)gtest.$(LIBSUFFIX)
