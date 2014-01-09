#ifndef __BUFFEREDDATA_H__
#define __BUFFEREDDATA_H__

#include <stddef.h>
#include <stdlib.h>

class BufferedData {
 public:
  BufferedData() : data_(NULL), capacity_(0), length_(0) {}

  ~BufferedData() {
    free(data_);
  }

  bool Push(uint8_t c) {
    if (!EnsureCapacity(length_ + 1)) {
      return false;
    }
    data_[length_++] = c;
    return true;
  }

  void Clear() {
    length_ = 0;
  }

  void SetLength(size_t newLen) {
    if (EnsureCapacity(newLen)) {
      length_ = newLen;
    }
  }

  size_t Length() const {
    return length_;
  }

  uint8_t* data() {
    return data_;
  }

 private:
  bool EnsureCapacity(size_t capacity) {
    if (capacity > capacity_) {
      size_t newsize = capacity * 2;

      uint8_t* data = static_cast<uint8_t*>(realloc(data_, newsize));

      if (!data)
        return false;

      data_ = data;
      capacity_ = newsize;
      return true;
    }
    return true;
  }

  uint8_t* data_;
  size_t capacity_;
  size_t length_;
};

#endif //__BUFFEREDDATA_H__
