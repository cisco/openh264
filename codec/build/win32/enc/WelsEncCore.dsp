# Microsoft Developer Studio Project File - Name="WelsEncCore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=WelsEncCore - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WelsEncCore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WelsEncCore.mak" CFG="WelsEncCore - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WelsEncCore - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "WelsEncCore - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\..\bin\Release"
# PROP Intermediate_Dir "..\..\..\obj\encoder\core\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zd /O2 /I "..\..\..\encoder\core\inc" /I "..\..\..\api\svc" /I "..\..\..\WelsThreadLib\api" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "WELS_SVC" /D "ENCODER_CORE" /D "HAVE_MMX" /D "HAVE_CACHE_LINE_ALIGN" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\..\bin\Release\welsecore.lib"

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\..\bin\Debug"
# PROP Intermediate_Dir "..\..\..\obj\encoder\core\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\encoder\core\inc" /I "..\..\..\api\svc" /I "..\..\..\WelsThreadLib\api" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "WELS_SVC" /D "ENCODER_CORE" /D "HAVE_MMX" /D "HAVE_CACHE_LINE_ALIGN" /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\..\bin\Debug\welsecore.lib"

!ENDIF 

# Begin Target

# Name "WelsEncCore - Win32 Release"
# Name "WelsEncCore - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\encoder\core\src\au_set.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\colorspace.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\cpu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\deblocking.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\decode_mb_aux.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\downsample_yuv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\encode_mb_aux.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\encoder.c

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# ADD CPP /D "OUPUT_REF_PIC"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\encoder_data_tables.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\encoder_ext.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\expand_pic.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\get_intra_predictor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\mc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\md.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\mgs_layer_encode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\mv_pred.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\nal_encap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\picture_handle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\pixel.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\property.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\ratectl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\ref_list_mgr_svc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\sei.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\set_mb_syn_cavlc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\slice_multi_threading.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_base_layer_md.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_enc_slice_segment.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_encode_mb.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_encode_slice.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_mode_decision.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_motion_estimate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_preprocess.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\svc_set_mb_syn_cavlc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\utils.c
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\src\vaa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\WelsThreadLib\src\WelsThreadLib.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\as264_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\au_set.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\bit_stream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\callback.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\colorspace.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\cpu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\cpu_core.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\deblocking.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\decode_mb_aux.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\downsample_yuv.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\dq_map.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\encode_mb_aux.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\encoder_context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\expand_pic.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\extern.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\get_intra_predictor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\layered_pic_buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\ls_defines.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\macros.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\mb_cache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\mc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\md.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\measure_time.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\mem_align.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\mgs_layer_encode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\mt_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\mv_pred.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\nal_encap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\nal_prefix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\param_svc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\parameter_sets.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\picture.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\picture_handle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\pixel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\property.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\rc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\ref_list_mgr_svc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\sei.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\set_mb_syn_cavlc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\slice.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\slice_multi_threading.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\stat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_base_layer_md.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_enc_frame.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_enc_golomb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_enc_macroblock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_enc_slice_segment.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_encode_mb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_encode_slice.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_mode_decision.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_motion_estimate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_preprocess.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\svc_set_mb_syn_cavlc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\trace.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\typedefs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\utils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\vaa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\vlc_encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\wels_common_basis.h
# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\inc\wels_const.h
# End Source File
# Begin Source File

SOURCE=..\..\..\WelsThreadLib\api\WelsThreadLib.h
# End Source File
# End Group
# Begin Group "asm"

# PROP Default_Filter "*.asm;*.inc"
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\accumulate_rs.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\accumulate_rs.asm
InputName=accumulate_rs

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\accumulate_rs.asm
InputName=accumulate_rs

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\coeff.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\coeff.asm
InputName=coeff

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\coeff.asm
InputName=coeff

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\coeff_level_to_dct.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\coeff_level_to_dct.asm
InputName=coeff_level_to_dct

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\coeff_level_to_dct.asm
InputName=coeff_level_to_dct

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\colorspace_rgb.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\colorspace_rgb.asm
InputName=colorspace_rgb

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\colorspace_rgb.asm
InputName=colorspace_rgb

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\colorspace_rgb_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\colorspace_rgb_sse2.asm
InputName=colorspace_rgb_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\colorspace_rgb_sse2.asm
InputName=colorspace_rgb_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\colorspace_yuv.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\colorspace_yuv.asm
InputName=colorspace_yuv

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\colorspace_yuv.asm
InputName=colorspace_yuv

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\cpu_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\cpu_mmx.asm
InputName=cpu_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\cpu_mmx.asm
InputName=cpu_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\dct_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\dct_mmx.asm
InputName=dct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\dct_mmx.asm
InputName=dct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\dct_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\dct_sse2.asm
InputName=dct_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\dct_sse2.asm
InputName=dct_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\deblock.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\deblock.asm
InputName=deblock

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\deblock.asm
InputName=deblock

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\dequant_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\dequant_sse2.asm
InputName=dequant_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\dequant_sse2.asm
InputName=dequant_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\downsampling.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\downsampling.asm
InputName=downsampling

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\downsampling.asm
InputName=downsampling

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\expand_picture.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\expand_picture.asm
InputName=expand_picture

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\expand_picture.asm
InputName=expand_picture

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\idct_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\idct_mmx.asm
InputName=idct_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\intra_pred.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\intra_pred.asm
InputName=intra_pred

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\intra_pred.asm
InputName=intra_pred

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\intra_pred_util.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\intra_pred_util.asm
InputName=intra_pred_util

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\intra_pred_util.asm
InputName=intra_pred_util

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mb_copy.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mb_copy.asm
InputName=mb_copy

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mb_copy.asm
InputName=mb_copy

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mc_chroma_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mc_chroma_mmx.asm
InputName=mc_chroma_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mc_chroma_mmx.asm
InputName=mc_chroma_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mc_copy_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mc_copy_mmx.asm
InputName=mc_copy_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mc_copy_mmx.asm
InputName=mc_copy_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mc_hc_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mc_hc_mmx.asm
InputName=mc_hc_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mc_hc_mmx.asm
InputName=mc_hc_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mc_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mc_mmx.asm
InputName=mc_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mc_mmx.asm
InputName=mc_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mc_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mc_sse2.asm
InputName=mc_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mc_sse2.asm
InputName=mc_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\mc_sse2_1.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\mc_sse2_1.asm
InputName=mc_sse2_1

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\mc_sse2_1.asm
InputName=mc_sse2_1

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\memzero.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\memzero.asm
InputName=memzero

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\memzero.asm
InputName=memzero

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\pixel_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\pixel_mmx.asm
InputName=pixel_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\pixel_mmx.asm
InputName=pixel_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\pixel_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\pixel_sse2.asm
InputName=pixel_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\pixel_sse2.asm
InputName=pixel_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\predenoise.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\predenoise.asm
InputName=predenoise

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\predenoise.asm
InputName=predenoise

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\quant_mmx.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\quant_mmx.asm
InputName=quant_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\quant_mmx.asm
InputName=quant_mmx

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\quant_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\quant_sse2.asm
InputName=quant_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
InputDir=\dev\tune\codec\Wels\project\encoder\core\asm
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\quant_sse2.asm
InputName=quant_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -I.\..\..\..\common\asm\ -I$(InputDir) -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\score.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\score.asm
InputName=score

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\score.asm
InputName=score

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\sse2inc.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\sse2inc.asm
InputName=sse2inc

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\sse2inc.asm
InputName=sse2inc

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\sub_scal_coeff2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\sub_scal_coeff2.asm
InputName=sub_scal_coeff2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\sub_scal_coeff2.asm
InputName=sub_scal_coeff2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\encoder\core\asm\vaa_sse2.asm

!IF  "$(CFG)" == "WelsEncCore - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Release
InputPath=..\..\..\encoder\core\asm\vaa_sse2.asm
InputName=vaa_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "WelsEncCore - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\obj\encoder\core\Debug
InputPath=..\..\..\encoder\core\asm\vaa_sse2.asm
InputName=vaa_sse2

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 -O3 -DPREFIX -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
