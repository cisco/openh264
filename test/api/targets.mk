API_TEST_SRCDIR=test/api
API_TEST_CPP_SRCS=\
	$(API_TEST_SRCDIR)/BaseDecoderTest.cpp\
	$(API_TEST_SRCDIR)/BaseEncoderTest.cpp\
	$(API_TEST_SRCDIR)/cpp_interface_test.cpp\
	$(API_TEST_SRCDIR)/DataGenerator.cpp\
	$(API_TEST_SRCDIR)/decode_encode_test.cpp\
	$(API_TEST_SRCDIR)/decoder_test.cpp\
	$(API_TEST_SRCDIR)/encoder_test.cpp\
	$(API_TEST_SRCDIR)/simple_test.cpp\

API_TEST_OBJS += $(API_TEST_CPP_SRCS:.cpp=.$(OBJ))

API_TEST_C_SRCS=\
	$(API_TEST_SRCDIR)/c_interface_test.c\
	$(API_TEST_SRCDIR)/sha1.c\

API_TEST_OBJS += $(API_TEST_C_SRCS:.c=.$(OBJ))

OBJS += $(API_TEST_OBJS)
$(API_TEST_SRCDIR)/%.$(OBJ): $(API_TEST_SRCDIR)/%.cpp
	$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(API_TEST_CFLAGS) $(API_TEST_INCLUDES) -c $(CXX_O) $<

$(API_TEST_SRCDIR)/%.$(OBJ): $(API_TEST_SRCDIR)/%.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) $(API_TEST_CFLAGS) $(API_TEST_INCLUDES) -c $(CXX_O) $<

