#ifndef FT12FIXED_H
#define FT12FIXED_H
#pragma once

#include "IFT12.h"

typedef class FT12Fixed_ : public IFT12 {
 public:
  FT12Fixed_() : IFT12() {}

  FT12Fixed_(const unsigned char Ctrl, const unsigned char Addr) : IFT12(Ctrl, Addr) {}

  FT12Fixed_(const void *pData, const size_t Size) : IFT12(pData, Size) {}

  virtual size_t FrameSize() const { return Header_Size + this->UserDataSize; }

  virtual void *CreateRawBuffer(void *pData, size_t *pSize) const {
    if (pSize != 0) *pSize = this->FrameSize();

    if (pData != NULL) {
      unsigned char *p = static_cast<unsigned char *>(pData);
      p[0] = Start;
      p[1] = Control;
      p[2] = Address;

      p += (3 + UserDataSize);

      if (UserDataSize > 0)
        memcpy(p, this->UserData, this->UserDataSize);
      else

        p[0] = ComputeChecksum();
      p[1] = End;

      return pData;
    }

    return 0;
  }

 protected:
  static const unsigned char Header_Size = 5;

} FT12Fixed; /* FT1.2 with fixed size from IEC-870-5-1	 */

#endif
