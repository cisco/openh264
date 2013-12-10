/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#ifdef ONLY_ENC_FRAMES_NUM
#undef ONLY_ENC_FRAMES_NUM
#endif//ONLY_ENC_FRAMES_NUM
#define ONLY_ENC_FRAMES_NUM		INT_MAX // 2, INT_MAX	// type the num you try to encode here, 2, 10, etc



//#define STICK_STREAM_SIZE

#if defined(__GNUC__)
#if !defined(MACOS)
#if !defined(_MATH_H_MATHDEF)
#define _MATH_H_MATHDEF
//#else
//#error "warning: have defined _MATH_H_MATHDEF!!"	// to check
#endif//_MATH_H_MATHDEF
#endif//MACOS
#endif//__GNUC__

#include "measure_time.h"
#include "param_svc.h"
//#include "layered_pic_buffer.h"
#include "read_config.h"

#if defined(MACOS)
#include "bundlewelsenc.h"
#else
#include "typedefs.h"
#endif//MACOS

#ifdef _MSC_VER
#include <io.h>     /* _setmode() */
#include <fcntl.h>  /* _O_BINARY */
#endif//_MSC_VER

#include "codec_def.h"
#include "codec_api.h"
#include "extern.h"
#include "macros.h"
#include "wels_const.h"
#include "logging.h"

#ifdef MT_ENABLED
#include "mt_defs.h"
#include "WelsThreadLib.h"
#endif//MT_ENABLED

#include <iostream>
using namespace std;
using namespace WelsSVCEnc;

/*
 *	Layer Context
 */
typedef struct LayerpEncCtx_s {
	int32_t				iDLayerQp;
	SMulSliceOption	sMso;
} SLayerPEncCtx;



/* Ctrl-C handler */
static int     g_iCtrlC = 0;
static void    SigIntHandler( int a )
{
    g_iCtrlC = 1;
}

int ParseConfig(CReadConfig& cRdCfg, SWelsSvcCodingParam& pSvcParam, SFilesSet& sFileSet)
{
	string strTag[4];
	int32_t iLeftTargetBitrate = 0;
	int32_t	iLeftSpatialBitrate[MAX_DEPENDENCY_LAYER] = { 0 };
	int32_t iRet = 0;
	int8_t iLayerCount = 0;
	string str_("SlicesAssign");
	const int kiSize = str_.size();
	
//	memset(&pSvcParam, 0, sizeof(WelsSVCParamConfig));

	while ( !cRdCfg.EndOfFile() ){
		long iRd = cRdCfg.ReadLine(&strTag[0]);
		if (iRd > 0){
			if ( strTag[0].empty() )
				continue;
			if (strTag[0].compare("OutputFile") == 0){			
				sFileSet.strBsFile	= strTag[1];
				continue;
			}
			else if (strTag[0].compare("MaxFrameRate") == 0){
				pSvcParam.fMaxFrameRate	= (float)atof(strTag[1].c_str());
				continue;
			}
			else if (strTag[0].compare("FramesToBeEncoded") == 0){
				pSvcParam.uiFrameToBeCoded	= atoi(strTag[1].c_str());
				continue;
			}
			else if ( strTag[0].compare("SourceSequenceInRGB24") == 0 ){
				pSvcParam.iInputCsp	= atoi(strTag[1].c_str()) == 0 ? videoFormatI420 : videoFormatRGB;
				continue;
			}
			else if (strTag[0].compare("GOPSize") == 0){
				pSvcParam.uiGopSize	= atoi(strTag[1].c_str());
				continue;
			}
			else if (strTag[0].compare("IntraPeriod") == 0){
				pSvcParam.uiIntraPeriod	= atoi(strTag[1].c_str());
				continue;
			}
			else if (strTag[0].compare("EnableSpsPpsIDAddition") == 0)
			{
				pSvcParam.bEnableSpsPpsIdAddition	= atoi(strTag[1].c_str())?true:false; 
				continue;
			}
			else if (strTag[0].compare("EnableScalableSEI") == 0)
			{
				pSvcParam.bEnableSSEI	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("EnableFrameCropping") == 0)
			{
				pSvcParam.bEnableFrameCroppingFlag = (atoi(strTag[1].c_str()) != 0);	
				continue;
			}
			else if (strTag[0].compare("LoopFilterDisableIDC") == 0){
				pSvcParam.iLoopFilterDisableIdc	= (int8_t)atoi(strTag[1].c_str());
				if (pSvcParam.iLoopFilterDisableIdc > 6 || pSvcParam.iLoopFilterDisableIdc < 0){
					fprintf(stderr, "Invalid parameter in iLoopFilterDisableIdc: %d.\n", pSvcParam.iLoopFilterDisableIdc);
					iRet = 1;
					break;
				}
				continue;
			}
			else if (strTag[0].compare("LoopFilterAlphaC0Offset") == 0){
				pSvcParam.iLoopFilterAlphaC0Offset	= (int8_t)atoi(strTag[1].c_str());
				if ( pSvcParam.iLoopFilterAlphaC0Offset < -6 )
					pSvcParam.iLoopFilterAlphaC0Offset	= -6;
				else if ( pSvcParam.iLoopFilterAlphaC0Offset > 6 )
					pSvcParam.iLoopFilterAlphaC0Offset	= 6;
				continue;
			}
			else if (strTag[0].compare("LoopFilterBetaOffset") == 0){
				pSvcParam.iLoopFilterBetaOffset	= (int8_t)atoi(strTag[1].c_str());
				if ( pSvcParam.iLoopFilterBetaOffset < -6 )
					pSvcParam.iLoopFilterBetaOffset	= -6;
				else if ( pSvcParam.iLoopFilterBetaOffset > 6 )
					pSvcParam.iLoopFilterBetaOffset	= 6;
				continue;
			}
			else if (strTag[0].compare("InterLayerLoopFilterDisableIDC") == 0){
				pSvcParam.iInterLayerLoopFilterDisableIdc = (int8_t)atoi(strTag[1].c_str());
				if (pSvcParam.iInterLayerLoopFilterDisableIdc > 6 || pSvcParam.iInterLayerLoopFilterDisableIdc < 0){
					fprintf(stderr, "Invalid parameter in iInterLayerLoopFilterDisableIdc: %d.\n", pSvcParam.iInterLayerLoopFilterDisableIdc);
					iRet = 1;
					break;
				}
				continue;
			}
			else if (strTag[0].compare("InterLayerLoopFilterAlphaC0Offset") == 0){
				pSvcParam.iInterLayerLoopFilterAlphaC0Offset	= (int8_t)atoi(strTag[1].c_str());
				if ( pSvcParam.iInterLayerLoopFilterAlphaC0Offset < -6 )
					pSvcParam.iInterLayerLoopFilterAlphaC0Offset	= -6;
				else if ( pSvcParam.iInterLayerLoopFilterAlphaC0Offset > 6 )
					pSvcParam.iInterLayerLoopFilterAlphaC0Offset	= 6;
				continue;
			}
			else if (strTag[0].compare("InterLayerLoopFilterBetaOffset") == 0){
				pSvcParam.iInterLayerLoopFilterBetaOffset	= (int8_t)atoi(strTag[1].c_str());
				if ( pSvcParam.iInterLayerLoopFilterBetaOffset < -6 )
					pSvcParam.iInterLayerLoopFilterBetaOffset	= -6;
				else if ( pSvcParam.iInterLayerLoopFilterBetaOffset > 6 )
					pSvcParam.iInterLayerLoopFilterBetaOffset	= 6;
				continue;
			}			
			else if ( strTag[0].compare("MultipleThreadIdc") == 0 )
			{
				// # 0: auto(dynamic imp. internal encoder); 1: multiple threads imp. disabled; > 1: count number of threads;
				pSvcParam.iMultipleThreadIdc	= atoi( strTag[1].c_str() );
				if ( pSvcParam.iMultipleThreadIdc < 0 )
					pSvcParam.iMultipleThreadIdc = 0;
				else if ( pSvcParam.iMultipleThreadIdc > MAX_THREADS_NUM )
					 pSvcParam.iMultipleThreadIdc = MAX_THREADS_NUM;
				continue;
			}
			else if (strTag[0].compare("EnableRC") == 0){
				pSvcParam.bEnableRc	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("RCMode") == 0){
				pSvcParam.iRCMode	= atoi(strTag[1].c_str());
				continue;
			}
			else if (strTag[0].compare("TargetBitrate") == 0){
				pSvcParam.iTargetBitrate	= 1000 * atoi(strTag[1].c_str());
				if ( pSvcParam.bEnableRc && pSvcParam.iTargetBitrate <= 0 ){
					fprintf(stderr, "Invalid target bitrate setting due to RC enabled. Check TargetBitrate field please!\n");
					return 1;
				}
				if ( pSvcParam.bEnableRc ){
					iLeftTargetBitrate	= pSvcParam.iTargetBitrate;
				}
				continue;
			}
			else if (strTag[0].compare("EnableDenoise") == 0){
				pSvcParam.bEnableDenoise	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("EnableSceneChangeDetection") == 0){
				pSvcParam.bEnableSceneChangeDetect	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("EnableBackgroundDetection") == 0)
			{
				pSvcParam.bEnableBackgroundDetection	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("EnableAdaptiveQuantization") == 0){
				pSvcParam.bEnableAdaptiveQuant	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("EnableLongTermReference") == 0){
				pSvcParam.bEnableLongTermReference	= atoi(strTag[1].c_str())?true:false;
				continue;
			}
			else if (strTag[0].compare("LtrMarkPeriod") == 0){
				pSvcParam.uiLtrMarkPeriod	= (uint32_t)atoi(strTag[1].c_str());
				continue;
			}
			else if (strTag[0].compare("NumLayers") == 0){
				pSvcParam.iNumDependencyLayer	= (int8_t)atoi(strTag[1].c_str());
				if (pSvcParam.iNumDependencyLayer > MAX_DEPENDENCY_LAYER || pSvcParam.iNumDependencyLayer <= 0){
					fprintf(stderr, "Invalid parameter in iNumDependencyLayer: %d.\n", pSvcParam.iNumDependencyLayer);
					iRet = 1;
					break;
				}
				continue;
			}
			else if (strTag[0].compare("LayerCfg") == 0){		
				if ( strTag[1].length() > 0 )
					sFileSet.sSpatialLayers[iLayerCount].strLayerCfgFile	= strTag[1];
//				pSvcParam.sDependencyLayers[iLayerCount].uiDependencyId	= iLayerCount;
				++ iLayerCount;
				continue;
			}
			else if (strTag[0].compare("PrefixNALAddingCtrl") == 0){
				int ctrl_flag = atoi(strTag[1].c_str());
				if (ctrl_flag > 1)
					ctrl_flag	= 1;
				else if (ctrl_flag < 0)
					ctrl_flag	= 0;
				pSvcParam.bPrefixNalAddingCtrl	= ctrl_flag?true:false;
				continue;
			}
		}
	}

	const int8_t kiActualLayerNum = WELS_MIN(pSvcParam.iNumDependencyLayer, iLayerCount);
	if (pSvcParam.iNumDependencyLayer > kiActualLayerNum){	// fixed number of dependency layer due to parameter error in settings
		pSvcParam.iNumDependencyLayer	= kiActualLayerNum;
	}
	
	assert( kiActualLayerNum <= MAX_DEPENDENCY_LAYER );

	for (int8_t iLayer = 0; iLayer < kiActualLayerNum; ++ iLayer){
		SLayerPEncCtx sLayerCtx;
		int32_t iLayerArg = -2;
		int32_t iNumQualityBitrateLayerSet = 0;

		SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
		CReadConfig cRdLayerCfg( sFileSet.sSpatialLayers[iLayer].strLayerCfgFile );

		memset(&sLayerCtx, 0, sizeof(SLayerPEncCtx));

		if ( !cRdLayerCfg.ExistFile() ){
			fprintf(stderr, "Unabled to open layer #%d configuration file: %s.\n", iLayer, cRdLayerCfg.GetFileName().c_str());
			continue;
		}
		
		while ( !cRdLayerCfg.EndOfFile() ){
			long iLayerRd = cRdLayerCfg.ReadLine(&strTag[0]);
			bool_t bFound = false;
			if (iLayerRd > 0){
				if ( strTag[0].empty() )
					continue;
				if (strTag[0].compare("SourceWidth") == 0){
					pDLayer->iFrameWidth	= atoi(strTag[1].c_str());
					pDLayer->iActualWidth= pDLayer->iFrameWidth;
					continue;
				}
				else if (strTag[0].compare("SourceHeight") == 0){
					pDLayer->iFrameHeight	= atoi(strTag[1].c_str());
					pDLayer->iActualHeight	= pDLayer->iFrameHeight;
					continue;
				}
				else if (strTag[0].compare("FrameRateIn") == 0){
					pDLayer->fInputFrameRate	= (float)atof(strTag[1].c_str());
					continue;
				}
				else if (strTag[0].compare("FrameRateOut") == 0){
					pDLayer->fOutputFrameRate = (float)atof(strTag[1].c_str());
					continue;
				}
				else if (strTag[0].compare("InputFile") == 0){		
					if ( strTag[1].length() > 0 )
						sFileSet.sSpatialLayers[iLayer].strSeqFile	= strTag[1];
					continue;
				}
				else if (strTag[0].compare("ReconFile") == 0){
					const int kiLen = strTag[1].length();
					if (kiLen >= MAX_FNAME_LEN)
						return 1;
#ifdef ENABLE_FRAME_DUMP
					pDLayer->sRecFileName[kiLen] = '\0';
					strncpy(pDLayer->sRecFileName, strTag[1].c_str(), kiLen);	// confirmed_safe_unsafe_usage
#endif//ENABLE_FRAME_DUMP
					continue;
				}
				else if (strTag[0].compare("ProfileIdc") == 0){
					pDLayer->uiProfileIdc	= atoi(strTag[1].c_str());
					continue;
				}
				else if (strTag[0].compare("FRExt") == 0){
//					pDLayer->frext_mode	= (bool_t)atoi(strTag[1].c_str());
					continue;
				}

				if (strTag[0].compare("SpatialBitrate") == 0){
					pDLayer->iSpatialBitrate	= 1000 * atoi(strTag[1].c_str());
					if ( pSvcParam.bEnableRc && pDLayer->iSpatialBitrate <= 0 ){
						fprintf(stderr, "Invalid spatial bitrate(%d) in dependency layer #%d.\n", pDLayer->iSpatialBitrate, iLayer);
						return 1;
					}
					if ( pSvcParam.bEnableRc &&pDLayer->iSpatialBitrate > iLeftTargetBitrate ){ 
						fprintf(stderr, "Invalid spatial(#%d) bitrate(%d) setting due to unavailable left(%d)!\n", iLayer, pDLayer->iSpatialBitrate, iLeftTargetBitrate);
						return 1;
					}
					iLeftSpatialBitrate[iLayer]	= pDLayer->iSpatialBitrate;
					continue;
				}
				if (strTag[0].compare("InitialQP") == 0){
					sLayerCtx.iDLayerQp	= atoi(strTag[1].c_str());
					continue;
				}
				if (strTag[0].compare("SliceMode") == 0){
					sLayerCtx.sMso.uiSliceMode	= (SliceMode)atoi(strTag[1].c_str());
					continue;
				}
				else if (strTag[0].compare("SliceSize") == 0){//SM_DYN_SLICE
					sLayerCtx.sMso.sSliceArgument.uiSliceSizeConstraint	= (SliceMode)atoi(strTag[1].c_str());
					continue;
				}
				else if (strTag[0].compare("SliceNum") == 0){
					sLayerCtx.sMso.sSliceArgument.iSliceNum = atoi(strTag[1].c_str());
					continue;
				}
				else if ( strTag[0].compare(0, kiSize, str_ ) == 0 )
				{
					const char* kpString = strTag[0].c_str();
					int uiSliceIdx = atoi(&kpString[kiSize]);
					assert( uiSliceIdx < MAX_SLICES_NUM );
					sLayerCtx.sMso.sSliceArgument.uiSliceMbNum[uiSliceIdx] = atoi( strTag[1].c_str() );
					continue;
				}
			}
		}
		pDLayer->iDLayerQp	= sLayerCtx.iDLayerQp;
		pDLayer->sMso.uiSliceMode		= sLayerCtx.sMso.uiSliceMode;		

		memcpy( &pDLayer->sMso, &sLayerCtx.sMso, sizeof(SMulSliceOption) );	// confirmed_safe_unsafe_usage
		memcpy( &pDLayer->sMso.sSliceArgument.uiSliceMbNum[0], &sLayerCtx.sMso.sSliceArgument.uiSliceMbNum[0], sizeof(sLayerCtx.sMso.sSliceArgument.uiSliceMbNum) );	// confirmed_safe_unsafe_usage
	}

	return iRet;
}

int ParseCommandLine( int argc, char ** argv, SVCEncodingParam & sParam)
{
	char * pCmd;
	int i = 0;

	if (argc <= 0) // no additional pCmd parameters 
		return 0;

	while ( i < argc )
	{
		pCmd = argv[i];

		if( !strcmp(pCmd, "-numl") ) {	// confirmed_safe_unsafe_usage
			int  iNumSpatial = atoi(argv[i+1]);
			sParam.iSpatialLayerNum = iNumSpatial;
			i += 2;
		} else if( !strcmp(pCmd, "-numt") ) {	// confirmed_safe_unsafe_usage
			int  iNumTemporal = atoi(argv[i+1]);
			sParam.iTemporalLayerNum = iNumTemporal;
			i += 2;
		} else if( !strcmp(pCmd,"-iper") ) {	// confirmed_safe_unsafe_usage
			int iPeriod = atoi(argv[i+1]);
			sParam.iIntraPeriod = iPeriod;
			i += 2;
		}
		else if( !strcmp(pCmd,"-spsid") ) {	// confirmed_safe_unsafe_usage
			int iSpsPpsId = atoi(argv[i+1]);
			sParam.bEnableSpsPpsIdAddition = iSpsPpsId?true:false;
			i += 2;
		} 
		else if( !strcmp(pCmd,"-denois") ) {	// confirmed_safe_unsafe_usage
			int iDenois = atoi(argv[i+1]);
			sParam.bEnableDenoise = iDenois?true:false;
			i += 2;
		} else if( !strcmp(pCmd,"-bgd") ) {	// confirmed_safe_unsafe_usage
			int iBgd = atoi(argv[i+1]);
			sParam.bEnableBackgroundDetection = iBgd?true:false;
			i += 2;
		} else if( !strcmp(pCmd,"-aq") ) {	// confirmed_safe_unsafe_usage
			int iAq = atoi(argv[i+1]);
			sParam.bEnableAdaptiveQuant = iAq?true:false;
			i += 2;
		} else if( !strcmp(pCmd,"-ltr") ) {	// confirmed_safe_unsafe_usage
			int iLtr = atoi(argv[i+1]);
			sParam.bEnableLongTermReference = iLtr?true:false;
			i += 2;
		} else if( !strcmp(pCmd,"-ltrper") ) {	// confirmed_safe_unsafe_usage
			int iLtrPer = atoi(argv[i+1]);
			sParam.iLtrMarkPeriod = iLtrPer;
			i += 2;	
		} else if( !strcmp(pCmd,"-rcm") ) {	// confirmed_safe_unsafe_usage
			int iRcMode = atoi(argv[i+1]);
			sParam.iRCMode = iRcMode;
			i += 2;
		} else if( !strcmp(pCmd,"-tarb") ) {	// confirmed_safe_unsafe_usage
			int iTarB = atoi(argv[i+1]);
			sParam.iTargetBitrate = iTarB;
			i += 2;
		} else if( !strcmp(pCmd,"-ltarb") )	// confirmed_safe_unsafe_usage
		{
			int	iLayer = atoi( argv[i+1] );
			int iSpatialBitrate = atoi( argv[i+2] );
			sParam.sSpatialLayers[iLayer].iSpatialBitrate	= iSpatialBitrate;
			i += 3;
                } else if( !strcmp(pCmd,"-trace") ) {
                        int32_t iLog = atoi (argv[i+1]);
                        WelsStderrSetTraceLevel(iLog);
                        i += 2;
                }
else {
			i ++;
		}		
	}

    return 0;
}

void PrintHelp()
{
	printf("\n Wels SVC Encoder Usage:\n\n");
	printf(" Syntax: welsenc.exe welsenc.cfg\n");
	printf(" Syntax: welsenc.exe welsenc.cfg [options]\n");

	printf("\n Supported Options:\n");
	printf("  -h      Print Help\n");
	printf("  -bf     Bit Stream File\n");
	printf("  -frms   Number of total frames to be encoded\n");
	printf("  -gop    GOPSize - GOP size (2,4,8,16,32,64, default: 1)\n");
	printf("  -iper   Intra period (default: -1) : must be a power of 2 of GOP size (or -1)\n");
	printf("  -spsid   Enable id adding in SPS/PPS per IDR \n");
	printf("  -denois Control denoising  (default: 0)\n");
	printf("  -scene  Control scene change detection (default: 0)\n");
	printf("  -bgd    Control background detection (default: 0)\n");
	printf("  -aq     Control adaptive quantization (default: 0)\n");
	printf("  -ltr    Control long term reference (default: 0)\n");
	printf("  -rc	  Control rate control: 0-disable; 1-enable \n");
	printf("  -tarb	  Overall target bitrate\n");
	printf("  -numl   Number Of Layers: Must exist with layer_cfg file and the number of input layer_cfg file must equal to the value set by this command\n");
	printf("  The options below are layer-based: (need to be set with layer id)\n");
	printf("  -org		(Layer) (original file); example: -org 0 src.yuv\n");
	printf("  -drec		(Layer) (reconstruction file); Setting the reconstruction file, this will only functioning when dumping reconstruction is enabled\n");
	printf("  -sw		(Layer) (source width)\n");
	printf("  -sh		(Layer) (source height)\n");
	printf("  -frin		(Layer) (input frame rate)\n");
	printf("  -frout  	(Layer) (output frame rate)\n");
	printf("  -lqp		(Layer) (base quality layer qp : must work with -ldeltaqp or -lqparr)\n");
	printf("  -ltarb	    (Layer) (spatial layer target bitrate)\n");
	printf("  -slcmd   (Layer) (spatial layer slice mode): pls refer to layerX.cfg for details ( -slcnum: set target slice num; -slcsize: set target slice size constraint ) \n");
	printf("\n");
}

int ParseCommandLine(int argc, char** argv, SWelsSvcCodingParam & pSvcParam, SFilesSet& sFileSet) 
{
	char* pCommand = NULL;
	char* pTemp = NULL;
	unsigned int uiQpChangeFlag[4] = {0};
	unsigned int uiQlPredModeChangeFlag[4] = {0};
	SLayerPEncCtx sLayerCtx[3];
	int n = 0;
	string str_("SlicesAssign");
	const int kiSize = str_.size();

	if (argc <= 0) // no additional pCmd parameters 
		return 0;

	while(n < argc)
	{
		pCommand = argv[n++];
		if (!(strcmp(pCommand,"-h")))	// confirmed_safe_unsafe_usage
		{
			PrintHelp();
			continue;
		}
		if (!(strcmp(pCommand,"-bf")))	// confirmed_safe_unsafe_usage
		{			
			sFileSet.strBsFile.assign(argv[n]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-frms")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.uiFrameToBeCoded = atoi(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-gop")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.uiGopSize = atoi(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-iper")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.uiIntraPeriod = atoi(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-spsid")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableSpsPpsIdAddition = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-denois")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableDenoise = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-scene")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableSceneChangeDetect = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if ( !(strcmp(pCommand,"-bgd")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableBackgroundDetection = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-aq")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableAdaptiveQuant = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-ltr")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableLongTermReference = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-ltrper")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.uiLtrMarkPeriod = atoi(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-rc")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.bEnableRc = atoi(argv[n ])?true:false;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-tarb")) )	// confirmed_safe_unsafe_usage
		{
			pSvcParam.iTargetBitrate = atoi(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-numl")) )	// confirmed_safe_unsafe_usage
		{
			bool_t bFound = false;
			pSvcParam.iNumDependencyLayer = atoi(argv[n++]);
			for (int ln = 0 ; ln < pSvcParam.iNumDependencyLayer ; ln++)
			{
//				pSvcParam.sDependencyLayers[ln].uiDependencyId = ln;				
				sFileSet.sSpatialLayers[ln].strLayerCfgFile.assign( argv[n] );
				++ n;
			}

			for (int8_t iLayer = 0; iLayer < pSvcParam.iNumDependencyLayer; ++ iLayer){
				SLayerPEncCtx sLayerCtx;	
				string strTag[4];
				int32_t iLayerArg = -2;
				int32_t iNumQualityBitrateLayerSet = 0;

				SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
				CReadConfig cRdLayerCfg( sFileSet.sSpatialLayers[iLayer].strLayerCfgFile );

				memset(&sLayerCtx, 0, sizeof(SLayerPEncCtx));

//				pDLayer->frext_mode = 0;
				if ( !cRdLayerCfg.ExistFile() ){
					fprintf(stderr, "Unabled to open layer #%d configuration file: %s.\n", iLayer, cRdLayerCfg.GetFileName().c_str());
					continue;
				}
				
				while ( !cRdLayerCfg.EndOfFile() ){
					long iLayerRd = cRdLayerCfg.ReadLine(&strTag[0]);
					if (iLayerRd > 0){
						if ( strTag[0].empty() )
							continue;
						if (strTag[0].compare("SourceWidth") == 0){
							pDLayer->iFrameWidth	= atoi(strTag[1].c_str());
							pDLayer->iActualWidth= pDLayer->iFrameWidth;
							continue;
						}
						else if (strTag[0].compare("SourceHeight") == 0){
							pDLayer->iFrameHeight	= atoi(strTag[1].c_str());
							pDLayer->iActualHeight	= pDLayer->iFrameHeight;
							continue;
						}
						else if (strTag[0].compare("FrameRateIn") == 0){
							pDLayer->fInputFrameRate	= (float)atof(strTag[1].c_str());
							continue;
						}
						else if (strTag[0].compare("FrameRateOut") == 0){
							pDLayer->fOutputFrameRate = (float)atof(strTag[1].c_str());
							continue;
						}
						else if (strTag[0].compare("InputFile") == 0){							
							if ( strTag[1].length() > 0 )
								sFileSet.sSpatialLayers[iLayer].strSeqFile = strTag[1];
							continue;
						}
						else if (strTag[0].compare("ReconFile") == 0){
#ifdef ENABLE_FRAME_DUMP
							const int kiLen = strTag[1].length();
							if (kiLen >= MAX_FNAME_LEN)
								return 1;
							pDLayer->sRecFileName[kiLen] = '\0';
							strncpy(pDLayer->sRecFileName, strTag[1].c_str(), kiLen);	// confirmed_safe_unsafe_usage
#endif//ENABLE_FRAME_DUMP
							continue;
						}
						else if (strTag[0].compare("ProfileIdc") == 0){
							pDLayer->uiProfileIdc	= atoi(strTag[1].c_str());
							continue;
						}
						else if (strTag[0].compare("FRExt") == 0){
//							pDLayer->frext_mode	= (bool_t)atoi(strTag[1].c_str());
							continue;
						}	
						if (strTag[0].compare("SpatialBitrate") == 0){
							pDLayer->iSpatialBitrate	= 1000 * atoi(strTag[1].c_str());
							continue;
						}

						if (strTag[0].compare("InitialQP") == 0){
							sLayerCtx.iDLayerQp	= atoi(strTag[1].c_str());
							continue;
						}

						if (strTag[0].compare("SliceMode") == 0){
							sLayerCtx.sMso.uiSliceMode	= (SliceMode)atoi(strTag[1].c_str());
							continue;
						}
						else if (strTag[0].compare("SliceSize") == 0){//SM_DYN_SLICE
							sLayerCtx.sMso.sSliceArgument.uiSliceSizeConstraint	= (SliceMode)atoi(strTag[1].c_str());
							continue;
						}
						else if (strTag[0].compare("SliceNum") == 0){
							sLayerCtx.sMso.sSliceArgument.iSliceNum = atoi(strTag[1].c_str());
							continue;
						}
						else if ( strTag[0].compare(0, kiSize, str_ ) == 0 )
						{
							const char* kpString = strTag[0].c_str();
							int uiSliceIdx = atoi(&kpString[kiSize]);
							assert( uiSliceIdx < MAX_SLICES_NUM );
							sLayerCtx.sMso.sSliceArgument.uiSliceMbNum[uiSliceIdx] = atoi( strTag[1].c_str() );
							continue;
						}
					}
				}
				pDLayer->iDLayerQp		= sLayerCtx.iDLayerQp;
				pDLayer->sMso.uiSliceMode		= sLayerCtx.sMso.uiSliceMode;		
	memcpy( &pDLayer->sMso, &sLayerCtx.sMso, sizeof(SMulSliceOption) );	// confirmed_safe_unsafe_usage
		memcpy( &pDLayer->sMso.sSliceArgument.uiSliceMbNum[0], &sLayerCtx.sMso.sSliceArgument.uiSliceMbNum[0], sizeof(sLayerCtx.sMso.sSliceArgument.uiSliceMbNum) );	// confirmed_safe_unsafe_usage

			}
			//n += 1;
			continue;
		}
		if( !(strcmp(pCommand,"-org")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			sFileSet.sSpatialLayers[iLayer].strSeqFile.assign( argv[n] );
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-drec")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			const int iLen = strlen(argv[n]);	// confirmed_safe_unsafe_usage
#ifdef ENABLE_FRAME_DUMP
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->sRecFileName[iLen] = '\0';
			strncpy(pDLayer->sRecFileName, argv[n], iLen);	// confirmed_safe_unsafe_usage
#endif//ENABLE_FRAME_DUMP
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-sw")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->iFrameWidth =  atoi(argv[n ]);
			pDLayer->iActualWidth= pDLayer->iFrameWidth;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-sh")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->iFrameHeight =  atoi(argv[n ]);
			pDLayer->iActualHeight= pDLayer->iFrameHeight;
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-frin")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->fInputFrameRate =  (float)atof(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-frout")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->fOutputFrameRate =  (float)atof(argv[n ]);
			++ n;
			continue;
		}	

		if( !(strcmp(pCommand,"-lqp")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			uiQpChangeFlag[iLayer] = 1;
			pDLayer->iDLayerQp = sLayerCtx[iLayer].iDLayerQp=  atoi(argv[n ]);
			n += 1;
			continue;
		}
		//sLayerCtx[iLayer].num_quality_layers = pDLayer->num_quality_layers = 1;

		if( !(strcmp(pCommand,"-ltarb")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->iSpatialBitrate	= 1000 * atoi(argv[n ]);
			++ n;
			continue;
		}

		if( !(strcmp(pCommand,"-slcmd")) )	// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];

			switch ( atoi(argv[n] ) )
			{
			case 0: 
				pDLayer->sMso.uiSliceMode = SM_SINGLE_SLICE;
				break;
			case 1: 
				pDLayer->sMso.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
				break;
			case 2: 
				pDLayer->sMso.uiSliceMode = SM_RASTER_SLICE;
				break;
			case 3: 
				pDLayer->sMso.uiSliceMode = SM_ROWMB_SLICE;
				break;
			case 4: 
				pDLayer->sMso.uiSliceMode = SM_DYN_SLICE;
				break;
			default: 
				pDLayer->sMso.uiSliceMode = SM_RESERVED;
				break;
			}
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-slcsize")) )//confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->sMso.sSliceArgument.uiSliceSizeConstraint = atoi(argv[n ]);
			++ n;
			continue;
		}
		if( !(strcmp(pCommand,"-slcnum")) )// confirmed_safe_unsafe_usage
		{
			unsigned int	iLayer = atoi( argv[n++] );
			SDLayerParam *pDLayer = &pSvcParam.sDependencyLayers[iLayer];
			pDLayer->sMso.sSliceArgument.iSliceNum = atoi(argv[n ]);
			++ n;
			continue;
		}
	}
	return 0;
}



int FillSpecificParameters( SVCEncodingParam &sParam )
{
	/* Test for temporal, spatial, SNR scalability */
	sParam.fFrameRate	= 30.0f;		// input frame rate  
	sParam.iPicWidth		= 1280;			// width of picture in samples
	sParam.iPicHeight	= 720;			// height of picture in samples
	sParam.iTargetBitrate= 2500000;		// target bitrate desired
	sParam.iRCMode       = 0;            //  rc mode control
	sParam.iTemporalLayerNum= 3;	// layer number at temporal level
	sParam.iSpatialLayerNum	= 4;	// layer number at spatial level
	sParam.bEnableDenoise    = 0;    // denoise control
	sParam.bEnableBackgroundDetection = 1; // background detection control	
	sParam.bEnableAdaptiveQuant       = 1; // adaptive quantization control
	sParam.bEnableLongTermReference  = 0; // long term reference control
	sParam.iLtrMarkPeriod = 30;

	sParam.iInputCsp			= videoFormatI420;			// color space of input sequence
	sParam.iKeyPicCodingMode= 1;// mode of key picture coding
	sParam.iIntraPeriod		= 320;		// period of Intra frame
	sParam.bEnableSpsPpsIdAddition = 1;
	sParam.bPrefixNalAddingCtrl = 1;

	int iIndexLayer = 0;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth	= 160;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight	= 90;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate	= 7.5f;
	sParam.sSpatialLayers[iIndexLayer].iQualityLayerNum	    = 1;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate		= 64000;
	sParam.sSpatialLayers[iIndexLayer].iCgsSnrRefined		= 0;
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[0]	= 0;
//	memset(sParam.iTemporalBitrate, 0, sizeof(sParam.iTemporalBitrate));
	sParam.sSpatialLayers[iIndexLayer].iInterSpatialLayerPredFlag	= 0;
#ifdef MT_ENABLED
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = 0;  
#endif

	++ iIndexLayer;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth	= 320;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight	= 180;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate	= 15.0f;
	sParam.sSpatialLayers[iIndexLayer].iQualityLayerNum	    = 1;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate		= 160000;
	sParam.sSpatialLayers[iIndexLayer].iCgsSnrRefined		= 0;
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[0]	= 0;	
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[1]	= 0;	
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[2]	= 0;
	sParam.sSpatialLayers[iIndexLayer].iInterSpatialLayerPredFlag	= 0;	
#ifdef MT_ENABLED
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = 0; 
#endif

	++ iIndexLayer;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth	= 640;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight	= 360;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate	= 30.0f;
	sParam.sSpatialLayers[iIndexLayer].iQualityLayerNum	    = 1;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate		= 512000;
	sParam.sSpatialLayers[iIndexLayer].iCgsSnrRefined		= 0;
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[0]	= 0;	
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[1]	= 0;	
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[2]	= 0;
	sParam.sSpatialLayers[iIndexLayer].iInterSpatialLayerPredFlag	= 0;
#ifdef MT_ENABLED
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = 0;                  
    sParam.sSpatialLayers[iIndexLayer].sSliceCfg.sSliceArgument.uiSliceNum = 1;    
#endif

	++ iIndexLayer;
	sParam.sSpatialLayers[iIndexLayer].iVideoWidth	= 1280;
	sParam.sSpatialLayers[iIndexLayer].iVideoHeight	= 720;
	sParam.sSpatialLayers[iIndexLayer].fFrameRate	= 30.0f;
	sParam.sSpatialLayers[iIndexLayer].iQualityLayerNum	    = 1;
	sParam.sSpatialLayers[iIndexLayer].iSpatialBitrate		= 1500000;
	sParam.sSpatialLayers[iIndexLayer].iCgsSnrRefined		= 0;
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[0]	= 0;	
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[1]	= 0;	
//	sParam.sSpatialLayers[iIndexLayer].iQualityBitrate[2]	= 0;
	sParam.sSpatialLayers[iIndexLayer].iInterSpatialLayerPredFlag	= 0;
#ifdef MT_ENABLED
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.uiSliceMode = 0;  
	sParam.sSpatialLayers[iIndexLayer].sSliceCfg.sSliceArgument.uiSliceNum = 1; 
#endif

	float fMaxFr = sParam.sSpatialLayers[sParam.iSpatialLayerNum-1].fFrameRate;
	for (int32_t i = sParam.iSpatialLayerNum-2; i >= 0; -- i)
	{
		if (sParam.sSpatialLayers[i].fFrameRate > fMaxFr+EPSN)
			fMaxFr = sParam.sSpatialLayers[i].fFrameRate;
	}
	sParam.fFrameRate = fMaxFr;

	return 0;
}

/* For SVC Demo test */
int ProcessEncodingSvcWithParam ( ISVCEncoder *pPtrEnc, int argc, char ** argv )
{
    const char * kpSrcFile = argv[1];
	const char * kpStrBsFile = argv[2];

	if ( pPtrEnc == NULL || kpSrcFile == NULL || kpStrBsFile == NULL )
		return 1;

	FILE *pFpBs = NULL;
	FILE *pFpSrc= NULL;
	SFrameBSInfo sFbi;
	SVCEncodingParam sSvcParam;
	int64_t iStart = 0, iTotal = 0;
#if defined ( STICK_STREAM_SIZE )
	FILE *fTrackStream = fopen("coding_size.stream", "wb");;
#endif

	pFpSrc	= fopen(kpSrcFile, "rb");
	if ( NULL == pFpSrc )
		return 1;
	pFpBs	= fopen(kpStrBsFile, "wb");
	if ( NULL == pFpBs){
		fclose( pFpSrc );
		pFpSrc = NULL;
		return 1;
	}

	memset( &sFbi, 0, sizeof(SFrameBSInfo) );
	memset( &sSvcParam, 0, sizeof(SVCEncodingParam) );

	FillSpecificParameters(sSvcParam);

	int iParsedNum = 3;
	if( ParseCommandLine(argc-iParsedNum, argv+iParsedNum, sSvcParam) != 0 )
	{
		printf("parse pCommand line failed\n");
		return 1;
	}

	if ( cmResultSuccess != pPtrEnc->Initialize( &sSvcParam, INIT_TYPE_PARAMETER_BASED ) )
	{
		fprintf(stderr, "Encoder Initialization failed!\n");
		return 1;
	}

	const int32_t iPicLumaSize = sSvcParam.iPicWidth * sSvcParam.iPicHeight;
	int32_t iFrameSize = 0;
	uint8_t *pPlanes[3] = { 0 };

	switch( sSvcParam.iInputCsp ) {
		int iStride;
	case videoFormatI420:
	case videoFormatYV12:
		iFrameSize  = (3 * iPicLumaSize)>>1;
		pPlanes[0]	= new uint8_t[iFrameSize];
		pPlanes[1]	= pPlanes[0] + iPicLumaSize;
		pPlanes[2]	= pPlanes[1]	+ (iPicLumaSize>>2);
		break;	
	case videoFormatYUY2:
	case videoFormatYVYU:
	case videoFormatUYVY:
		iStride      = CALC_BI_STRIDE(sSvcParam.iPicWidth,  16);
		iFrameSize  = iStride * sSvcParam.iPicHeight;
		pPlanes[0]   = new uint8_t[iFrameSize];
		break;
	case videoFormatRGB:
	case videoFormatBGR:
		iStride      = CALC_BI_STRIDE(sSvcParam.iPicWidth,  24);
		iFrameSize  = iStride * sSvcParam.iPicHeight;
		pPlanes[0]	= new uint8_t[iFrameSize];
		break;
	case videoFormatBGRA:
	case videoFormatRGBA:
	case videoFormatARGB:
	case videoFormatABGR:
		iStride = 4 * sSvcParam.iPicWidth;
		iFrameSize  = iStride * sSvcParam.iPicHeight;
		pPlanes[0]	= new uint8_t[iFrameSize];
		break;
	default:
		return 1;
	}
	
	int32_t iFrame = 0;
	while (true) 
	{
		if ( feof(pFpSrc) )
			break;
#ifdef ONLY_ENC_FRAMES_NUM
		if ( iFrame >= ONLY_ENC_FRAMES_NUM )
			break;
#endif//ONLY_ENC_FRAMES_NUM
		if ( fread(pPlanes[0], sizeof(uint8_t), iFrameSize, pFpSrc) <= 0 )
				break;

		iStart	= WelsTime();
		long iEncode = pPtrEnc->EncodeFrame( pPlanes[0], &sFbi);
		iTotal += WelsTime() - iStart;
		if ( videoFrameTypeInvalid == iEncode ){
			fprintf(stderr, "EncodeFrame() failed: %d.\n", iEncode);
			break;
		}

		/* Write bit-stream */
		if ( pFpBs != NULL && videoFrameTypeSkip != iEncode ){	// file handler to write bit stream
			int iLayer = 0;
			while ( iLayer < sFbi.iLayerNum ){
				SLayerBSInfo *pLayerBsInfo = &sFbi.sLayerInfo[iLayer];
				if ( pLayerBsInfo != NULL ){
					int iLayerSize = 0;
					int iNalIdx = pLayerBsInfo->iNalCount -1;
					do {
						iLayerSize += pLayerBsInfo->iNalLengthInByte[iNalIdx];
						-- iNalIdx;
					} while(iNalIdx >= 0);
					fwrite(pLayerBsInfo->pBsBuf, 1, iLayerSize, pFpBs);	// write pure bit stream into file
				}
				++ iLayer;
			}
			++ iFrame;
		}		
	}

	if (iFrame > 0){
		double dElapsed = iTotal / 1e6;
		printf( "Frames:		%d\nencode time:	%f sec\nFPS:		%f fps\n", iFrame, dElapsed, (iFrame * 1.0)/dElapsed );
	}

	if ( NULL != pPlanes[0] )
	{
        delete [] pPlanes[0];
		pPlanes[0] = NULL;
	}

	if ( pFpBs ){
		fclose( pFpBs );
		pFpBs = NULL;
	}
	if ( pFpSrc ){
		fclose( pFpSrc );
		pFpSrc= NULL;
	}

	return 0;
}


int ProcessEncodingSvcWithConfig ( ISVCEncoder *pPtrEnc, int argc, char **argv )
{
	int iRet				= 0;	 

	if ( pPtrEnc == NULL )	
		return 1;
	
	SFrameBSInfo sFbi;
	SWelsSvcCodingParam sSvcParam;
	int64_t iStart = 0, iTotal = 0;

	// Preparing encoding process
	FILE* pFileYUV[MAX_DEPENDENCY_LAYER] = {0};
	int32_t iActualFrameEncodedCount = 0;
	int32_t iFrameIdx = 0;
	int32_t	iTotalFrameMax = -1;
	int8_t  iDlayerIdx = 0;
	uint8_t * pYUV[MAX_DEPENDENCY_LAYER] = { 0 };
	SSourcePicture  **  pSrcPicList = NULL;
#if (defined(RUN_SIMULATOR) || defined(WIN32)||defined(_MACH_PLATFORM) || (defined(__GNUC__)))
	// Inactive with sink with output file handler
	FILE *pFpBs = NULL;
#endif
#if defined(COMPARE_DATA)
	//For getting the golden file handle
	FILE *fpGolden = NULL;
#endif
#if defined ( STICK_STREAM_SIZE )
	FILE *fTrackStream = fopen("coding_size.stream", "wb");;
#endif
	SFilesSet fs;
	// for configuration file
	CReadConfig cRdCfg;
	int iParsedNum = 2;

	memset(&sFbi, 0, sizeof(SFrameBSInfo));
	memset(&sSvcParam, 0, sizeof(SWelsSvcCodingParam));	

	sSvcParam.iInputCsp	= videoFormatI420;	// I420 in default
	sSvcParam.sDependencyLayers[0].uiProfileIdc	= PRO_BASELINE;
//	svc_cfg->sDependencyLayers[0].frext_mode	= 0;

	// for configuration file
	cRdCfg.Openf(argv[1]);
	if ( !cRdCfg.ExistFile() ){
		fprintf(stderr, "Specified file: %s not exist, maybe invalid path or parameter settting.\n", cRdCfg.GetFileName().c_str());
		iRet = 1;
		goto INSIDE_MEM_FREE;
	}	

	iRet = ParseConfig(cRdCfg, sSvcParam, fs);	
	if ( iRet ){
		fprintf(stderr, "parse svc parameter config file failed.\n");
		iRet = 1;
		goto INSIDE_MEM_FREE;
	}
	
	if ( ParseCommandLine(argc-iParsedNum, argv+iParsedNum, sSvcParam, fs) != 0 )
	{
		printf("parse pCommand line failed\n");
		iRet = 1;
		goto INSIDE_MEM_FREE;
	}	

	iTotalFrameMax = (int32_t)sSvcParam.uiFrameToBeCoded;
	sSvcParam.SUsedPicRect.iLeft = 0;
	sSvcParam.SUsedPicRect.iTop = 0;
//	sSvcParam.max_pic_width	= 
	sSvcParam.iActualPicWidth =
	sSvcParam.SUsedPicRect.iWidth = sSvcParam.sDependencyLayers[sSvcParam.iNumDependencyLayer-1].iFrameWidth;
//	pSvcParam.max_pic_height	= 
	sSvcParam.iActualPicHeight =
	sSvcParam.SUsedPicRect.iHeight = sSvcParam.sDependencyLayers[sSvcParam.iNumDependencyLayer-1].iFrameHeight;	
	
	if ( cmResultSuccess != pPtrEnc->Initialize((void *)&sSvcParam, INIT_TYPE_CONFIG_BASED) )	// SVC encoder initialization
	{
		fprintf( stderr, "SVC encoder Initialize failed\n");
		iRet = 1;
		goto INSIDE_MEM_FREE;
	}
#if (defined(RUN_SIMULATOR) || defined(WIN32)||defined(_MACH_PLATFORM) || (defined(__GNUC__)))
	// Inactive with sink with output file handler	
	if ( fs.strBsFile.length() > 0 ){
		pFpBs = fopen (fs.strBsFile.c_str(), "wb");
		if (pFpBs == NULL){
			fprintf( stderr, "Can not open file (%s) to write bitstream!\n", fs.strBsFile.c_str() );
			iRet = 1;
			goto INSIDE_MEM_FREE;
		}
	}
#endif	
	
#if defined(COMPARE_DATA)
	//For getting the golden file handle	
	if((fpGolden = fopen(argv[3], "rb")) == NULL) 
	{
		fprintf(stderr, "Unable to open golden sequence file, check corresponding path!\n");
		iRet = 1;
		goto INSIDE_MEM_FREE;
	}
#endif

	pSrcPicList = new SSourcePicture * [sSvcParam.iNumDependencyLayer];		
	while (iDlayerIdx < sSvcParam.iNumDependencyLayer) {
		SDLayerParam *pDLayer = &sSvcParam.sDependencyLayers[iDlayerIdx];			
		const int kiPicResSize = pDLayer->iFrameWidth * pDLayer->iFrameHeight;
		SSourcePicture * pSrcPic = new SSourcePicture;
		if( pSrcPic == NULL ){
			iRet = 1;
			goto INSIDE_MEM_FREE;
		}
		memset(pSrcPic, 0, sizeof(SSourcePicture));
		
		pYUV[iDlayerIdx] = new uint8_t [(3*kiPicResSize)>>1];
		if (pYUV[iDlayerIdx] == NULL)
		{
			iRet = 1;
			goto INSIDE_MEM_FREE;
		}

		pSrcPic->iColorFormat = videoFormatI420;
		pSrcPic->iPicWidth = pDLayer->iFrameWidth;
		pSrcPic->iPicHeight = pDLayer->iFrameHeight;
		pSrcPic->iStride[0] = pDLayer->iFrameWidth;
		pSrcPic->iStride[1] = pSrcPic->iStride[2] = pDLayer->iFrameWidth >> 1;

		pSrcPicList[iDlayerIdx] = pSrcPic;		

		pFileYUV[iDlayerIdx]	= fopen( fs.sSpatialLayers[iDlayerIdx].strSeqFile.c_str(), "rb");
		if (pFileYUV[iDlayerIdx] != NULL){
			if( !fseek( pFileYUV[iDlayerIdx], 0, SEEK_END ) )
			{
				int64_t i_size = ftell( pFileYUV[iDlayerIdx] );
				fseek( pFileYUV[iDlayerIdx], 0, SEEK_SET );
				iTotalFrameMax = WELS_MAX( (int32_t)(i_size / ((3*kiPicResSize)>>1) ), iTotalFrameMax );
			}
		}
		else{
			fprintf(stderr, "Unable to open source sequence file (%s), check corresponding path!\n", fs.sSpatialLayers[iDlayerIdx].strSeqFile.c_str());
			iRet = 1;
			goto INSIDE_MEM_FREE;
		}			

		++ iDlayerIdx;
	}
	
	iFrameIdx = 0;
	while (iFrameIdx < iTotalFrameMax && (((int32_t)sSvcParam.uiFrameToBeCoded <= 0) || (iFrameIdx < (int32_t)sSvcParam.uiFrameToBeCoded)) ) {
		bool_t bOnePicAvailableAtLeast = false;
		bool_t bSomeSpatialUnavailable	  = false;

#ifdef ONLY_ENC_FRAMES_NUM
		// Only encoded some limited frames here
		if ( iActualFrameEncodedCount >= ONLY_ENC_FRAMES_NUM )
		{
			break;
		}
#endif//ONLY_ENC_FRAMES_NUM

		iDlayerIdx = 0;
        int  nSpatialLayerNum = 0;
		while (iDlayerIdx < sSvcParam.iNumDependencyLayer) {
			SDLayerParam * pDLayer = &sSvcParam.sDependencyLayers[iDlayerIdx];
			const int kiPicResSize = ((pDLayer->iFrameWidth * pDLayer->iFrameHeight)*3)>>1;			
			uint32_t uiSkipIdx = (1 << pDLayer->iTemporalResolution);
			
			bool_t bCanBeRead= false;

			if ( iFrameIdx % uiSkipIdx == 0 )	// such layer is enabled to encode indeed
			{				
				bCanBeRead = (fread(pYUV[iDlayerIdx], 1, kiPicResSize, pFileYUV[iDlayerIdx]) == kiPicResSize);
				
				if ( bCanBeRead )
				{										
					bOnePicAvailableAtLeast	= true;					

					pSrcPicList[nSpatialLayerNum]->pData[0] = pYUV[iDlayerIdx];
					pSrcPicList[nSpatialLayerNum]->pData[1] = pSrcPicList[nSpatialLayerNum]->pData[0] +
						(pDLayer->iFrameWidth * pDLayer->iFrameHeight);
					pSrcPicList[nSpatialLayerNum]->pData[2] = pSrcPicList[nSpatialLayerNum]->pData[1] + 
						((pDLayer->iFrameWidth * pDLayer->iFrameHeight)>>2);

					pSrcPicList[nSpatialLayerNum]->iPicWidth = pDLayer->iFrameWidth;
					pSrcPicList[nSpatialLayerNum]->iPicHeight = pDLayer->iFrameHeight;
					pSrcPicList[nSpatialLayerNum]->iStride[0] = pDLayer->iFrameWidth;
					pSrcPicList[nSpatialLayerNum]->iStride[1] = pSrcPicList[nSpatialLayerNum]->iStride[2]
					  = pDLayer->iFrameWidth >> 1;

					++ nSpatialLayerNum;
				}
				else	// file end while reading
				{
					bSomeSpatialUnavailable = true;
					break;
				}
			}
			else
			{					
				
			}		
			
			++ iDlayerIdx;			
		}

		if ( bSomeSpatialUnavailable )
			break;

		if ( !bOnePicAvailableAtLeast ){
			++ iFrameIdx;
			continue;
		}		
		
		// To encoder this frame
		iStart	= WelsTime();			
		int iEncFrames = pPtrEnc->EncodeFrame(const_cast<const SSourcePicture**>(pSrcPicList), nSpatialLayerNum, &sFbi);
		iTotal += WelsTime() - iStart;		

		// fixed issue in case dismatch source picture introduced by frame skipped, 1/12/2010
		if ( videoFrameTypeSkip == iEncFrames )
		{
			continue;
		}

		if ( iEncFrames != videoFrameTypeInvalid && iEncFrames != videoFrameTypeSkip )
		{
			int iLayer = 0;
			int iFrameSize = 0;
			while ( iLayer < sFbi.iLayerNum ){
				SLayerBSInfo *pLayerBsInfo = &sFbi.sLayerInfo[iLayer];
				if ( pLayerBsInfo != NULL ){
					int iLayerSize = 0;
					int iNalIdx = pLayerBsInfo->iNalCount -1;
					do {
						iLayerSize += pLayerBsInfo->iNalLengthInByte[iNalIdx];
						-- iNalIdx;
					} while(iNalIdx >= 0);
#if defined(COMPARE_DATA)
						//Comparing the result of encoder with golden pData
                        {
							unsigned char *pUCArry = new unsigned char [iLayerSize];
							
							fread(pUCArry, 1, iLayerSize, fpGolden);

							for (int w=0; w<iLayerSize; w++) {
								if (pUCArry[w] != pLayerBsInfo->pBsBuf[w]) {
									fprintf(stderr, "error @frame%d/layer%d/byte%d!!!!!!!!!!!!!!!!!!!!!!!!\n", iFrameIdx, iLayer, w);
									//fprintf(stderr, "%x - %x\n", pUCArry[w], pLayerBsInfo->pBsBuf[w]);									
									break;
								}
							}
							fprintf( stderr, "frame%d/layer%d comparation completed!\n", iFrameIdx, iLayer);
							
							delete [] pUCArry;
						} 
#endif
#if (defined(RUN_SIMULATOR) || defined(WIN32)||defined(_MACH_PLATFORM) || (defined(__GNUC__)))
					fwrite(pLayerBsInfo->pBsBuf, 1, iLayerSize, pFpBs);	// write pure bit stream into file
#endif					
					iFrameSize += iLayerSize;
				}
				++ iLayer;
			}
#if defined (STICK_STREAM_SIZE)
			if ( fTrackStream ){
				fwrite( &iFrameSize, 1, sizeof(int), fTrackStream );
			}
#endif//STICK_STREAM_SIZE
			++ iActualFrameEncodedCount;	// excluding skipped frame time
		}
		else{
			fprintf(stderr, "EncodeFrame(), ret: %d, frame index: %d.\n", iEncFrames, iFrameIdx);
		}

		++ iFrameIdx;
	}

	if (iActualFrameEncodedCount > 0){
		double dElapsed = iTotal / 1e6;
		printf( "Width:		%d\nHeight:		%d\nFrames:		%d\nencode time:	%f sec\nFPS:		%f fps\n",
			sSvcParam.iActualPicWidth, sSvcParam.iActualPicHeight,
			iActualFrameEncodedCount, dElapsed, (iActualFrameEncodedCount * 1.0)/dElapsed );
	}	

INSIDE_MEM_FREE:
	{
#if (defined(RUN_SIMULATOR) || defined(WIN32)||defined(_MACH_PLATFORM) || (defined(__GNUC__)))
	if (pFpBs)
	{
		fclose(pFpBs);
		pFpBs = NULL;
	}
#endif
#if defined (STICK_STREAM_SIZE)
	if ( fTrackStream ){
		fclose( fTrackStream );
		fTrackStream = NULL;
	}
#endif
#if defined (COMPARE_DATA)	
	if ( fpGolden ){
		fclose(fpGolden);
		fpGolden = NULL;
	}  
#endif
	// Destruction memory introduced in this routine
	iDlayerIdx = 0;	
	while (iDlayerIdx < sSvcParam.iNumDependencyLayer)
	{
		if (pFileYUV[iDlayerIdx] != NULL){
			fclose(pFileYUV[iDlayerIdx]);
			pFileYUV[iDlayerIdx] = NULL;
		}
		++ iDlayerIdx;		
	}	

	if( pSrcPicList ){
		for( int32_t i=0;i<sSvcParam.iNumDependencyLayer;i++ )
		{
			if( pSrcPicList[i] ){
				delete pSrcPicList[i];
				pSrcPicList[i] = NULL;
			}
		}
		delete pSrcPicList;
		pSrcPicList = NULL;
	}

	for( int32_t i=0;i<MAX_DEPENDENCY_LAYER;i++ ){
		if( pYUV[i] ){
			delete [] pYUV[i];
			pYUV[i] = NULL;
		}
	}
	}

	return iRet;
}

//  Merge from Heifei's Wonder.  Lock process to a single core
void LockToSingleCore()
{  
#ifdef _MSC_VER
	//for 2005 compiler, change "DWORD" to "DWORD_PTR"
	DWORD ProcessAffMask = 0, SystemAffMask = 0;
	HANDLE hProcess = GetCurrentProcess();

	GetProcessAffinityMask(hProcess, &ProcessAffMask, &SystemAffMask);
	if (ProcessAffMask > 1)
	{
		// more than one CPU core available. Fix to only one:
		if (ProcessAffMask & 2) 
		{
			ProcessAffMask = 2;
		}
		else 
		{
			ProcessAffMask = 1;
		}
		// Lock process to a single CPU core
		SetProcessAffinityMask(hProcess, ProcessAffMask);
	}

	// set high priority to avoid interrupts during test
	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
#endif
	return ;
}

long CreateSVCEncHandle(ISVCEncoder** ppEncoder)
{
	long ret = 0;
#if defined(MACOS)
	ret = WelsEncBundleLoad();
	WelsEncBundleCreateEncoder(ppEncoder);
#else
	ret = CreateSVCEncoder( ppEncoder );
#endif//MACOS
	return ret;
}

void DestroySVCEncHanlde(ISVCEncoder* pEncoder)
{
	if (pEncoder)
	{
#if defined(MACOS)
		WelsEncBundleDestroyEncoder(pEncoder);
#else
		DestroySVCEncoder( pEncoder );
#endif//MACOS

	}
}

/****************************************************************************
 * main:
 ****************************************************************************/
#if (defined(MACOS))
int main_demo( int argc, char **argv )
#else
int main( int argc, char **argv )
#endif
{	
	ISVCEncoder* pSVCEncoder	= NULL;
    FILE *pFileOut					= NULL; 
    FILE *pFileIn					= NULL;
	int iRet					= 0;
	
#ifdef _MSC_VER
	_setmode(_fileno(stdin), _O_BINARY);    /* thanks to Marcoss Morais <morais at dee.ufcg.edu.br> */
	_setmode(_fileno(stdout), _O_BINARY);

	// remove the LOCK_TO_SINGLE_CORE micro, user need to enable it with manual  
	// LockToSingleCore();
#endif

	/* Control-C handler */
	signal( SIGINT, SigIntHandler );

	iRet = CreateSVCEncHandle( &pSVCEncoder );
	if ( iRet )
	{
		cout << "CreateSVCEncoder() failed!!" << endl;		
		goto exit;
	}

	if (argc < 2)
	{
		goto exit;
	}
	else
	{
		string	strCfgFileName = argv[1];
		basic_string <char>::size_type index;
		static const basic_string <char>::size_type npos = size_t(-1);
		index = strCfgFileName.rfind(".cfg");	// check configuration type (like .cfg?)
		if ( index == npos )
		{
			if (argc > 2)
			{
				iRet = ProcessEncodingSvcWithParam( pSVCEncoder, argc, argv );
				if ( iRet != 0 )
					goto exit;
			}
			else
			{
				cout << "You specified pCommand is invalid!!" << endl;
				goto exit;
			}
		}
		else
		{
			iRet = ProcessEncodingSvcWithConfig( pSVCEncoder, argc, argv);
			if (iRet > 0)
				goto exit;
		}
	}

	DestroySVCEncHanlde( pSVCEncoder );
	return 0;

exit:
	DestroySVCEncHanlde( pSVCEncoder );
	PrintHelp();
	return 1;
}
