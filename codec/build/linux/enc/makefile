NAME= welsenc

### include debug information: 1=yes, 0=no
DBG= 0
NASM = 1
DEPEND= dependencies

OUTDIR=		../../../../bin/linux
BINDIR= 	../bin
INCLUDE=  -I../../../encoder/core/inc -I../../../encoder/plus/inc -I../../../api/svc -I../../../WelsThreadLib/api -I../../../console/enc/inc -I../../../common
THREADLIBSRCDIR=../../../WelsThreadLib/src
CORESRCDIR=	../../../encoder/core/src
PLUSSRCDIR=	../../../encoder/plus/src
ASMSRCDIR=	../../../encoder/core/asm
MAINSRCDIR=	../../../console/enc/src
COMMONSRCDIR=   ../../../common

OBJMAINDIR= ../obj
OBJDIR= ../obj/enc

CC= $(shell which gcc)
AS= $(shell which nasm)
CXX = g++ -m32
GCC = gcc -m32

ASFLAGS= -f elf -DNOPREFIX -I ../../../encoder/core/asm/

LIBS= -lstdc++ -ldl -lpthread -lm
#-lm
CFLAGS=  $(INCLUDE) -m32 -fPIC -D__GCC__ -DLINUX -D__NO_CTYPE -DWELS_TESTBED -DMT_ENABLED

ifeq ($(DBG),1)
#SUFFIX= .dbg
CFLAGS+= -g
else
#SUFFIX=
CFLAGS+= -O3
endif
ifeq ($(NASM), 1)
CFLAGS += -DX86_ASM
endif
OBJSUF= .o$(SUFFIX)

ENCODESRC= $(CORESRCDIR)/wels_preprocess.cpp \
$(CORESRCDIR)/au_set.cpp \
$(CORESRCDIR)/cpu.cpp \
$(CORESRCDIR)/deblocking.cpp \
$(CORESRCDIR)/decode_mb_aux.cpp \
$(CORESRCDIR)/encode_mb_aux.cpp \
$(CORESRCDIR)/encoder.cpp \
$(CORESRCDIR)/encoder_data_tables.cpp \
$(CORESRCDIR)/encoder_ext.cpp \
$(CORESRCDIR)/expand_pic.cpp \
$(CORESRCDIR)/get_intra_predictor.cpp \
$(CORESRCDIR)/mc.cpp \
$(CORESRCDIR)/md.cpp \
$(CORESRCDIR)/memory_align.cpp \
$(CORESRCDIR)/mv_pred.cpp \
$(CORESRCDIR)/nal_encap.cpp \
$(CORESRCDIR)/picture_handle.cpp \
$(CORESRCDIR)/property.cpp \
$(CORESRCDIR)/ratectl.cpp \
$(CORESRCDIR)/ref_list_mgr_svc.cpp \
$(CORESRCDIR)/sample.cpp \
$(CORESRCDIR)/set_mb_syn_cavlc.cpp \
$(CORESRCDIR)/slice_multi_threading.cpp \
$(CORESRCDIR)/svc_enc_slice_segment.cpp \
$(CORESRCDIR)/svc_base_layer_md.cpp \
$(CORESRCDIR)/svc_encode_mb.cpp \
$(CORESRCDIR)/svc_encode_slice.cpp \
$(CORESRCDIR)/svc_mode_decision.cpp \
$(CORESRCDIR)/svc_motion_estimate.cpp \
$(CORESRCDIR)/svc_set_mb_syn_cavlc.cpp \
$(CORESRCDIR)/utils.cpp \
$(THREADLIBSRCDIR)/WelsThreadLib.cpp \
$(PLUSSRCDIR)/welsEncoderExt.cpp \
$(PLUSSRCDIR)/welsCodecTrace.cpp \
$(COMMONSRCDIR)/logging.cpp

ASMSRC=	$(ASMSRCDIR)/coeff.asm \
$(ASMSRCDIR)/cpuid.asm \
$(ASMSRCDIR)/dct.asm \
$(ASMSRCDIR)/deblock.asm \
$(ASMSRCDIR)/expand_picture.asm \
$(ASMSRCDIR)/intra_pred.asm \
$(ASMSRCDIR)/intra_pred_util.asm \
$(ASMSRCDIR)/mb_copy.asm \
$(ASMSRCDIR)/mc_chroma.asm \
$(ASMSRCDIR)/mc_luma.asm \
$(ASMSRCDIR)/memzero.asm \
$(ASMSRCDIR)/quant.asm \
$(ASMSRCDIR)/satd_sad.asm \
$(ASMSRCDIR)/score.asm \
$(ASMSRCDIR)/asm_inc.asm \
$(ASMSRCDIR)/vaa.asm


MAINSRC= $(MAINSRCDIR)/read_config.cpp \
$(MAINSRCDIR)/welsenc.cpp

OBJENC=	$(OBJDIR)/wels_preprocess.o \
$(OBJDIR)/au_set.o \
$(OBJDIR)/cpu.o \
$(OBJDIR)/deblocking.o \
$(OBJDIR)/decode_mb_aux.o \
$(OBJDIR)/encode_mb_aux.o \
$(OBJDIR)/encoder.o \
$(OBJDIR)/encoder_data_tables.o \
$(OBJDIR)/encoder_ext.o \
$(OBJDIR)/expand_pic.o \
$(OBJDIR)/get_intra_predictor.o \
$(OBJDIR)/mc.o \
$(OBJDIR)/md.o \
$(OBJDIR)/memory_align.o \
$(OBJDIR)/mv_pred.o \
$(OBJDIR)/nal_encap.o \
$(OBJDIR)/picture_handle.o \
$(OBJDIR)/property.o \
$(OBJDIR)/ratectl.o \
$(OBJDIR)/ref_list_mgr_svc.o \
$(OBJDIR)/sample.o \
$(OBJDIR)/set_mb_syn_cavlc.o \
$(OBJDIR)/slice_multi_threading.o \
$(OBJDIR)/svc_enc_slice_segment.o \
$(OBJDIR)/svc_base_layer_md.o \
$(OBJDIR)/svc_encode_mb.o \
$(OBJDIR)/svc_encode_slice.o \
$(OBJDIR)/svc_mode_decision.o \
$(OBJDIR)/svc_motion_estimate.o \
$(OBJDIR)/svc_set_mb_syn_cavlc.o \
$(OBJDIR)/utils.o \
$(OBJDIR)/WelsThreadLib.o \
$(OBJDIR)/welsEncoderExt.o \
$(OBJDIR)/welsCodecTrace.o \
$(OBJDIR)/logging.o

ifeq ($(NASM), 1)
OBJENC += $(OBJDIR)/cpuid.o \
$(OBJDIR)/coeff.o \
$(OBJDIR)/dct.o \
$(OBJDIR)/deblock.o \
$(OBJDIR)/expand_picture.o \
$(OBJDIR)/intra_pred_util.o \
$(OBJDIR)/intra_pred.o \
$(OBJDIR)/mb_copy.o \
$(OBJDIR)/mc_chroma.o \
$(OBJDIR)/mc_luma.o \
$(OBJDIR)/memzero.o \
$(OBJDIR)/quant.o \
$(OBJDIR)/satd_sad.o \
$(OBJDIR)/score.o \
$(OBJDIR)/asm_inc.o \
$(OBJDIR)/vaa.o
endif
OBJBIN=	$(OBJDIR)/read_config.o \
$(OBJDIR)/welsenc.o

BINLIB=    	$(BINDIR)/$(NAME).a
SHAREDLIB=  	$(BINDIR)/$(NAME).so
BIN=    	$(BINDIR)/$(NAME).exe

default: depend checkdir lib dylib exe release

dependencies:
	@echo "" >dependencies

checkdir:
	@echo 'checkdir..'
	@if test ! -d $(OUTDIR) ; \
	then \
		mkdir -p $(OUTDIR) ; \
	fi
	@if test ! -d $(BINDIR) ; \
	then \
		mkdir -p $(BINDIR) ; \
	fi
	@if test ! -d $(OBJMAINDIR) ; \
	then \
		mkdir -p $(OBJMAINDIR) ; \
	fi
	@if test ! -d $(OBJDIR) ; \
	then \
		mkdir -p $(OBJDIR) ; \
	fi
	@echo

clean:
	@echo remove all objects
	@rm -f $(OBJENC)
	@rm -f $(OBJBIN)
	@rm -f $(BINLIB)
	@rm -f $(SHAREDLIB)
	@rm -f $(BIN)

tags:
	@echo update tag table
	@etags $(THREADLIBSRCDIR)/*.cpp $(COMMSRCDIR)/*.cpp $(CORESRCDIR)/*.cpp $(PLUSSRCDIR)/*.cpp $(MAINSRCDIR)/*.cpp


lib:   	$(OBJENC)
	@echo '$(OBJENC)'
	@echo
	@echo 'ar cr $(BINLIB) $(OBJENC)'
	@echo
	@echo 'creating libraries "$(BINLIB)"'
	@ar cr $(BINLIB) $(OBJENC)
	@echo '... done'
	#@echo 'cp $(BINLIB) /usr/lib'
	#@cp $(BINLIB) /usr/lib
	@echo


dylib:   $(OBJENC)
	@echo '$(OBJENC)'
	@echo
	@echo '$(GCC) -shared -Wl,-Bsymbolic -m32 -o $(SHAREDLIB) $(OBJENC) $(LIBS)'
	@echo 'creating dynamic library "$(SHAREDLIB)"'
	@$(GCC)  -shared -Wl,-Bsymbolic -m32 -o $(SHAREDLIB) $(OBJENC)  $(LIBS)
	@echo '... done'
	@echo

release:
	@echo 'release..'
	@echo 'cp -f $(SHAREDLIB) $(OUTDIR)'
	@cp -f $(SHAREDLIB) $(OUTDIR)
	@echo 'cp -f $(BIN) $(OUTDIR)'
	@cp -f $(BIN) $(OUTDIR)
	@echo

exe:	$(OBJBIN)
	@echo
	@echo '$(OBJBIN)'
	@echo
	@echo '$(GCC) $(LIBS) $(OBJBIN) $(BINLIB) -m32 -o $(BIN)'
	@echo 'creating binary "$(BIN)"'
	@$(GCC)  $(OBJBIN) $(BINLIB) -m32 -o $(BIN) $(LIBS)
	@echo '... done'
	@echo

depend:
	@echo
	@echo 'checking dependencies'
	@$(SHELL) -ec '$(CC) -m32 -MM $(CFLAGS) $(ENCODESRC) $(ASMSRC) $(MAINSRC)\
         | sed '\''s@\(.*\)\.o[ :]@$(OBJDIR)/\1.o$(SUFFIX):@g'\''               \
         >$(DEPEND)'
	@echo

$(OBJDIR)/%.o$(SUFFIX): $(THREADLIBSRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(CORESRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(PLUSSRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(ASMSRCDIR)/%.asm
	@echo 'compiling object file "$@" ...'
	@$(AS) $(ASFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(MAINSRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(MAINSRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(COMMONSRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

include $(DEPEND)

