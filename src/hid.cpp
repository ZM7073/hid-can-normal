
#include <iostream>
#include <vector>
#include <QString>
#include <QDebug>
#include <QObject>

#include "hid.h"

extern "C" {
#include "setupapi.h"
#include "hidsdi.h"
#include <windows.h>
#include <dbt.h>
}

HidCan::HidCan(int rs): ReportSize(rs), connState(0){
    hReadHandle=INVALID_HANDLE_VALUE;
    hWriteHandle=INVALID_HANDLE_VALUE;
    hDevHandle=INVALID_HANDLE_VALUE;

    ZeroMemory(&WriteOverlapped,sizeof(WriteOverlapped));
    ZeroMemory(&ReadOverlapped,sizeof(ReadOverlapped));

    WriteOverlapped.Offset=0;
    WriteOverlapped.OffsetHigh=0;
    WriteOverlapped.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);

    ReadOverlapped.Offset=0;
    ReadOverlapped.OffsetHigh=0;
    ReadOverlapped.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
}

HidCan::~HidCan(){}

void HidCan::listHidDev(){
    ;
}

void HidCan::FindHidDev(){
    ;
}

void HidCan::IterHidDev(){
    ;
}

//点击打开设备按钮的处理函数
void HidCan::OpenHidDev(){
    //定义一个GUID的结构体HidGuid来保存HID设备的接口类GUID。
    GUID HidGuid;
    //定义一个DEVINFO的句柄hDevInfoSet来保存获取到的设备信息集合句柄。
    HDEVINFO hDevInfoSet;
    //定义MemberIndex，表示当前搜索到第几个设备，0表示第一个设备。
    DWORD MemberIndex;
    //DevInterfaceData，用来保存设备的驱动接口信息
    SP_DEVICE_INTERFACE_DATA DevInterfaceData;
    //定义一个BOOL变量，保存函数调用是否返回成功
    BOOL Result;
    //定义一个RequiredSize的变量，用来接收需要保存详细信息的缓冲长度。
    DWORD RequiredSize;
    //定义一个指向设备详细信息的结构体指针。
    PSP_DEVICE_INTERFACE_DETAIL_DATA	pDevDetailData;

    //定义一个HIDD_ATTRIBUTES的结构体变量，保存设备的属性。
    HIDD_ATTRIBUTES DevAttributes;

    //对DevInterfaceData结构体的cbSize初始化为结构体大小
    DevInterfaceData.cbSize=sizeof(DevInterfaceData);
    //对DevAttributes结构体的Size初始化为结构体大小
    DevAttributes.Size=sizeof(DevAttributes);

    //调用HidD_GetHidGuid函数获取HID设备的GUID，并保存在HidGuid中。
    HidD_GetHidGuid(&HidGuid);

    hDevInfoSet=SetupDiGetClassDevs(&HidGuid,
                                    NULL,
                                    NULL,
                                    DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);

    MemberIndex=0;
    bool opened =false;
    while(1){
        //调用SetupDiEnumDeviceInterfaces在设备信息集合中获取编号为
        //MemberIndex的设备信息。
        Result=SetupDiEnumDeviceInterfaces(hDevInfoSet,
                                           NULL,
                                           &HidGuid,
                                           MemberIndex,
                                           &DevInterfaceData);

        //如果获取信息失败，则说明设备已经查找完毕，退出循环。
        if(Result==FALSE)
        {
            break;
        }

        //将MemberIndex指向下一个设备
        MemberIndex++;

        Result=SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
                                               &DevInterfaceData,
                                               NULL,
                                               NULL,
                                               &RequiredSize,
                                               NULL);

        //然后，分配一个大小为RequiredSize缓冲区，用来保存设备详细信息。
        pDevDetailData=(PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(RequiredSize);
        if(pDevDetailData==NULL) //如果内存不足，则直接返回。
        {
            SetupDiDestroyDeviceInfoList(hDevInfoSet);
            return;
        }

        //并设置pDevDetailData的cbSize为结构体的大小（注意只是结构体大小，
        //不包括后面缓冲区）。
        pDevDetailData->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        //然后再次调用SetupDiGetDeviceInterfaceDetail函数来获取设备的
        //详细信息。这次调用设置使用的缓冲区以及缓冲区大小。
        Result=SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
                                               &DevInterfaceData,
                                               pDevDetailData,
                                               RequiredSize,
                                               NULL,
                                               NULL);

        //将设备路径复制出来，然后销毁刚刚申请的内存。
        //MyDevPathName=pDevDetailData->DevicePath;
        //free(pDevDetailData);

        //如果调用失败，则查找下一个设备。
        if(Result==FALSE) continue;

        hDevHandle=CreateFile(pDevDetailData->DevicePath,
                              NULL,
                              FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);        

        //如果打开成功，则获取设备属性。
        if(hDevHandle!=INVALID_HANDLE_VALUE){
            //获取设备的属性并保存在DevAttributes结构体中
            Result=HidD_GetAttributes(hDevHandle,
                                      &DevAttributes);

            //关闭刚刚打开的设备
            CloseHandle(hDevHandle);

            //获取失败，查找下一个
            if(Result==FALSE) continue;


            //如果获取成功，则将属性中的VID、PID以及设备版本号与我们需要的
            //进行比较，如果都一致的话，则说明它就是我们要找的设备。
            if(DevAttributes.VendorID==CAN_VID) //如果VID相等
                if(DevAttributes.ProductID==CAN_PID) //并且PID相等
                    // if(DevAttributes.VersionNumber==MyPvn) //并且设备版本号相等
                {
                qDebug() << pDevDetailData->DevicePath[0];
                qDebug() << "hid device opened";
                // GetDeviceCapabilities();

                hReadHandle=CreateFile(pDevDetailData->DevicePath,
                                       GENERIC_READ,
                                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                       NULL);

                //写方式打开设备               
                hWriteHandle=CreateFile(pDevDetailData->DevicePath,
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

                if(hReadHandle==INVALID_HANDLE_VALUE || hWriteHandle==INVALID_HANDLE_VALUE){
                    qDebug() << "CreateFile hWriteHandle or hReadHandle fail";
                    return;
                }

                ResetEvent(ReadOverlapped.hEvent);
                ResetEvent(WriteOverlapped.hEvent);

                hidDevPath=pDevDetailData->DevicePath[0];
                connState=1;
                opened=true;
                break;
            }
        }
        //如果打开失败，则查找下一个设备
        else continue;
    }

    //调用SetupDiDestroyDeviceInfoList函数销毁设备信息集合
    SetupDiDestroyDeviceInfoList(hDevInfoSet);
    return;
}

bool HidCan::SendHidReport(QByteArray &WriteReportBuffer){
    if(connState != 1)
        return false;

    BOOL Result;
    qDebug() << "send error" << WriteReportBuffer.size();
    char *buff=WriteReportBuffer.data();
    qDebug() << "send error" << WriteReportBuffer.size();
    Result=WriteFile(hWriteHandle, buff, ReportSize,
                     NULL, &WriteOverlapped);
    return Result;
}

bool HidCan::Send(QByteArray &buff){
    if(connState != 1)
        return false;

    BOOL Result;
    qDebug() << "send error" << buff.size() << (int)buff[2];
    char *nbuff=buff.data();
    qDebug() << "send error" << buff.size();
    Result=WriteFile(hWriteHandle, nbuff, ReportSize,
                     NULL, &WriteOverlapped);
    return Result;
    //buff[0]=0;
    qDebug() << "send error" << buff.size();
    if(SendHidReport(buff)==false){
        qDebug() << "send error";
        return false;
    }
    qDebug() << "send error";
    return true;
}


bool HidCan::RecvHidReport(QByteArray &ReadReportBuffer)
{
    if(connState != 1)
        return false;

    BOOL Result;
    char *nbuff=ReadReportBuffer.data();

    Result=ReadFile(hReadHandle, nbuff, ReportSize,
                    NULL, &ReadOverlapped);
    return Result;
}

bool HidCan::Recv(QByteArray &buff){
    if(RecvHidReport(buff)==false)
    {
        qDebug() << "recv error";
        return false;
    }
    return true;
}

void HidCan::GetDeviceCapabilities()
{
    //Get the Capabilities structure for the device.
    PHIDP_PREPARSED_DATA	PreparsedData;
    HIDP_CAPS  Capabilities;
    ZeroMemory(&Capabilities, sizeof(HIDP_CAPS));

    HidD_GetPreparsedData
            (hDevHandle,
             &PreparsedData);

    HidP_GetCaps
            (PreparsedData,
             &Capabilities);

    qDebug() << "Usage Page: " << Capabilities.UsagePage;

    qDebug() << "Input Report Byte Length: " <<  Capabilities.InputReportByteLength;
    qDebug() << "Output Report Byte Length: " <<  Capabilities.OutputReportByteLength;
    qDebug() << "Feature Report Byte Length: " <<  Capabilities.FeatureReportByteLength;
    qDebug() << "Number of Link Collection Nodes: " <<  Capabilities.NumberLinkCollectionNodes;
    qDebug() << "Number of Input Button Caps: " <<  Capabilities.NumberInputButtonCaps;
    qDebug() << "Number of InputValue Caps: " <<  Capabilities.NumberInputValueCaps;
    qDebug() << "Number of InputData Indices: " <<  Capabilities.NumberInputDataIndices;
    qDebug() << "Number of Output Button Caps: " <<  Capabilities.NumberOutputButtonCaps;
    qDebug() << "Number of Output Value Caps: " <<  Capabilities.NumberOutputValueCaps;
    qDebug() << "Number of Output Data Indices: " <<  Capabilities.NumberOutputDataIndices;
    qDebug() << "Number of Feature Button Caps: " <<  Capabilities.NumberFeatureButtonCaps;
    qDebug() << "Number of Feature Value Caps: " <<  Capabilities.NumberFeatureValueCaps;
    qDebug() << "Number of Feature Data Indices: " <<  Capabilities.NumberFeatureDataIndices;

    HidD_FreePreparsedData(PreparsedData);
}
