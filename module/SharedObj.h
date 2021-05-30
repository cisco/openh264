/*
 * Copyright 2018, Mozilla Foundation and contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SharedObj_h__
#define __SharedObj_h__

#include "RefCounted.h"

template <class T>
class SharedObj : public RefCounted {
 public:
  SharedObj(): mObj() {
    memset (&mObj, 0, sizeof (T));
  }

  virtual ~SharedObj() {
  }

  T* get() {
    return &mObj;
  }

 protected:
  T mObj;
};

template <class T>
SharedObj<T>* MakeShared() {
  SharedObj<T>* obj = new SharedObj<T>();
  return obj;
}

#endif  // __SharedObj_h__
