/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QThread>
#include <QtGui/QLineEdit>
#include <QQueue>
#include <QMetaType>

#include <vector>
#include "math.h"

#include "hid.h"

QT_BEGIN_NAMESPACE
class QDateTimeEdit;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QThread;
class QStatusBar;
class QHBoxLayout;
class QVBoxLayout;
class QTextEdit;
class QTextCharFormat;
class QTextCursor;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE

class HidIOEvent;
class KeyLineEdit;
class Reg;

struct CommData
{
    /*char brp;
    char tsg1;
    char tsg2;
    char reserved[EPSIZE-3];*/
public:
    CommData();
    CommData(QByteArray&);
    QByteArray* toByteArray();

public:
    char num;
    char flag;
    char can1_bsp;
    char bs1_bs2_1;
    short can1_start_sid;
    short can1_end_sid;
    char can2_bsp;
    char bs1_bs2_2;
    short can2_start_sid;
    short can2_end_sid;

    char reserved[EPSIZE-14];

    QByteArray *canByteArray;
};

Q_DECLARE_METATYPE(CommData)

class Window : public QWidget{
    Q_OBJECT

public:
    Window();    
    ~Window();

    void runs();

protected:
    bool winEvent(MSG *msg, long *result);
    void closeEvent(QCloseEvent*);

public slots:
    void setLBitRate(int br);
    void setRBitRate(int br);
    void setLSp(int sp);
    void setRSp(int sp);

    void onWriteButtonClicked();
    void onReadButtonClicked();
    void setValue( int );
    void getLineEditText();

    void lCustomBrCheck();
    void lFilterBrCheck();
    void rCustomBrCheck();
    void rFilterBrCheck();

signals:
    void valueChanged( int );

private:
    void createSpinBoxes();
    void createDateTimeEdits();
    void createButtons();
    void createCommDebug();

    void parseInputText();
    bool prepareSendData();
    void prepareReadData();
    void convertRecvData();
    int calcRegs();
    long findReg(const CommData &RecvData);

    QString processText(const QByteArray &byte_array, const QString &in_out);
    bool checkInputText(const QString &inputText, QByteArray &inData);

    QGroupBox *spinBoxesGroup;
    QGroupBox *editsGroup;

    void registerUsbHotPlugEvent();

    unsigned short lBrNum;
    unsigned short rBrNum;
    unsigned short lSpNum;
    unsigned short rSpNum;
    bool lCustomBr;
    bool rCustomBr;
    bool lFilterBr;
    bool rFilterBr;

    HidCan *hidDev;
    HidIOEvent *hidThreads;
    QHBoxLayout *xLayout;
    QVBoxLayout *commDebugLayout;
    QStatusBar *statsBar;

    KeyLineEdit *textBrowser2;
    QTextEdit *textBrowser1;
    QComboBox *lSjwSpinBox;
    QComboBox *rSjwSpinBox;
    QComboBox *lFormatComboBox;
    QComboBox *rFormatComboBox;
    QDoubleSpinBox *lBrpSpinBox;
    QDoubleSpinBox *rBrpSpinBox;
    QDoubleSpinBox *lBs1SpinBox;
    QDoubleSpinBox *rBs1SpinBox;
    QDoubleSpinBox *lBs2SpinBox;
    QDoubleSpinBox *rBs2SpinBox;
    QLineEdit *lSIdLineEdit;
    QLineEdit *lEIdLineEdit;
    QLineEdit *rSIdLineEdit;
    QLineEdit *rEIdLineEdit;

    QByteArray RecvBuff;
    QByteArray SendBuff;
    CommData *RecvCommData;
    CommData *SendCommData;    

    std::vector<Reg> r5k;
    std::vector<Reg> r1m;
};

class HidIOEvent: public QThread{
public:
    HidIOEvent(Window*);
    ~HidIOEvent();
    void stop()    {
        stopped=true;
    }

protected:
    void run();

private:
    Window *pWd;
    volatile bool stopped;
};

class KeyLineEdit: public QLineEdit{
public:
    KeyLineEdit(int max=30, int memoI=0): maxMemoNum(max), memoIdx(memoI){}
    void enqueue();

protected:
    virtual void keyPressEvent(QKeyEvent * event );

private:
    QQueue<QString> textMemo;
    int maxMemoNum;
    int memoIdx;

private:
    void keyDown();
    void keyUp();
    void dequeue();
};

struct Reg{
public:
    Reg(char brp, char t1, char t2, long br):
            BRP(brp), TSG1(t1), TSG2(t2), bitRate(br)
    {
        // bitRate=fclk_io/(2*(BRP+1)*((TSG1+1)+(TSG2+1)+1);
        samplePos=(TSG1+1)*100/(TSG2+TSG1+1);
    }
    Reg(){}

    char BRP;
    char TSG1;
    char TSG2;
    long bitRate;
    int samplePos;
};

#define BR5K 500000
#define BR1M 1000000
#define BRBIAS 0.01

#define fclk_io 19.6608E6 // 19.6608MHz
#define fclk_main 30E6 // 50MHz

#endif
