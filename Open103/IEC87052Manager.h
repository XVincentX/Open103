#ifndef IEC87052MANAGER_H
#define IEC87052MANAGER_H
#pragma once

#include "FT12Fixed.h"
#include "FT12Variable.h"
#include "CommunicationPort.h"

class IEC8705103Manager;


typedef class IEC87052Manager_
{

    friend class IEC8705103Manager;
public:

    IEC87052Manager_(CommunicationPort *port, unsigned char address)	: port(port),address(address), FLastSentFrame(DBG_NEW FT12Fixed()),FLastReceivedFrame(DBG_NEW FT12Fixed()), VLastReceivedFrame(DBG_NEW FT12Variable()), VLastSentFrame(DBG_NEW FT12Variable()), CurrentFCB(0) {}



    /* Function 0 */
    bool ResetRemoteLink()
    {
        PLACEMENT_NEW (FLastSentFrame) FT12Fixed(IFT12::CreateControlByte(1,0,0,0,0,0,0),address);

        LPIFT12 fout = 0;
        CurrentFCB = 1;
        return this->SendReceiveAndCheck(FLastSentFrame,&fout);
    }

    /*Function 3 - 4*/
    bool UserData(const void *pData, const size_t Size, bool Confirm = true)
    {
        unsigned char CByte = (Confirm == true ? IFT12::CreateControlByte(1,CurrentFCB,1,0,0,1,1) : IFT12::CreateControlByte(1,0,0,0,1,0,0));
        if (Confirm)
            CurrentFCB = !CurrentFCB;
        PLACEMENT_NEW (VLastSentFrame) FT12Variable(CByte,address);
        VLastSentFrame->SetUserData(pData,Size);

        LPIFT12 fout = 0;
        return this->SendReceiveAndCheck(VLastSentFrame,&fout);
    }

    /*Function 9*/
    bool StatusLink()
    {
        PLACEMENT_NEW (FLastSentFrame) FT12Fixed(IFT12::CreateControlByte(1,CurrentFCB,0,1,0,0,1),address);
        CurrentFCB = !CurrentFCB;
        LPIFT12 fout = 0;
        return this->SendReceiveAndCheck(FLastSentFrame,&fout);
    }

    /*Function 10-11*/
    bool UserDataClass(unsigned char Class)
    {
        PLACEMENT_NEW (FLastSentFrame) FT12Fixed(IFT12::CreateControlByte(1,CurrentFCB,1,1,0,1,Class - 1),address);
        CurrentFCB = !CurrentFCB;
        LPIFT12 fout = 0;
        return this->SendReceiveAndCheck(FLastSentFrame,&fout);
    }

    void SetAddress(unsigned char NewAddress)
    {
        this->address = NewAddress;
    }

    void SetFCB(unsigned char FCB)
    {
        CurrentFCB = FCB;
    }



    ~IEC87052Manager_()
    {
        if (VLastSentFrame != 0)
        {
            delete VLastSentFrame;
            VLastSentFrame = 0;
        }

        if (VLastReceivedFrame != 0)
        {
            delete VLastReceivedFrame;
            VLastReceivedFrame = 0;
        }

        if (FLastSentFrame != 0)
        {
            delete FLastSentFrame;
            FLastSentFrame = 0;
        }

        if (FLastReceivedFrame != 0)
        {
            delete FLastReceivedFrame;
            FLastReceivedFrame = 0;
        }

    }

private:


    /* Utility function. Write frame on CommunicationPort */
    bool SendFrame(const IFT12 *frame)
    {
        size_t size = frame->FrameSize();
        unsigned char *ptr = static_cast<unsigned char*>(frame->CreateRawBuffer(buffer,0));
        return port->Write(ptr,size) > 0;
    }

    /* Utility function. Reads frame on CommunicationPort */
    bool ReceiveFrame(LPIFT12 *frame)
    {
        tbytes = port->Read(buffer,sizeof(buffer));
        (*frame) = FromRawData(buffer,tbytes);
        return tbytes > 0;
    }

    /* Utility function. Write and reads a frame on CommunicationPort. Will return CheckReturnFrame result only */
    inline bool SendAndReceiveFrame(const IFT12 *frameIn, LPIFT12 *frameOut)
    {
        if (!SendFrame(frameIn))
            return false;
        return ReceiveFrame(frameOut);
    }

    /* Utility function. Write, reads and checks a frame on CommunicationPort. */
    inline bool SendReceiveAndCheck(const IFT12 *frameIn, LPIFT12 *frameOut)
    {
        do
        {
            if (!SendAndReceiveFrame(frameIn,frameOut))
                return false;
        }
        while (CheckControlReturnFrame(frameIn,*frameOut)==false);

        return true;
    }

    /* Scans entire frame for its */
    inline bool CheckControlReturnFrame(const IFT12 *src, const IFT12 *dest)
    {

        if (src->Address != dest->Address)
        {
            TRACEENDL("Received frame is not related to sent one.");

            return false;
        }
        /*0, 1, 8, 9, 11 (responses)*/
        if ((dest->Control & 0x40) == 1)   //bit 2
        {
            TRACEENDL("PRM is not valid.");
            return false;
        }

        if ((dest->Control & 0x20) == 1) //bit 4
        {
            TRACEENDL("DFC indicates an overflow condition.");
            return false;
        }

        unsigned char RetFunc = (dest->Control & 0xF);
        unsigned char StartFunc = (src->Control & 0xF);

        if (StartFunc == 0 || StartFunc == 3)
        {
            if (RetFunc == 1)
            {
                TRACEENDL("Nack confirmation received");
                return false;
            }
            else
            {
                //TRACEENDL("Confirm ack received");
            }
        }
        else if (StartFunc == 4)
            TRACEENDL("There is nothing to check. It will not answer");

        return true;
    }


    unsigned char address;
    unsigned char CurrentFCB;

    IFT12* FromRawData(const void *pData, const size_t Size)
    {
        if (tbytes == 0)
            return 0;
        if (static_cast<const unsigned char*>(pData)[0] == 0x10)
            return PLACEMENT_NEW (FLastReceivedFrame) FT12Fixed(pData,Size);

        return  PLACEMENT_NEW (VLastReceivedFrame) FT12Variable(pData,Size);
    }

    FT12Fixed *FLastSentFrame;
    FT12Fixed *FLastReceivedFrame;
    FT12Variable *VLastSentFrame;
    FT12Variable *VLastReceivedFrame;

    CommunicationPort *port;
    unsigned char buffer[250];
    int tbytes;

} IEC87052Manager;


#endif
