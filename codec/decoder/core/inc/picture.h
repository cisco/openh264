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

//picture.h     -       reconstruction picture/ reference picture/ residual picture are declared here
#ifndef WELS_PICTURE_H__
#define WELS_PICTURE_H__

#include "typedefs.h"

namespace WelsDec {

/*
 *  Reconstructed Picture definition
 *  It is used to express reference picture, also consequent reconstruction picture for output
 */
typedef struct TagPicture {
/************************************payload data*********************************/
uint8_t*        pBuffer[4];             // pointer to the first allocated byte, basical offset of buffer, dimension:
uint8_t*        pData[4];               // pointer to picture planes respectively
int32_t         iLinesize[4];// linesize of picture planes respectively used currently
int32_t         iPlanes;                        // How many planes are introduced due to color space format?
// picture information

/*******************************from EC mv copy****************************/
bool bIdrFlag;

/*******************************from other standard syntax****************************/
/*from sps*/
int32_t         iWidthInPixel;  // picture width in pixel
int32_t         iHeightInPixel;// picture height in pixel
/*from slice header*/
int32_t         iFramePoc;              // frame POC

/*******************************sef_definition for misc use****************************/
bool            bUsedAsRef;                                                     //for ref pic management
bool            bIsLongRef;     // long term reference frame flag       //for ref pic management
uint8_t         uiRefCount;
bool            bAvailableFlag; // indicate whether it is available in this picture memory block.

bool            bIsComplete;    // indicate whether current picture is complete, not from EC
/*******************************for future use****************************/
uint8_t         uiTemporalId;
uint8_t         uiSpatialId;
uint8_t         uiQualityId;

int32_t         iFrameNum;              // frame number                 //for ref pic management
int32_t         iLongTermFrameIdx;                                      //id for long term ref pic

int32_t     iSpsId; //against mosaic caused by cross-IDR interval reference.
int32_t     iPpsId;
unsigned long long uiTimeStamp;
bool bNewSeqBegin;
int32_t iMbEcedNum;
int32_t iMbEcedPropNum;
int32_t iMbNum;
} SPicture, *PPicture; // "Picture" declaration is comflict with Mac system

} // namespace WelsDec

#endif//WELS_PICTURE_H__
