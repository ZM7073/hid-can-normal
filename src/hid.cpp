
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

//������豸��ť�Ĵ�����
void HidCan::OpenHidDev(){
    //����һ��GUID�Ľṹ��HidGuid������HID�豸�Ľӿ���GUID��
    GUID HidGuid;
    //����һ��DEVINFO�ľ��hDevInfoSet�������ȡ�����豸��Ϣ���Ͼ����
    HDEVINFO hDevInfoSet;
    //����MemberIndex����ʾ��ǰ�������ڼ����豸��0��ʾ��һ���豸��
    DWORD MemberIndex;
    //DevInterfaceData�����������豸�������ӿ���Ϣ
    SP_DEVICE_INTERFACE_DATA DevInterfaceData;
    //����һ��BOOL���������溯�������Ƿ񷵻سɹ�
    BOOL Result;
    //����һ��RequiredSize�ı���������������Ҫ������ϸ��Ϣ�Ļ��峤�ȡ�
    DWORD RequiredSize;
    //����һ��ָ���豸��ϸ��Ϣ�Ľṹ��ָ�롣
    PSP_DEVICE_INTERFACE_DETAIL_DATA	pDevDetailData;

    //����һ��HIDD_ATTRIBUTES�Ľṹ������������豸�����ԡ�
    HIDD_ATTRIBUTES DevAttributes;

    //��DevInterfaceData�ṹ���cbSize��ʼ��Ϊ�ṹ���С
    DevInterfaceData.cbSize=sizeof(DevInterfaceData);
    //��DevAttributes�ṹ���Size��ʼ��Ϊ�ṹ���С
    DevAttributes.Size=sizeof(DevAttributes);

    //����HidD_GetHidGuid������ȡHID�豸��GUID����������HidGuid�С�
    HidD_GetHidGuid(&HidGuid);

    hDevInfoSet=SetupDiGetClassDevs(&HidGuid,
                                    NULL,
                                    NULL,
                                    DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);

    MemberIndex=0;
    bool opened =false;
    while(1){
        //����SetupDiEnumDeviceInterfaces���豸��Ϣ�����л�ȡ���Ϊ
        //MemberIndex���豸��Ϣ��
        Result=SetupDiEnumDeviceInterfaces(hDevInfoSet,
                                           NULL,
                                           &HidGuid,
                                           MemberIndex,
                                           &DevInterfaceData);

        //�����ȡ��Ϣʧ�ܣ���˵���豸�Ѿ�������ϣ��˳�ѭ����
        if(Result==FALSE)
        {
            break;
        }

        //��MemberIndexָ����һ���豸
        MemberIndex++;

        Result=SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
                                               &DevInterfaceData,
                                               NULL,
                                               NULL,
                                               &RequiredSize,
                                               NULL);

        //Ȼ�󣬷���һ����СΪRequiredSize�����������������豸��ϸ��Ϣ��
        pDevDetailData=(PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(RequiredSize);
        if(pDevDetailData==NULL) //����ڴ治�㣬��ֱ�ӷ��ء�
        {
            SetupDiDestroyDeviceInfoList(hDevInfoSet);
            return;
        }

        //������pDevDetailData��cbSizeΪ�ṹ��Ĵ�С��ע��ֻ�ǽṹ���С��
        //���������滺��������
        pDevDetailData->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        //Ȼ���ٴε���SetupDiGetDeviceInterfaceDetail��������ȡ�豸��
        //��ϸ��Ϣ����ε�������ʹ�õĻ������Լ���������С��
        Result=SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
                                               &DevInterfaceData,
                                               pDevDetailData,
                                               RequiredSize,
                                               NULL,
                                               NULL);

        //���豸·�����Ƴ�����Ȼ�����ٸո�������ڴ档
        //MyDevPathName=pDevDetailData->DevicePath;
        //free(pDevDetailData);

        //�������ʧ�ܣ��������һ���豸��
        if(Result==FALSE) continue;

        hDevHandle=CreateFile(pDevDetailData->DevicePath,
                              NULL,
                              FILE_SHARE_READ|FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);        

        //����򿪳ɹ������ȡ�豸���ԡ�
        if(hDevHandle!=INVALID_HANDLE_VALUE){
            //��ȡ�豸�����Բ�������DevAttributes�ṹ����
            Result=HidD_GetAttributes(hDevHandle,
                                      &DevAttributes);

            //�رոոմ򿪵��豸
            CloseHandle(hDevHandle);

            //��ȡʧ�ܣ�������һ��
            if(Result==FALSE) continue;


            //�����ȡ�ɹ����������е�VID��PID�Լ��豸�汾����������Ҫ��
            //���бȽϣ������һ�µĻ�����˵������������Ҫ�ҵ��豸��
            if(DevAttributes.VendorID==CAN_VID) //���VID���
                if(DevAttributes.ProductID==CAN_PID) //����PID���
                    // if(DevAttributes.VersionNumber==MyPvn) //�����豸�汾�����
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

                //д��ʽ���豸               
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
        //�����ʧ�ܣ��������һ���豸
        else continue;
    }

    //����SetupDiDestroyDeviceInfoList���������豸��Ϣ����
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
