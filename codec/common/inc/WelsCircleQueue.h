/*!
 * \copy
 *     Copyright (c)  2009-2015, Cisco Systems
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
 * \file    WelsCircleQueue.h
 *
 * \brief   for the queue function needed in ThreadPool
 *
 * \date    9/27/2015 Created
 *
 *************************************************************************************
 */


#ifndef _WELS_CIRCLE_QUEUE_H_
#define _WELS_CIRCLE_QUEUE_H_

#include "typedefs.h"
#include <stdlib.h>

namespace WelsCommon {

template<typename TNodeType>
class CWelsCircleQueue {
 public:
  CWelsCircleQueue() {
    m_iMaxNodeCount = 50;
    m_pCurrentQueue = NULL;
    //here using array to simulate list is to avoid the frequent malloc/free of Nodes which may cause fragmented memory
    m_iCurrentListStart = m_iCurrentListEnd = 0;
  };
  ~CWelsCircleQueue() {
	  if (m_pCurrentQueue)
	    free (m_pCurrentQueue);
  };

  int32_t size() {
    return ((m_iCurrentListEnd >= m_iCurrentListStart)
            ? (m_iCurrentListEnd - m_iCurrentListStart)
            : (m_iMaxNodeCount - m_iCurrentListStart + m_iCurrentListEnd));
  }

  int32_t push_back (TNodeType* pNode) {
	if (NULL == m_pCurrentQueue)
	{
		m_pCurrentQueue = static_cast<TNodeType**> (malloc (m_iMaxNodeCount * sizeof (TNodeType*)));
		if (NULL == m_pCurrentQueue)
			return 1;	
	}
	
    if ((NULL != pNode) && (find (pNode))) {      //not checking NULL for easier testing
      return 1;
    }
    return InternalPushBack (pNode);
  }

  bool find (TNodeType* pNode) {
    if (size() > 0) {
      if (m_iCurrentListEnd > m_iCurrentListStart) {
        for (int32_t idx = m_iCurrentListStart; idx < m_iCurrentListEnd; idx++) {
          if (pNode == m_pCurrentQueue[idx]) {
            return true;
          }
        }
      } else {
        for (int32_t idx = m_iCurrentListStart; idx < m_iMaxNodeCount; idx++) {
          if (pNode == m_pCurrentQueue[idx]) {
            return true;
          }
        }
        for (int32_t idx = 0; idx < m_iCurrentListEnd; idx++) {
          if (pNode == m_pCurrentQueue[idx]) {
            return true;
          }
        }
      }
    }
    return false;
  }

  void pop_front() {
    if (size() > 0) {
      m_pCurrentQueue[m_iCurrentListStart] = NULL;
      m_iCurrentListStart = ((m_iCurrentListStart < (m_iMaxNodeCount - 1))
                             ? (m_iCurrentListStart + 1)
                             : 0);
    }
  }

  TNodeType* begin() {
    if (size() > 0) {
      return m_pCurrentQueue[m_iCurrentListStart];
    }
    return NULL;
  }

  TNodeType* GetIndexNode (const int32_t iIdx) {
    if (size() > 0) {
      if ((iIdx + m_iCurrentListStart) < m_iMaxNodeCount) {
        return m_pCurrentQueue[m_iCurrentListStart + iIdx];
      } else {
        return m_pCurrentQueue[m_iCurrentListStart + iIdx - m_iMaxNodeCount];
      }
    }
    return NULL;
  }

 private:
  int32_t InternalPushBack (TNodeType* pNode) {
    m_pCurrentQueue[m_iCurrentListEnd] = pNode;
    m_iCurrentListEnd ++;

    if (m_iCurrentListEnd == m_iMaxNodeCount) {
      m_iCurrentListEnd = 0;
    }
    if (m_iCurrentListEnd == m_iCurrentListStart) {
      int32_t ret = ExpandQueue();
      if (ret) {
        return 1;
      }
    }
    return 0;
  }

  int32_t ExpandQueue() {
    TNodeType** tmpCurrentTaskQueue = static_cast<TNodeType**> (malloc (m_iMaxNodeCount * 2 * sizeof (TNodeType*)));
    if (tmpCurrentTaskQueue == NULL) {
      return 1;
    }

    memcpy (tmpCurrentTaskQueue,
            (m_pCurrentQueue + m_iCurrentListStart),
            (m_iMaxNodeCount - m_iCurrentListStart)*sizeof (TNodeType*));
    if (m_iCurrentListEnd > 0) {
      memcpy (tmpCurrentTaskQueue + m_iMaxNodeCount - m_iCurrentListStart,
              m_pCurrentQueue,
              m_iCurrentListEnd * sizeof (TNodeType*));
    }

    free (m_pCurrentQueue);

    m_pCurrentQueue = tmpCurrentTaskQueue;
    m_iCurrentListEnd = m_iMaxNodeCount;
    m_iCurrentListStart = 0;
    m_iMaxNodeCount = m_iMaxNodeCount * 2;

    return 0;
  }
  int32_t m_iCurrentListStart;
  int32_t m_iCurrentListEnd;
  int32_t m_iMaxNodeCount;
  TNodeType** m_pCurrentQueue;
};

}


#endif



