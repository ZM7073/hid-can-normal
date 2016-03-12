#ifndef HID_H
#define HID_H

extern "C" {
#include "windows.h"
}

#include <QObject>

class HidCan: public QObject
{
    //Q_OBJECT

public:
    HidCan(int);
    ~HidCan();

    void listHidDev();
    void FindHidDev();
    void IterHidDev();
    void OpenHidDev();
    void GetDeviceCapabilities();
    volatile int connState;
    WCHAR hidDevPath;
    bool Send(QByteArray &);
    bool Recv(QByteArray&);

private:
    bool SendHidReport(QByteArray&);
    bool RecvHidReport(QByteArray&);
    int ReportSize;

public:
    //用来保存读数据的设备句柄
    HANDLE hReadHandle;
    //用来保存写数据的设备句柄
    HANDLE hWriteHandle;
    //定义一个用来保存打开设备的句柄。
    HANDLE hDevHandle;
    char *RxBuff;
    char *TxBuff;
    int bSize;

public:
    //发送报告用的OVERLAPPED。
    OVERLAPPED WriteOverlapped;
    //接收报告用的OVERLAPPED。
    OVERLAPPED ReadOverlapped;
};

#define EPSIZE 65

#define CAN_PID 0x5750
#define CAN_VID 0x0483

#endif // HID_H
