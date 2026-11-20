/*!
 * \copy
 *     Copyright (c)  2008-2013, Cisco Systems
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
 *
 *  memmgr_nal_unit.c
 *
 *  Abstract
 *      memory manager utils for NAL Unit list available
 *
 *  History
 *      07/10/2008 Created
 *
 *****************************************************************************/
#include "memmgr_nal_unit.h"
#include "memory_align.h"
#include "error_code.h"

namespace WelsDec {

int32_t MemInitNalList (PAccessUnit* ppAu, const uint32_t kuiSize, CMemoryAlign* pMa) {
  uint32_t uiIdx = 0;
  uint8_t* pBase = NULL, *pPtr = NULL;
  const uint32_t kuiSizeAu = sizeof (SAccessUnit);
  const uint32_t kuiSizeNalUnitPtr = kuiSize * sizeof (PNalUnit);
  const uint32_t kuiSizeNalUnit = sizeof (SNalUnit);
  const uint32_t kuiCountSize = (kuiSizeAu + kuiSizeNalUnitPtr + kuiSize * kuiSizeNalUnit) * sizeof (uint8_t);

  if (kuiSize == 0)
    return ERR_INFO_INVALID_PARAM;

  if (*ppAu != NULL) {
    MemFreeNalList (ppAu, pMa);
  }

  pBase = (uint8_t*)pMa->WelsMallocz (kuiCountSize, "Access Unit");
  if (pBase == NULL)
    return ERR_INFO_OUT_OF_MEMORY;
  pPtr = pBase;
  *ppAu = (PAccessUnit)pPtr;
  pPtr += kuiSizeAu;
  (*ppAu)->pNalUnitsList = (PNalUnit*)pPtr;
  pPtr += kuiSizeNalUnitPtr;
  do {
    (*ppAu)->pNalUnitsList[uiIdx] = (PNalUnit)pPtr;
    pPtr += kuiSizeNalUnit;
    ++ uiIdx;
  } while (uiIdx < kuiSize);

  (*ppAu)->uiCountUnitsNum      = kuiSize;
  (*ppAu)->uiAvailUnitsNum      = 0;
  (*ppAu)->uiActualUnitsNum     = 0;
  (*ppAu)->uiStartPos           = 0;
  (*ppAu)->uiEndPos             = 0;
  (*ppAu)->bCompletedAuFlag     = false;
  (*ppAu)->pExpandedBlock       = NULL;

  return ERR_NONE;
}

int32_t MemFreeNalList (PAccessUnit* ppAu, CMemoryAlign* pMa) {
  if (ppAu != NULL) {
    PAccessUnit pAu = *ppAu;
    if (pAu != NULL) {
      uint8_t* pBlock = (uint8_t*)pAu->pExpandedBlock;
      while (pBlock != NULL) { // blocks added by ExpandNalUnitList, latest first
        uint8_t* pPrevBlock = * ((uint8_t**)pBlock);
        pMa->WelsFree (pBlock, "Access Unit expanded");
        pBlock = pPrevBlock;
      }
      pMa->WelsFree (pAu, "Access Unit");
      *ppAu = NULL;
    }
  }
  return ERR_NONE;
}


/*
 *  ExpandNalUnitList
 *  Grow the list of NAL Unit pointers to kiExpSize and add new NAL Units for the new entries.
 *  NAL Units already in the list stay where they are until MemFreeNalList: decoding keeps pointers
 *  into them (pCtx->pNalCur, pCtx->pSliceHeader, the current dq layer's ref marking and reordering).
 */
int32_t ExpandNalUnitList (PAccessUnit* ppAu, const int32_t kiOrgSize, const int32_t kiExpSize, CMemoryAlign* pMa) {
  if (kiExpSize <= kiOrgSize)
    return ERR_INFO_INVALID_PARAM;
  else {
    PAccessUnit pAu = *ppAu;
    uint8_t* pBase = NULL, *pPtr = NULL;
    PNalUnit* pNalUnitsList = NULL;
    int32_t iIdx = 0;
    const uint32_t kuiSizeBlockLink = WELS_ALIGN (sizeof (uint8_t*), 16); // keeps the NAL Units after it aligned
    const uint32_t kuiSizeNalUnit = sizeof (SNalUnit);
    const uint32_t kuiSizeNalUnits = (kiExpSize - kiOrgSize) * kuiSizeNalUnit;
    const uint32_t kuiSizeNalUnitPtr = kiExpSize * sizeof (PNalUnit);
    const uint32_t kuiCountSize = kuiSizeBlockLink + kuiSizeNalUnits + kuiSizeNalUnitPtr;

    pBase = (uint8_t*)pMa->WelsMallocz (kuiCountSize, "Access Unit expanded");
    if (pBase == NULL)
      return ERR_INFO_OUT_OF_MEMORY;
    pPtr = pBase;
    * ((uint8_t**)pPtr) = (uint8_t*)pAu->pExpandedBlock; // chain to the block expanded before, freed in MemFreeNalList
    pPtr += kuiSizeBlockLink;
    pNalUnitsList = (PNalUnit*) (pPtr + kuiSizeNalUnits);

    for (iIdx = 0; iIdx < kiOrgSize; ++ iIdx) // keep the NAL Units in place, in their current order
      pNalUnitsList[iIdx] = pAu->pNalUnitsList[iIdx];
    for (; iIdx < kiExpSize; ++ iIdx) {
      pNalUnitsList[iIdx] = (PNalUnit)pPtr;
      pPtr += kuiSizeNalUnit;
    }

    pAu->pNalUnitsList          = pNalUnitsList;
    pAu->uiCountUnitsNum        = kiExpSize;
    pAu->pExpandedBlock         = pBase;
    return ERR_NONE;
  }
}

/*
 *  MemGetNextNal
 *  Get next NAL Unit for using.
 *  Need expand NAL Unit list if exceeding count number of available NAL Units withing an Access Unit
 */
PNalUnit MemGetNextNal (PAccessUnit* ppAu, CMemoryAlign* pMa) {
  PAccessUnit pAu = *ppAu;
  PNalUnit pNu = NULL;

  if (pAu->uiAvailUnitsNum >= pAu->uiCountUnitsNum) { // need expand list
    const uint32_t kuiExpandingSize = pAu->uiCountUnitsNum + (MAX_NAL_UNIT_NUM_IN_AU >> 1);
    if (ExpandNalUnitList (ppAu, pAu->uiCountUnitsNum, kuiExpandingSize, pMa))
      return NULL; // out of memory
    pAu = *ppAu;
  }

  pNu = pAu->pNalUnitsList[pAu->uiAvailUnitsNum++]; // ready for next nal position

  memset (pNu, 0, sizeof (SNalUnit)); // Please do not remove this for cache intend!!

  return pNu;
}

} // namespace WelsDec
