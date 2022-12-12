#ifndef IFT12_H
#define IFT12_H
#pragma once

#ifndef DBG_NEW
#define DBG_NEW new
#endif
#ifndef PLACEMENT_NEW
#define PLACEMENT_NEW new
#endif

#define TRACEENDL(x) OutputDebugStringA(x)

typedef class IFT12_ {
 public:
  IFT12_() : UserData(0), UserDataSize(0) {}
  /*Creates a dataunit. */
  IFT12_(const unsigned char Ctrl, const unsigned char Addr)
      : UserData(0), UserDataSize(0), Control(Ctrl), Address(Addr) {}
  /*Creates a FT12 struct from raw data*/
  IFT12_(const void *pData, const size_t Size) : UserData(0), UserDataSize(0) {
    char *p = const_cast<char *>(static_cast<const char *>(pData)); /*Transform to char, then remove const attribute*/

    if (p[0] != Start || p[Size - 1] != End) {
      TRACEENDL("Invalid start or end byte. This frame is not valid.");
      return;
    }

    Control = p[1];
    Address = p[2];
    CheckSum = p[Size - 2];

    if (Size > 5) {
      UserData = p + 3;
      UserDataSize = Size - 5;
    }

    if (ComputeChecksum() != CheckSum) TRACEENDL("Current checksum is not valid. This frame should be discarderd");
  }

  /*Computes the current frame size*/
  virtual size_t FrameSize() const = 0;

  /*Creates a Control Byte for FT12*/
  inline static unsigned char CreateControlByte(const unsigned char PRM, const unsigned char FCB,
                                                const unsigned char FCV, const unsigned char F1, const unsigned char F2,
                                                const unsigned char F3, const unsigned char F4) {
    unsigned char ret = 0;

    if (PRM != 0) ret |= 0x40;
    if (FCB != 0) ret |= 0x20;
    if (FCV != 0) ret |= 0x10;
    if (F1 != 0) ret |= 0x8;
    if (F2 != 0) ret |= 0x4;
    if (F3 != 0) ret |= 0x2;
    if (F4 != 0) ret |= 0x1;
    return ret;
  }

  /*Sets user data pointer for current frame*/
  void SetUserData(const void *pData, const size_t Size) {
    UserData = const_cast<void *>(pData);
    UserDataSize = Size;
  }

  /*Gets user data pointer for current frame*/
  void GetUserData(const void **pData, size_t *pSize) const {
    *pData = this->UserData;
    *pSize = this->UserDataSize;
  }

  /*Creates a raw buffer that can be send from a Communication Port*/
  virtual void *CreateRawBuffer(void *pData, size_t *pSize) const = 0;

  static const unsigned char Start = 0x10; /*Fixed field from 870-5-1*/
  unsigned char Control;
  unsigned char Address;
  unsigned char CheckSum;
  static const unsigned char End = 0x16; /*Fixed field from 870-5-1	 */

 protected:
  void *UserData;
  size_t UserDataSize;

  unsigned char ComputeChecksum() const {
    unsigned char result = 0;
    result += Control;
    result += Address;

    unsigned char *p = static_cast<unsigned char *>(UserData);

    for (size_t i = 0; i < UserDataSize; i++) result += *(p++);

    return result;
  }

} IFT12, *LPIFT12; /*FT1.2 with fixed size from IEC-870-5-1*/

#endif
