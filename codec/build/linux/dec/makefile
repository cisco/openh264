NAME= welsdec

### include debug information: 1=yes, 0=no
DBG= 0
NASM = 1
DEPEND= dependencies

BINDIR= 	../bin
OUTDIR= 	../../../../bin/linux
INCLUDE= 	-I../../../api/svc -I../../../decoder/core/inc -I../../../decoder/plus/inc -I../../../console/dec/inc
CORESRCDIR=	../../../decoder/core/src
PLUSSRCDIR=	../../../decoder/plus/src
ASMSRCDIR=	../../../decoder/core/asm
MAINSRCDIR=	../../../console/dec/src

OBJMAINDIR= ../obj
OBJDIR= ../obj/dec

CC= $(shell which gcc)
AS= $(shell which nasm)
CXX = g++ -m32
GCC = gcc -m32

ASFLAGS= -f elf -DNOPREFIX -I ../../../decoder/core/asm/

LIBS= -lstdc++ -ldl
#-lm 
CFLAGS=  $(INCLUDE) -fPIC -D__GCC__ -DLINUX -D__NO_CTYPE -DHAVE_CACHE_LINE_ALIGN

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

DECODESRC=$(CORESRCDIR)/au_parser.cpp \
$(CORESRCDIR)/bit_stream.cpp \
$(CORESRCDIR)/cpu.cpp \
$(CORESRCDIR)/deblocking.cpp \
$(CORESRCDIR)/decode_mb_aux.cpp \
$(CORESRCDIR)/decoder.cpp \
$(CORESRCDIR)/decoder_data_tables.cpp \
$(CORESRCDIR)/expand_pic.cpp \
$(CORESRCDIR)/fmo.cpp \
$(CORESRCDIR)/get_intra_predictor.cpp \
$(CORESRCDIR)/manage_dec_ref.cpp \
$(CORESRCDIR)/mc.cpp \
$(CORESRCDIR)/mem_align.cpp \
$(CORESRCDIR)/memmgr_nal_unit.cpp \
$(CORESRCDIR)/mv_pred.cpp \
$(CORESRCDIR)/parse_mb_syn_cavlc.cpp \
$(CORESRCDIR)/pic_queue.cpp \
$(CORESRCDIR)/rec_mb.cpp \
$(CORESRCDIR)/decode_slice.cpp \
$(CORESRCDIR)/decoder_core.cpp \
$(CORESRCDIR)/utils.cpp \
$(PLUSSRCDIR)/welsDecoderExt.cpp \
$(PLUSSRCDIR)/welsCodecTrace.cpp

ASMSRC= $(ASMSRCDIR)/block_add.asm \
$(ASMSRCDIR)/cpuid.asm \
$(ASMSRCDIR)/deblock.asm \
$(ASMSRCDIR)/expand_picture.asm \
$(ASMSRCDIR)/dct.asm \
$(ASMSRCDIR)/intra_pred.asm \
$(ASMSRCDIR)/mc_chroma.asm \
$(ASMSRCDIR)/mb_copy.asm \
$(ASMSRCDIR)/mc_luma.asm \
$(ASMSRCDIR)/memzero.asm \
$(ASMSRCDIR)/asm_inc.asm \
 
MAINSRC= $(MAINSRCDIR)/d3d9_utils.cpp \
$(MAINSRCDIR)/h264dec.cpp \
$(MAINSRCDIR)/read_config.cpp

OBJDEC=$(OBJDIR)/au_parser.o \
$(OBJDIR)/bit_stream.o \
$(OBJDIR)/cpu.o \
$(OBJDIR)/deblocking.o \
$(OBJDIR)/decode_mb_aux.o \
$(OBJDIR)/decoder.o \
$(OBJDIR)/decoder_data_tables.o \
$(OBJDIR)/expand_pic.o \
$(OBJDIR)/fmo.o \
$(OBJDIR)/get_intra_predictor.o \
$(OBJDIR)/manage_dec_ref.o \
$(OBJDIR)/mc.o \
$(OBJDIR)/mem_align.o \
$(OBJDIR)/memmgr_nal_unit.o \
$(OBJDIR)/mv_pred.o \
$(OBJDIR)/parse_mb_syn_cavlc.o \
$(OBJDIR)/pic_queue.o \
$(OBJDIR)/rec_mb.o \
$(OBJDIR)/decode_slice.o \
$(OBJDIR)/decoder_core.o \
$(OBJDIR)/utils.o \
$(OBJDIR)/welsDecoderExt.o \
$(OBJDIR)/welsCodecTrace.o

ifeq ($(NASM), 1)
OBJDEC+=$(OBJDIR)/block_add.o \
$(OBJDIR)/cpuid.o \
$(OBJDIR)/deblock.o \
$(OBJDIR)/expand_picture.o \
$(OBJDIR)/dct.o \
$(OBJDIR)/intra_pred.o \
$(OBJDIR)/mc_chroma.o \
$(OBJDIR)/mb_copy.o \
$(OBJDIR)/mc_luma.o \
$(OBJDIR)/memzero.o \
$(OBJDIR)/asm_inc.o 
endif

OBJBIN=	$(OBJDIR)/d3d9_utils.o \
$(OBJDIR)/h264dec.o \
$(OBJDIR)/read_config.o

BINLIB=    	$(BINDIR)/$(NAME).a
SHAREDLIB=  	$(BINDIR)/$(NAME).so
BIN=    	$(BINDIR)/$(NAME).exe

default: depend checkdir lib dylib exe release

dependencies:
	@echo "" >dependencies
	
checkdir:
	@echo 'checkdir..'
	@if test ! -d $(BINDIR) ; \
	then \
		mkdir -p $(BINDIR) ; \
	fi
	@if test ! -d $(OUTDIR) ; \
	then \
		mkdir -p $(OUTDIR) ; \
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
	
release:
	@echo 'release..'
	@echo 'cp -f $(SHAREDLIB) $(OUTDIR)'
	@cp -f $(SHAREDLIB) $(OUTDIR)
	@echo 'cp -f $(BIN) $(OUTDIR)'
	@cp -f $(BIN) $(OUTDIR)
	@echo

clean:
	@echo remove all objects
	@rm -f $(OBJDEC)
	@rm -f $(OBJBIN)
	@rm -f $(BINLIB)
	@rm -f $(SHAREDLIB)
	@rm -f $(BIN)    

tags:
	@echo update tag table
	@etags $(CORESRCDIR)/*.c $(CORESRCDIR)/*.cpp $(PLUSSRCDIR)/*.cpp $(MAINSRCDIR)/*.cpp
	
	
lib:   	$(OBJDEC) 
	@echo '$(OBJDEC)'
	@echo
	@echo 'ar cr $(BINLIB) $(OBJDEC)'
	@echo
	@echo 'creating libraries "$(BINLIB)"'
	@ar cr $(BINLIB) $(OBJDEC)
	@echo '... done'
	#@echo 'cp $(BINLIB) /usr/lib'
	#@cp $(BINLIB) /usr/lib
	@echo


dylib:   $(OBJDEC)
	@echo '$(OBJDEC)'
	@echo
	@echo '$(CXX) -shared -Wl,-Bsymbolic -o $(SHAREDLIB) $(OBJDEC) $(LIBS)'
	@echo 'creating dynamic library "$(SHAREDLIB)"'
	@$(CXX)  -shared -Wl,-Bsymbolic -o $(SHAREDLIB) $(OBJDEC)  $(LIBS)
	@echo '... done'
	@echo
	

exe:	$(OBJBIN)
	@echo	
	@echo '$(OBJBIN)'
	@echo
	@echo '$(CXX) $(LIBS) $(OBJBIN) $(BINLIB) -o $(BIN)'
	@echo 'creating binary "$(BIN)"'
	@$(CXX) $(OBJBIN) $(BINLIB) -o $(BIN) $(LIBS) 
	@echo '... done'
	@echo

depend:
	@echo
	@echo 'checking dependencies'
	@$(SHELL) -ec '$(CC) -m32 -MM $(CFLAGS) $(DECODESRC) $(ASMSRC) $(MAINSRC)\
         | sed '\''s@\(.*\)\.o[ :]@$(OBJDIR)/\1.o$(SUFFIX):@g'\''               \
         >$(DEPEND)'
	@echo

#$(OBJDIR)/%.o$(SUFFIX): $(COMMSRCDIR)/%.c
#	@echo 'compiling object file "$@" ...'
#	@$(CC) -m32 -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o$(SUFFIX): $(CORESRCDIR)/%.c
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

#$(OBJDIR)/%.o$(SUFFIX): $(ASMCOMDIR)/%.asm
#	@echo 'compiling object file "$@" ...'
#	@$(AS) $(ASFLAGS) -o $@ $<
	
$(OBJDIR)/%.o$(SUFFIX): $(MAINSRCDIR)/%.cpp
	@echo 'compiling object file "$@" ...'
	@$(CC) -m32 -c $(CFLAGS) -o $@ $<		
	
include $(DEPEND)

