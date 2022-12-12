#ifndef FT12VARIABLE_H
#define FT12VARIABLE_H
#pragma once

#include "IFT12.h"

typedef class FT12Variable_ : public IFT12 {
 public:
  FT12Variable_() : IFT12() {}

  FT12Variable_(const unsigned char Ctrl, const unsigned char Addr) : IFT12(Ctrl, Addr) {}

  FT12Variable_(const void *pData, const size_t Size) {
    char *p = const_cast<char *>(static_cast<const char *>(pData));

    if (p[0] != Start || p[3] != Start || p[Size - 1] != End) {
      TRACEENDL("Invalid start or end byte. This frame is not valid.");
      return;
    }

    if (p[1] != p[2]) {
      TRACEENDL("Lenght field is not equal across frame. This is invalid!");
      return;
    }

    Lenght = p[1];
    Control = p[4];
    Address = p[5];
    CheckSum = p[Size - 2];

    UserData = p + 6;  // Skipping address and control since they are separate fields.
    UserDataSize = Size - Header_Size;

    if (ComputeChecksum() != CheckSum) TRACEENDL("Current checksum is not valid. This frame should be discarderd");
  }

  virtual size_t FrameSize() const { return Header_Size + this->UserDataSize; }

  virtual void *CreateRawBuffer(void *pData, size_t *pSize) const {
    if (pSize != 0) *pSize = this->FrameSize();

    if (pData != NULL) {
      unsigned char *p = static_cast<unsigned char *>(pData);
      p[0] = Start;
      p[1] = p[2] = 2 + static_cast<unsigned char>(UserDataSize);
      p[3] = Start;
      p[4] = Control;
      p[5] = Address;

      p += 6;

      if (UserDataSize > 0) {
        memcpy(p, this->UserData, this->UserDataSize);
        p += UserDataSize;
      }

      p[0] = ComputeChecksum();
      p[1] = End;

      return pData;
    }

    return 0;
  }

 protected:
  static const unsigned char Start = 0x68; /*Fixed field from 870-5-1*/
  static const unsigned char Header_Size = 8;
  unsigned char Lenght;

} FT12Variable, *LPFT12Variable; /* FT1.2 with Variable size from IEC-870-5-1	*/

#endif
