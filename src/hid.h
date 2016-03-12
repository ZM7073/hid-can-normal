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
    //������������ݵ��豸���
    HANDLE hReadHandle;
    //��������д���ݵ��豸���
    HANDLE hWriteHandle;
    //����һ������������豸�ľ����
    HANDLE hDevHandle;
    char *RxBuff;
    char *TxBuff;
    int bSize;

public:
    //���ͱ����õ�OVERLAPPED��
    OVERLAPPED WriteOverlapped;
    //���ձ����õ�OVERLAPPED��
    OVERLAPPED ReadOverlapped;
};

#define EPSIZE 65

#define CAN_PID 0x5750
#define CAN_VID 0x0483

#endif // HID_H
