#ifndef IEC8705103Manager_H
#define IEC8705103Manager_H
#pragma once

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>

#include "IEC87052Manager.h"
#include "gettimeofday.h"

#define TRACEENDL(x) OutputDebugStringA(x)
/*
Some notes on 103 that may be useful to know during implementation:

1)	103 admits exclusively frame format FT1.2 that is defined in 6.2.4.2 of IEC 60870-5-1.
Formats with fixed and with variable block lengths are admitted. Also the single control
character E5H transmission is admitted.

2)   The frame with fixed length has no link user data. It is referred to below as a short message.
The	single character A2H is not used.

3)	The control system constitutes the master, the protection equipment the slave; i.e. the control system
is always primary station, the protection equipment always secondary station. The RES bit is not used.
The following function codes are used:
PRM = 1 -> 0, 3, 4, 9, 10, 11
PRM = 0 -> 0, 1, 8, 9, 11
Address field A always consists of one octet only. For broadcast (send/no reply) the address is defined	as 255.

4)	A LINK PROTOCOL DATA UNIT (LPDU) of this companion standard contains not more than one APPLICATION SERVICE DATA UNIT
(ASDU).

5)	The COMMON ADDRESS OF ASDU shall normally be identical to the address used in the link layer.

6)	The following basic application functions, defined in IEC 60870-5-5, are used:
� Station initialization
� General interrogation
� Clock synchronization
� Command transmission
Additionally, four application functions are defined within this companion standard:
� Test mode
� Blocking of monitor direction
� Transmission of disturbance data
� Generic services

*/

#define MAX_DIST_COUNT 255
#define MAX_SDV_COUNT 5000

/* IEC 870-5-103 protocol manager. */
class IEC8705103Manager {
 private:
  typedef enum FunctionType_ {
    None = 0,
    DistanceProtection = 128,
    OvercurrentProtection = 160,
    TrasformerDifferentialProtection = 176,
    LineDifferentialProtection = 192,
    GenericFunctionType = 254,
    GlobalFunctionType = 255
  } FunctionType;

  typedef union DUI_ {
    DUI_() : dui_(0) {}
    DUI_(unsigned int dui) : dui_(dui) {}
    DUI_(unsigned char Ti, unsigned char Vsi, unsigned char Cot, unsigned char Ca) {
      SepDui.TypeIdentification = Ti;
      SepDui.VariableStructureIdentifier = Vsi;
      SepDui.CauseOfTrasmission = Cot;
      SepDui.CommonAddress = Ca;
    }

    struct {
      unsigned char TypeIdentification;          /* 1-32 standard - 31-255 private range */
      unsigned char VariableStructureIdentifier; /* 0-9 ok, 10-127 not used. Last bit is sequence identifier. */
      unsigned char CauseOfTrasmission;          /* 0 not used, 1-63 compatible range, 64-255 private range */
      unsigned char CommonAddress;               /* 0-254 local, 255 global. */
    } SepDui;

    unsigned int dui_; /* Can be useful have all header in a single value */

  } DUI; /*DataUnitIdentifier*/
  typedef union IFI_ {
    IFI_() : ifi_(0) {}
    IFI_(unsigned short ifi) : ifi_(ifi) {}
    IFI_(FunctionType Ft, unsigned char In) {
      SepIfi.FunctionType = static_cast<unsigned char>(Ft);
      SepIfi.InformationNumber = In;
    }

    struct {
      unsigned char FunctionType;      /* Various ranges, see documentation. */
      unsigned char InformationNumber; /* The full range <0..255> is used independently in the control direction as well
                                          as in the monitor direction. */

    } SepIfi;

    unsigned short int ifi_; /* Can be useful to have all header in a single value */

  } IFI;  // Information Object Identifier;

 public:
  typedef struct cp56Time2A_ {
    friend class IEC8705103Manager;
    friend std::ofstream &operator<<(std::ofstream &out, const cp56Time2A_ &object) {
      /*dd/mm/yyyy,hh:mm:ss.ssssss*/
      unsigned short secs;
      unsigned short millisecs;

      secs = object.Milliseconds / 1000;
      millisecs = object.Milliseconds % 1000;

      out << object.GetDayMonth() << "/" << object.GetMonth() << "/" << object.GetYear() << "," << object.GetHours()
          << ":" << object.GetMinutes() << ":" << secs << "." << millisecs;
      return out;
    }

    /*Simple emtpy object*/
    cp56Time2A_() {}

    /*Constructs a date object from raw data of protection equipment*/
    cp56Time2A_(const unsigned char *pDate) {
      memcpy(&this->Milliseconds, pDate, sizeof(unsigned short));
      // unsigned short *tmpVal =  &(this->Milliseconds);
      // tmpVal[0] = pDate[0];
      // tmpVal[1] = pDate[1];
      this->Minutes = pDate[2];
      this->Hours = pDate[3];
      this->Day = pDate[4];
      this->Month = pDate[5];
      this->Year = pDate[6];
    }

    /*Constructs a date object from single values*/
    cp56Time2A_(unsigned short milliseconds, unsigned char minutes, unsigned char hours, unsigned char dayweek,
                unsigned char daymonth, unsigned char month, unsigned char years, unsigned char SU)
        : Minutes(0), Hours(0), Day(0), Month(0), Year(0) {
      unsigned short *tmpVal = &(this->Milliseconds);
      unsigned char *pDate = (unsigned char *)&milliseconds;
      tmpVal[0] = pDate[1];
      tmpVal[1] = pDate[0];

      Minutes = (minutes & 0x3F);
      Hours = (hours & 0x1F);

      if (SU == 0) Hours |= 0x80;

      Day = ((daymonth & 0x1F));
      if (dayweek & 1) Day |= 0x20;
      if ((dayweek >> 1) & 1) Day |= 0x40;
      if ((dayweek >> 2) & 1) Day |= 0x80;
      Month = (month & 0xF);
      Year = ((years - 100) & 0x7F);
    }

    inline void AddMilliseconds(unsigned short add) { this->Milliseconds += add; }

    inline unsigned short GetMilliseconds() const { return this->Milliseconds; }
    inline unsigned short GetMinutes() const { return this->Minutes & 0x3F; }
    inline unsigned short GetHours() const { return this->Hours & 0x1F; }
    inline unsigned short GetSummerTime() const { return this->Hours & 0x80; }
    inline unsigned short GetMonth() const { return this->Month & 0xF; }
    inline unsigned short GetYear() const { return 2000 + (this->Year & 0x7F); }
    inline unsigned short GetDayWeek() const { return this->Day & 0xE0; }
    inline unsigned short GetDayMonth() const { return this->Day & 0x1F; }

   private:
    unsigned short Milliseconds;

    // IV(1), RV(1), Minutes(6)
    unsigned char Minutes;

    // SU(1); RV(2), Hours(5)
    unsigned char Hours;

    // DayWeek(3),DayMonth(5)
    unsigned char Day;

    // RV(3),Month(5)
    unsigned char Month;

    // RV(1),Year(7)
    unsigned char Year;

  } cp56Time2A; /*Seven (and four) octet binary time*/

  typedef struct ASDUHeader_ {
    ASDUHeader_() {}
    ASDUHeader_(DUI d, IFI i) : DataUnitIdentifier(d), InformationObjectIdentifier(i) {}

    DUI DataUnitIdentifier;
    IFI InformationObjectIdentifier;

    inline static unsigned char extractBitRange(char byte, int startingPos, int offset) {
      return (byte >> startingPos) & ((1 << offset) - 1);
    }

  } ASDUHeader;  // AdsuHeader. Cannot codify body too. It's variable.

  typedef struct Energy_ {
    unsigned int value;
    unsigned char IFI;
  } Energy;  // Meter value from ASDU 205

  enum Command {
    AutoRecloser_On_Off = 16,
    Teleprotection_On_Off = 17,
    Protection_On_Off = 18,
    LedReset = 19,
    Activate_Char_1 = 23,
    Activate_Char_2 = 24,
    Activate_Char_3 = 25,
    Activate_Char_4 = 26
  };

  struct ASDU29 {
    unsigned short FAN;
    unsigned char NOT;
    unsigned short TAP;
  };
  struct TAG {
    unsigned char FType;
    unsigned char In;
    unsigned char DIP;
  };
  struct ASDU30 {
    unsigned char TOV;
    unsigned char FAN[2];
    unsigned char ACC;
    unsigned char NDV;
    unsigned short NFE;
  };

  typedef struct Disturbance_ {
    struct {
      struct {
        unsigned short NOT;
        unsigned short TAP;
        TAG TagsValue[MAX_SDV_COUNT];
      } TagsHeader[MAX_DIST_COUNT];  // Number of Tags

      unsigned short TagsCount;
    } TagsList;
    struct {
      struct {
        ASDU30 Header;
        int SDV[MAX_SDV_COUNT];  // Sign required!f
        float RPV;
        float RSV;
        float RFA;
      } Channels[MAX_DIST_COUNT];

      unsigned short Count;            // Number of channels
      unsigned short ChannelElements;  // Channel elements
    } ChannelList;

    unsigned short SamplingTime;  // Sampling time is in Microseconds
    unsigned char FaultNumber;
    cp56Time2A startTime;  // Time of first recording;
    cp56Time2A EventTime;  // EventTime

  } Disturbance, *LPDISTURBANCE;
  typedef struct COMTRADEChannel_ {
    COMTRADEChannel_(std::string ch_id, std::string ph, std::string ccbm) : ch_id(ch_id), ph(ph), ccbm(ccbm) {}
    std::string ch_id;
    std::string ph;
    std::string ccbm;
  } COMTRADEChannel;

  typedef struct AnalogChannel_ : public COMTRADEChannel {
    AnalogChannel_(std::string ch_id, std::string ph, std::string ccbm, std::string uu, unsigned int channelCode)
        : uu(uu), channelCode(channelCode), COMTRADEChannel(ch_id, ph, ccbm) {}

    AnalogChannel_() : channelCode(0), uu(""), COMTRADEChannel("", "", "") {}

    std::string uu;
    unsigned int channelCode;

  } AnalogChannel;

  typedef struct DigitalChannel_ : public COMTRADEChannel {
    DigitalChannel_() : COMTRADEChannel("", "", "") {}
    DigitalChannel_(std::string ch_id, std::string ph, std::string ccbm, std::string y, unsigned char FType,
                    unsigned char Ifi)
        : Ifi(Ifi), FType(FType), y(y), currVal(0), COMTRADEChannel(ch_id, ph, ccbm) {}
    std::string y;

    int currVal;

    unsigned char FType;
    unsigned char Ifi;
  } DigitalChannel;

  /*better ctor than the void one from ProtocolManager super class.*/
  IEC8705103Manager() {}
  IEC8705103Manager(CommunicationPort *p, const unsigned char address)
      : linklayermanager(DBG_NEW IEC87052Manager(p, address)), _address(address), fType(None) {
    memset(&this->DCurrent, 0, sizeof(Disturbance));
  }

  void SetAddress(unsigned char NewAddress) {
    this->_address = NewAddress;
    this->linklayermanager->SetAddress(NewAddress);
  }

  void SetFCB(unsigned char FCB) { linklayermanager->SetFCB(FCB); }

  /*Waits on the port for next message ADUS using Class request*/
  bool GetNextADSU(const void **pAdsu, size_t *Size, unsigned char Class) {
    bool result = linklayermanager->UserDataClass(Class);
    if (result == false) return false;

    this->linklayermanager->VLastReceivedFrame->GetUserData(pAdsu, Size);
    return true;
  }

  /*Sends a custom message on current link layer. Answers will be delivered using GetNextASDU() function*/
  bool CustomMessage(const void *pData, const size_t Size, const bool Confirm) {
    return linklayermanager->UserData(pData, Size, Confirm);
  }

  ~IEC8705103Manager() {
    if (linklayermanager != 0) {
      delete linklayermanager;
      linklayermanager = 0;
    }
  }

  bool StationInit() {
    /*Sequence from the standard IEC: Reset, Status, UserData1, UserData1*/
    if (!this->linklayermanager->ResetRemoteLink()) return false;
    if (!this->linklayermanager->StatusLink()) return false;

    void *pData = 0;
    size_t size = 0;

    do {
      linklayermanager->VLastReceivedFrame->GetUserData(const_cast<const void **>(&pData), &size);
      this->linklayermanager->UserDataClass(1);
    } while (size == 0);

    memcpy((void *)&lastHeader, pData, ASDUHeaderSize);
    pData = static_cast<char *>(pData) + ASDUHeaderSize;

    // Since is a Init, i already know response type. It's a good thing to make response checking.

    if (lastHeader.DataUnitIdentifier.SepDui.TypeIdentification != 5) {
      if (lastHeader.DataUnitIdentifier.SepDui.CauseOfTrasmission >= 3 &&
          lastHeader.DataUnitIdentifier.SepDui.CauseOfTrasmission <= 5) {
        TRACEENDL("Communication reset");
        if (!this->linklayermanager->UserDataClass(1)) return false;
        return true;
      } else {
        TRACEENDL("This is not an identification message.");
        // TRACEENDL(Logger::ToString(lastHeader.DataUnitIdentifier.SepDui.TypeIdentification));
        return false;
      }
    }

    if (lastHeader.DataUnitIdentifier.SepDui.CommonAddress != linklayermanager->address) {
      TRACEENDL("Common address is not the right one. This frame should be discarded.");
      return false;
    }

    if (lastHeader.DataUnitIdentifier.SepDui.CauseOfTrasmission != 4 &&
        lastHeader.DataUnitIdentifier.SepDui.CauseOfTrasmission != 3) {
      // TRACEENDL("Cause of trasmission is not due to a communication link reset. This frame should be discarded.");
      // return false;
    }

    if (lastHeader.InformationObjectIdentifier.SepIfi.InformationNumber != 3) {
      // TRACEENDL("Cause of trasmission is not due to a communication link reset. This frame should be discarded.");
      // return false;
    }

    // unsigned char Sq =
    // ADSUHeader::extractBitRange(lastHeader.DataUnitIdentifier.SepDui.VariableStructureIdentifier,0,7);
    unsigned char numInfo =
        ASDUHeader::extractBitRange(lastHeader.DataUnitIdentifier.SepDui.VariableStructureIdentifier, 7, 1);

    if (numInfo != 1) {
      TRACEENDL("Welcome message should be with only 1 information number. This frame is not a good one");
      return false;
    }

    this->fType = static_cast<FunctionType>(lastHeader.InformationObjectIdentifier.SepIfi.FunctionType);
    // TRACEENDL("Function type for this protection:" +
    // Logger::ToString(lastHeader.InformationObjectIdentifier.SepIfi.FunctionType))

    char *str = static_cast<char *>(pData);
    if (str[0] == 2) {
      TRACEENDL("This equipment has no support for generic services.");
    } else {
      TRACEENDL("This equipment has got support for generic services.");
    }

    str++;
    str[13] = 0;

    // TRACEENDL(std::string("Information element:") + str);

    if (!this->linklayermanager->UserDataClass(1)) return false;

    return true;
  }
  inline bool TimeSync(const time_t *time, timeval *tz) {
    tm t;
    localtime_s(&t, time);

    cp56Time2A t2a(static_cast<unsigned short>(t.tm_sec * 1000), static_cast<unsigned char>(t.tm_min),
                   static_cast<unsigned char>(t.tm_hour), static_cast<unsigned char>(t.tm_wday),
                   static_cast<unsigned char>(t.tm_mday), static_cast<unsigned char>(1 + t.tm_mon),
                   static_cast<unsigned char>(t.tm_year), static_cast<unsigned char>(t.tm_isdst));

    char buffer[ASDUHeaderSize + cp56TimeSize] = {0};

    *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(6, 129, 8, this->_address), IFI(GlobalFunctionType, 0));
    memcpy(buffer + ASDUHeaderSize, &t2a, cp56TimeSize);

    return linklayermanager->UserData(buffer, ASDUHeaderSize + cp56TimeSize, true);

    // After clock sync, shoud (do not know when) arrive an ADSU 6 message for OK of clock sync.
  }

  /*Use the scan number to check return ADSU values*/
  inline bool GeneralInterrogation(unsigned char ScanNumber) {
    unsigned char buffer[ASDUHeaderSize + 1];
    *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(7, 129, 9, this->_address), IFI(GlobalFunctionType, 0));
    buffer[ASDUHeaderSize] = ScanNumber;
    return linklayermanager->UserData(buffer, ASDUHeaderSize + 1);
  }
  /*
  Sends a command to the equipment.
  Command:
  <16> := auto-recloser on/off
  <17> := teleprotection on/off
  <18> := protection on/off
  <19> := LED reset
  <23> := activate characteristic 1
  <24> := activate characteristic 2
  <25> := activate characteristic 3
  <26> := activate characteristic 4
  DCO: 1 OFF, 2 ON
  RII: Numbert to check command execution in ADSU return messages.
  */
  inline bool CommandTrasmission(Command Command, unsigned char DCO, unsigned char RII, int FTYPE) {
    switch (this->fType) {
      case DistanceProtection:
        break;  // it supports any function.
      case OvercurrentProtection:
        if (Command >= 16 && Command <= 19) break;
      case LineDifferentialProtection:
        if (Command == 16 || Command == 18 || Command == 19) break;
      case TrasformerDifferentialProtection:
        if (Command == 18 || Command == 19) break;
      default:
        TRACEENDL("This protection does not support current function");
        return false;
        break;
    }

    unsigned char buffer[ASDUHeaderSize + 2] = {0};
    *(reinterpret_cast<ASDUHeader *>(buffer)) =
        ASDUHeader(DUI(20, 129, 20, this->_address), IFI((FunctionType)FTYPE, Command));
    buffer[ASDUHeaderSize] = DCO;
    buffer[ASDUHeaderSize + 1] = RII;
    /*
    TRACEENDL("B.Address:"+Logger::ToString(this->_address)+" AsduHeaderSize:"+Logger::ToString(ASDUHeaderSize));
    std::string buf;

    for (int i = 0; i < ASDUHeaderSize + 2; i++)
    {
    buf+= " " + Logger::ToString((int)buffer[i]);
    }

    TRACEENDL("B.Address:"+Logger::ToString(this->_address)+" Buffer content:" + buf)
    */
    return linklayermanager->UserData(buffer, ASDUHeaderSize + 2);
  }
  /*Enable/Disable Test mode for current equipment*/
  inline bool EnableTestMode() { return false; }  // Siprotec cannot handle this.
  /*Retrieves Disturbance data using current aviable value from a valid ASDU23 message.*/
  inline bool DisturbanceData(const void *pAsdu) {
    memcpy((void *)&lastHeader, pAsdu, ASDUHeaderSize);
    switch (lastHeader.DataUnitIdentifier.SepDui.TypeIdentification) {
      case 23:
        DisturbanceRequest(pAsdu);
        break;
      case 26:
        DisturbanceTransfer(pAsdu);
        break;
      case 27:
        DisturbanceChannel(pAsdu);
        break;
      case 28:
        DisturbanceTags(pAsdu);
        break;
      case 29:
        DisturbanceTagsGet(pAsdu);
        break;
      case 30:
        DisturbanceChannelGet(pAsdu);
        break;
      case 31:
        return DisturbanceEnd(pAsdu);
        break;
      default:
        return false;
        break;

        return false;
    }
  }
  /*Determines if current ASDU is due to a Disturbance Message*/
  inline bool IsDisturbanceMessage(const void *pAsdu) {
    memcpy((void *)&lastHeader, pAsdu, ASDUHeaderSize);
    return lastHeader.DataUnitIdentifier.SepDui.TypeIdentification >= 23 &&
           lastHeader.DataUnitIdentifier.SepDui.TypeIdentification <= 31;
  }
  /*Query protection for generic service and data/write functions*/
  inline bool GenericService() { return false; }  // Siprotec cannot handle this.
  /*Perform complete Station start.*/
  inline bool StationStart() {
    if (!this->StationInit()) return false;

    time_t tma;
    time(&tma);
    timeval tv;
    gettimeofday(&tv, 0);

    if (!this->TimeSync(&tma, &tv)) return false;

    if (!this->GeneralInterrogation(this->_address)) return false;

    return this->CommandTrasmission(IEC8705103Manager::LedReset, 2, 10, this->fType);
  }
  /*Loops on StationStart function checking return value. Will try and retry until station will be aviable.*/
  inline void BlockingStationStart() {
    while (this->StationStart() == false) {
      continue;
    }
  }

  /*Returns last disturbance data (if any)*/
  const Disturbance &GetDisturbanceData() const {
    /*It's safe to do.
    1) It's a reference so is not possible to delete it.
    */
    return this->DCurrent;
  }

  /*Saves disturbance values as a Comtrade file
  PARAMETERS:
  filename: Filename in which save. You will find a .cfg and a .dat file
  StationName: Name of the station. Choose the one you like
  StNum: Number of station. Choose the one you like
  data: Disturbance data retrieved using 103 functions.
  achannels: Analog channel description. 8, as 103 requires
  dchannels: Digital channels description. The one not contained here will be discarded
  DChannelCount: How many digital channels have we got
  linefreq: Line Frequency
  nsamples: number of samples

  */
  static bool SaveToComtrade(std::string filename, std::string StationName, unsigned short StNum,
                             const LPDISTURBANCE data, const AnalogChannel achannels[8], unsigned short AChannelCount,
                             DigitalChannel *dchannels, unsigned short DChannelCount, std::string linefreq,
                             std::string nsamples = "1") {
    std::ofstream file((filename + ".cfg").c_str(), std::ios::out);

    if (!file) {
      TRACEENDL("Unable to open file cfg");
      return false;
    }

    file << StationName << "," << StNum << ",1999" << std::endl;
    file << AChannelCount + DChannelCount << "," << AChannelCount << "A," << DChannelCount << "D" << std::endl;

    for (unsigned short i = 0; i < AChannelCount; i++) {
      short max = *std::max_element(
          data->ChannelList.Channels[achannels[i].channelCode].SDV,
          data->ChannelList.Channels[achannels[i].channelCode].SDV + data->ChannelList.ChannelElements);
      short min = *std::min_element(
          data->ChannelList.Channels[achannels[i].channelCode].SDV,
          data->ChannelList.Channels[achannels[i].channelCode].SDV + data->ChannelList.ChannelElements);

      file << i + 1 << "," << achannels[i].ch_id << "," << achannels[i].ph << "," << achannels[i].ccbm << ","
           << achannels[i].uu << "," << 1 / (32768) * data->ChannelList.Channels[achannels[i].channelCode].RFA << ","
           << 0 << "," << 0 << "," << min << "," << max << ","
           << data->ChannelList.Channels[achannels[i].channelCode].RPV << ","
           << data->ChannelList.Channels[achannels[i].channelCode].RSV << ",S" << std::endl;
    }

    for (unsigned short i = 0; i < DChannelCount; i++)
      file << i << "," << dchannels[i].ch_id << "," << dchannels[i].ph << "," << dchannels[i].ccbm << ","
           << dchannels[i].y << std::endl;

    file << linefreq << std::endl;  // Frequency.?
    file << "1" << std::endl;       // Sampling frequency is the same in all measures.
    file << 1 / (static_cast<float>(data->SamplingTime) / 1000000.f) << "," << data->ChannelList.ChannelElements
         << std::endl;
    file << data->startTime << std::endl;
    file << data->EventTime << std::endl;
    file << "ASCII" << std::endl << "1.0"; /*ASCII file format - multiplication factor for the time differential */

    file.close();

    file.open((filename + ".dat").c_str(), std::ios::out);

    if (!file) {
      TRACEENDL("Unable to open file cfg");
      return false;
    }

    for (int k = 0; k < DChannelCount; k++) {
      if (k != 0) file << ",";
      int theval = 0;

      for (int j = 0; j < data->TagsList.TagsCount; j++) {
        if (data->TagsList.TagsHeader[j].TAP == 0) {
          for (int w = 0; w < data->TagsList.TagsHeader[j].NOT; w++) {
            if (data->TagsList.TagsHeader[j].TagsValue[w].FType == dchannels[k].FType &&
                data->TagsList.TagsHeader[j].TagsValue[w].In == dchannels[k].Ifi) {
              dchannels[k].currVal = data->TagsList.TagsHeader[j].TagsValue[w].DIP - 1;
              // TRACEENDL(Logger::ToString(dchannels[k].FType)+"-"+ Logger::ToString(dchannels[k].Ifi) + std::string("
              // initial value:") + Logger::ToString(dchannels[k].currVal));
            }
          }
        }
      }
    }

    for (int i = 0; i < data->ChannelList.ChannelElements; i++) {
      file << i + 1 << "," << data->SamplingTime * i << ",";

      for (int x = 0; x < 255; x++) {
        int k = achannels[x].channelCode;

        if (k == 0) continue;

        if (x != 0) file << ",";

        short sdv = data->ChannelList.Channels[k].SDV[i];

        if ((data->ChannelList.Channels[k].SDV[i] & 0x8000) == 0x8000) {
          sdv = (-32768) + (data->ChannelList.Channels[k].SDV[i] & 0x7FFF);
        }

        file << sdv;
      }

      if (i != 0) {
        for (int j = 0; j < data->TagsList.TagsCount; j++) {
          if (data->TagsList.TagsHeader[j].TAP == i) {
            for (int k = 0; k < DChannelCount; k++) {
              for (int w = 0; w < data->TagsList.TagsHeader[j].NOT; w++) {
                if (data->TagsList.TagsHeader[j].TagsValue[w].FType == dchannels[k].FType &&
                    data->TagsList.TagsHeader[j].TagsValue[w].In == dchannels[k].Ifi) {
                  dchannels[k].currVal = data->TagsList.TagsHeader[j].TagsValue[w].DIP - 1;
                  // TRACEENDL(Logger::ToString(dchannels[k].FType)+"-"+ Logger::ToString(dchannels[k].Ifi) +
                  // std::string(" updated to:") + Logger::ToString(dchannels[k].currVal));
                }
              }
            }
          }
        }
      }

      for (int z = 0; z < DChannelCount; z++) {
        file << "," << dchannels[z].currVal;
      }

      file << std::endl;
    }

    file.close();

    return true;
  }
  static unsigned short GetDPI(const void *pAsdu) {
    SkipBytes(&pAsdu);
    unsigned short *ret;
    ret = (unsigned short *)(pAsdu);
    return *ret;
  }

  static void GetMeasurandsII(const void *pAsdu, unsigned short *measures) {
    ASDUHeader *h = (ASDUHeader *)pAsdu;
    SkipBytes(&pAsdu);
    unsigned short data[16] = {0};
    memcpy(data, pAsdu, 16 * sizeof(unsigned short));

    for (unsigned int i = 0; i < h->DataUnitIdentifier.SepDui.VariableStructureIdentifier; i++)
      measures[i] = data[i] >> 3;
  }

  static void GetEnergy(const void *pAsdu, Energy *pEnergy) {
    SkipBytes(&pAsdu, 5);
    memcpy(&pEnergy->IFI, pAsdu, 1);
    SkipBytes(&pAsdu, 1);
    memcpy(&pEnergy->value, pAsdu, 4);
  }

  static void GetTimeFromTaggedMessage(const void *pAsdu, cp56Time2A *time, unsigned short *Rel) {
    IEC8705103Manager::ASDUHeader *h = (IEC8705103Manager::ASDUHeader *)pAsdu;
    const void *pt = pAsdu;

    unsigned short skip = 0;
    unsigned short relPos = 0;

    SkipBytes(&pt);
    if (h->DataUnitIdentifier.SepDui.TypeIdentification == 1) {
      *Rel = 0;
      relPos = 0;
      skip = 1;
    }

    else if (h->DataUnitIdentifier.SepDui.TypeIdentification == 2) {
      relPos = 1;
      skip = 5;
    }

    else if (h->DataUnitIdentifier.SepDui.TypeIdentification == 4) {
      relPos = 4;
      skip = 8;
    }

    SkipBytes(&pt, skip);
    *time = cp56Time2A((const unsigned char *)pt);

    if (relPos != 0) {
      pt = pAsdu;
      SkipBytes(&pt, relPos + 6);
      memcpy(Rel, pt, 2);
    }
  }

 private:
  const static char cp56TimeSize = 7;
  const static char ASDU26Size = 10;
  const static char ASDU29Size = 5;
  const static char ASDU27Size = 16;
  const static char ASDU30Size = 7;
  const static char DUISize = 4;
  const static char IFISize = 2;

 public:
  const static char ASDUHeaderSize = DUISize + IFISize;

 private:
  IEC87052Manager *linklayermanager;
  const ASDUHeader lastHeader;
  FunctionType fType;
  Disturbance DCurrent;
  unsigned char _address;

  inline bool DisturbanceRequest(const void *pAsdu) {
    memcpy((void *)&lastHeader, pAsdu, ASDUHeaderSize);

    if (lastHeader.DataUnitIdentifier.SepDui.VariableStructureIdentifier == 0) {
      TRACEENDL("There is no disturbance data in this message");
      return false;
    }

    unsigned char VSQ = lastHeader.DataUnitIdentifier.SepDui.VariableStructureIdentifier;

    SkipBytes(&pAsdu);
    static unsigned char buffer[ASDUHeaderSize + 5];

    unsigned char FAN[2]; /* = static_cast<const unsigned short*>(pAsdu)[0];  */  // Fault number
    memcpy(&FAN, pAsdu, 2);
    SkipBytes(&pAsdu, 2);
    const unsigned char SOF = static_cast<const unsigned char *>(pAsdu)[0];  // Fault informations
    SkipBytes(&pAsdu, 1);
    this->DCurrent.EventTime = cp56Time2A(static_cast<const unsigned char *>(pAsdu));
    SkipBytes(&pAsdu, cp56TimeSize);

    // TRACEENDL("Fault record found. FAN: "+Logger::ToString((*(unsigned short *)&FAN))+"
    // SOF:"+Logger::ToString((int)SOF));

    // Ask for disturbance only if no another transfer is going on.
    if ((SOF & 0x2) != 0x2) {
      *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(24, 129, 31, this->_address), IFI(this->fType, 0));
      buffer[6] = 1;
      buffer[7] = 0;
      buffer[8] = FAN[0];
      buffer[9] = FAN[1];
      buffer[10] = 0;

      this->linklayermanager->UserData(buffer, ASDUHeaderSize + 5, true);

    } else
      TRACEENDL("Disturbance already in trasmission");
    return true;
  }
  inline bool DisturbanceTransfer(const void *pAsdu) {
    SkipBytes(&pAsdu, ASDUHeaderSize + 1);

    struct {
      unsigned char TOV;
      unsigned char FAN[2];
      unsigned short NOF;
      unsigned char NOC;
      unsigned short NOE;
      unsigned short INT;
    } A26;

    unsigned char bf[10];

    memcpy((void *)bf, pAsdu, ASDU26Size);

    // TRACEENDL("A26 message content:");

    A26.TOV = bf[0];
    A26.FAN[0] = bf[1];
    A26.FAN[1] = bf[2];
    A26.NOF = *((unsigned short *)(&bf[3]));
    A26.NOC = bf[5];
    A26.NOE = *((unsigned short *)(&bf[6]));
    A26.INT = *((unsigned short *)(&bf[8]));

    this->DCurrent.FaultNumber = (*(unsigned char *)&A26.FAN);
    /*
    TRACEENDL("TOV:"+Logger::ToString((int)A26.TOV));
    TRACEENDL("FAN:"+Logger::ToString((*(ushort_t*)&A26.FAN)));
    TRACEENDL("NOF:"+Logger::ToString((int)A26.NOF));
    TRACEENDL("NOC:"+Logger::ToString((int)A26.NOC));
    TRACEENDL("NOE:"+Logger::ToString((int)A26.NOE));
    TRACEENDL("INT:"+Logger::ToString((int)A26.INT));
    */
    SkipBytes(&pAsdu, ASDU26Size);

    this->DCurrent.startTime = cp56Time2A(static_cast<const unsigned char *>(pAsdu));
    this->DCurrent.startTime.Day = this->DCurrent.EventTime.Day;
    this->DCurrent.startTime.Month = this->DCurrent.EventTime.Month;
    this->DCurrent.startTime.Year = this->DCurrent.EventTime.Year;

    static unsigned char buffer[ASDUHeaderSize + 5];
    this->DCurrent.SamplingTime = A26.INT;
    this->DCurrent.ChannelList.Count = A26.NOC;
    this->DCurrent.ChannelList.ChannelElements = A26.NOE;

    *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(24, 129, 31, this->_address), IFI(this->fType, 0));

    buffer[6] = 2;
    buffer[7] = A26.TOV;
    buffer[8] = A26.FAN[0];
    buffer[9] = A26.FAN[1];
    buffer[10] = 0;

    return this->linklayermanager->UserData(buffer, ASDUHeaderSize + 5, true);
  }
  inline bool DisturbanceTags(const void *pAsdu) {
    SkipBytes(&pAsdu, 8);
    const unsigned char *FAN = reinterpret_cast<const unsigned char *>(pAsdu);

    static unsigned char buffer[ASDUHeaderSize + 5];
    *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(24, 129, 31, this->_address), IFI(this->fType, 0));

    buffer[6] = 16;  // Type of order - Request for tags
    buffer[7] = 1;
    buffer[8] = FAN[0];
    buffer[9] = FAN[1];
    buffer[10] = 0;

    return linklayermanager->UserData(buffer, ASDUHeaderSize + 5, true);
  }
  inline bool DisturbanceTagsGet(const void *pAsdu) {
    unsigned char *buffer = (unsigned char *)pAsdu;

    SkipBytes(&pAsdu);
    ASDU29 A29;
    A29.FAN = *((unsigned short *)pAsdu);
    SkipBytes(&pAsdu, 2);
    A29.NOT = *((unsigned char *)pAsdu);
    SkipBytes(&pAsdu, 1);
    unsigned char *t = (unsigned char *)pAsdu;
    A29.TAP = (((unsigned short)t[1]) << 8) | t[0];

    /*
    TRACEENDL("A29 CONTENTS:")
    TRACEENDL("FAN:"+Logger::ToString(A29.FAN));
    TRACEENDL("NOT:"+Logger::ToString(A29.NOT));
    TRACEENDL("TAP:"+Logger::ToString(A29.TAP));
    */
    SkipBytes(&pAsdu, 5);

    this->DCurrent.TagsList.TagsHeader[this->DCurrent.TagsList.TagsCount].NOT = A29.NOT;
    this->DCurrent.TagsList.TagsHeader[this->DCurrent.TagsList.TagsCount].TAP = A29.TAP;

    for (int i = 0; i < buffer[8]; i++) {
      /*
      TRACEENDL("Tag value");
      TRACEENDL("FT:" + Logger::ToString((int)buffer[11+(3*i)+0]));
      TRACEENDL("IFI:" + Logger::ToString((int)buffer[11+(3*i)+1]));
      TRACEENDL("DPI:" + Logger::ToString((int)buffer[11+(3*i)+2]));
      */

      this->DCurrent.TagsList.TagsHeader[this->DCurrent.TagsList.TagsCount].TagsValue[i].FType =
          buffer[11 + (3 * i) + 0];
      this->DCurrent.TagsList.TagsHeader[this->DCurrent.TagsList.TagsCount].TagsValue[i].In = buffer[11 + (3 * i) + 1];
      this->DCurrent.TagsList.TagsHeader[this->DCurrent.TagsList.TagsCount].TagsValue[i].DIP = buffer[11 + (3 * i) + 2];
    }

    this->DCurrent.TagsList.TagsCount++;

    return true;
  }
  inline bool DisturbanceChannel(const void *pAsdu) {
    struct {
      unsigned char TOV;
      unsigned char FAN[2];
      unsigned char ACC;
      float RPV;
      float RSV;
      float RFA;
    } A27;

    SkipBytes(&pAsdu, ASDUHeaderSize + 1);
    memcpy(static_cast<void *>(&A27), pAsdu, ASDU27Size);
    /*
    TRACEENDL("A27 CONTENT:");
    TRACEENDL("TOV:" + Logger::ToString((int)A27.TOV));
    TRACEENDL("FAN:" + Logger::ToString(((*(ushort_t*)&A27.FAN))));
    TRACEENDL("ACC:" + Logger::ToString((int)A27.ACC));
    TRACEENDL("RPV:" + Logger::ToString(A27.RPV));
    TRACEENDL("RSV:" + Logger::ToString(A27.RSV));
    TRACEENDL("RFA:" + Logger::ToString(A27.RFA));
    */
    // TRACEENDL("Trasmitting channel " + Logger::ToString((int)A27.ACC));

    static unsigned char buffer[ASDUHeaderSize + 5];
    *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(24, 129, 31, this->_address), IFI(this->fType, 0));

    buffer[6] = 8;  // Type of order - Request for channel
    buffer[7] = A27.TOV;
    buffer[8] = A27.FAN[0];
    buffer[9] = A27.FAN[1];
    buffer[10] = A27.ACC;

    this->DCurrent.ChannelList.Channels[A27.ACC].RFA = A27.RFA;
    this->DCurrent.ChannelList.Channels[A27.ACC].RSV = A27.RSV;
    this->DCurrent.ChannelList.Channels[A27.ACC].RPV = A27.RPV;

    return linklayermanager->UserData(buffer, sizeof(buffer), true);
  }
  inline bool DisturbanceChannelGet(const void *pAsdu) {
    SkipBytes(&pAsdu, 7);
    const ASDU30 *tmp;
    tmp = static_cast<const ASDU30 *>(pAsdu);
    /*
    TRACEENDL("ASDU30 CONTENT:");
    TRACEENDL("TOV:"+Logger::ToString((int)tmp->TOV));
    TRACEENDL("FAN:" + Logger::ToString(((*(ushort_t*)&tmp->FAN))));
    TRACEENDL("ACC:"+Logger::ToString((int)tmp->ACC));
    TRACEENDL("NDV:"+Logger::ToString((int)tmp->NDV));
    TRACEENDL("NFE:"+Logger::ToString((int)tmp->NFE));
    */
    unsigned short *tf = (unsigned short *)(&tmp->FAN);
    unsigned short *cf = (unsigned short *)(&this->DCurrent.ChannelList.Channels[tmp->ACC].Header.FAN);

    *cf = *tf;

    this->DCurrent.ChannelList.Channels[tmp->ACC].Header.ACC = tmp->ACC;
    this->DCurrent.ChannelList.Channels[tmp->ACC].Header.NDV = tmp->NDV;
    this->DCurrent.ChannelList.Channels[tmp->ACC].Header.NFE = tmp->NFE;
    this->DCurrent.ChannelList.Channels[tmp->ACC].Header.TOV = tmp->TOV;

    SkipBytes(&pAsdu, 7);

    for (int i = 0; i < tmp->NDV; i++) {
      unsigned int val = *(static_cast<const unsigned short *>(pAsdu));
      int sval = val;
      if ((val & 0x8000) == 0x8000) {
        sval = val - (2 * 32768);
      }

      this->DCurrent.ChannelList.Channels[tmp->ACC].SDV[i + this->DCurrent.ChannelList.Channels[tmp->ACC].Header.NFE] =
          sval;
      if (i + this->DCurrent.ChannelList.Channels[tmp->ACC].Header.NFE > MAX_SDV_COUNT) {
        TRACEENDL("Overflow with values!");
        break;
      }

      SkipBytes(&pAsdu, 2);
    }

    return true;
  }
  inline bool DisturbanceEnd(const void *pAsdu) {
    bool ret = false;
    struct {
      unsigned char TOO;
      unsigned char TOV;
      unsigned char FAN[2];
      unsigned char ACC;
    } A31;

    SkipBytes(&pAsdu);
    memcpy(static_cast<void *>(&A31), pAsdu, 5);

    unsigned char buffer[ASDUHeaderSize + 5];
    *(reinterpret_cast<ASDUHeader *>(buffer)) = ASDUHeader(DUI(25, 129, 31, this->_address), IFI(this->fType, 0));

    unsigned char respcode;

    switch (A31.TOO) {
      case 32:
        respcode = 64;
        break;
      case 34:
        TRACEENDL("Disturbance data aborted by protection");
        respcode = 65;
        break;
      case 35:
        respcode = 66;
        break;
      case 37:
        TRACEENDL("Channel trasmission aborted by protection");
        respcode = 67;
        break;
      case 38:
        respcode = 68;
        break;
      case 40:
        TRACEENDL("Tag trasmission aborted by protection");
        respcode = 69;
        break;
    }
    buffer[6] = respcode;
    buffer[7] = A31.TOV;
    buffer[8] = A31.FAN[0];
    buffer[9] = A31.FAN[1];

    buffer[10] = A31.ACC;

    if (buffer[6] == 64) {
      TRACEENDL("Disturbance data end.");
      ret = true;
    }

    linklayermanager->UserData(buffer, sizeof(buffer), true);
    return ret;
  }

  // I won't let you copy this object.
  IEC8705103Manager &operator=(const IEC8705103Manager &cSource) {}

  /*Advances current pointer of requested bytes*/
  static inline void SkipBytes(const void **pAsdu, unsigned short bytes = ASDUHeaderSize) {
    *pAsdu = ((unsigned char *)(*pAsdu)) + bytes;
  }

  static unsigned short swap_uint16(unsigned short val) { return (val << 8) | (val >> 8); }

  static unsigned char rev_byte(unsigned char c) {
    int shift;
    unsigned char result = 0;
    for (shift = 0; shift < 8; shift++) {
      if (c & (0x01 << shift)) result |= (0x80 >> shift);
    }
    return result;
  }

  static void ReadAndSwap(const void *pAsdu, const int Size, unsigned short *data) {
    SkipBytes(&pAsdu);
    memcpy(data, pAsdu, Size * sizeof(unsigned short));
    return;
    for (int i = 0; i < Size; i++) {
      data[i] = swap_uint16(data[i]);
      unsigned char *c = reinterpret_cast<unsigned char *>(&data[i]);
      c[0] = rev_byte(c[0]);
      c[1] = rev_byte(c[1]);
    }
  }
};

#endif  // IEC8705103Manager_H
