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
 *  read_config.h
 *
 *  Abstract
 *      Class for reading parameter settings in a configure file.
 *
 *  History
 *      08/18/2008 Created
 *
 *****************************************************************************/
#if !(defined(WIN32) || defined(WIN64))
#include <string.h>
#include <stdio.h>
#endif



#include "read_config.h"

CReadConfig::CReadConfig (const char* kpConfigFileName)
  : m_pCfgFile (0)
  , m_strCfgFileName (kpConfigFileName)
  , m_ulLines (0) {
  if (strlen (kpConfigFileName) > 0) {	// FIXME: To check validation in configure file name
    m_pCfgFile = fopen (kpConfigFileName, "r");
  }
}

CReadConfig::~CReadConfig() {
  if (m_pCfgFile) {
    fclose (m_pCfgFile);
    m_pCfgFile = NULL;
  }
}

long CReadConfig::ReadLine (string* pStr, const int kiValSize/* = 4*/) {
  if (m_pCfgFile == NULL || pStr == NULL || kiValSize <= 1)
    return 0;

  string* strTags = &pStr[0];
  int iTagNum = 0, iNum = 0;
  bool bCommentFlag = false;

  while (iNum < kiValSize) {
    pStr[iNum]	= "";
    ++ iNum;
  }

  do {
    const char kChar = (char)fgetc (m_pCfgFile);

    if (kChar == '\n' || feof (m_pCfgFile)) {
      ++ m_ulLines;
      break;
    }
    if (kChar == '#')
      bCommentFlag = true;
    if (!bCommentFlag) {
      if (kChar == '\t' || kChar == ' ') {
        if (iTagNum >= kiValSize)
          break;
        if (! (*strTags).empty()) {
          ++ iTagNum;
          strTags	= &pStr[iTagNum];
        }
      } else
        *strTags += kChar;
    }

  } while (true);

  return 1 + iTagNum;
}

const bool CReadConfig::EndOfFile() {
  if (m_pCfgFile == NULL)
    return true;
  return feof (m_pCfgFile) ? true : false;
}

const int CReadConfig::GetLines() {
  return m_ulLines;
}

const bool CReadConfig::ExistFile() {
  return (m_pCfgFile != NULL);
}

const string& CReadConfig::GetFileName() {
  return m_strCfgFileName;
}
