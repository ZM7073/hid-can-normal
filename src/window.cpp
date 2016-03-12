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

#include <QtGui>
#include <QDebug>

#define _WIN32_WINNT 0x0500
#define _WIN32_WINDOWS 0x0500
#define WINVER 0x0500

extern "C" {
#include "windows.h"
#include "dbt.h"
#include "setupapi.h"
#include "hidsdi.h"
}
#include "window.h"
Window::Window()
{
    RecvBuff.fill(0, 65);
    //SendBuff = new QByteArray(65, 97);
    //RecvCommData = new CommData;
    SendCommData = new CommData;

    lBrNum = 0;
    rBrNum = 0;
    lSpNum = 0;
    rSpNum = 0;
    lCustomBr = false;
    rCustomBr = false;
    lFilterBr = false;
    rFilterBr = false;
    calcRegs();

    setFixedSize (800, 500);
    setWindowTitle(tr("CAN Setting"));

    createSpinBoxes();
    createDateTimeEdits();
    createButtons();
    createCommDebug();

    QHBoxLayout *sLayout = new QHBoxLayout;
    spinBoxesGroup->setFixedWidth(380);
    editsGroup->setFixedWidth(380);
    sLayout->addWidget(spinBoxesGroup);
    sLayout->addWidget(editsGroup);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(sLayout);
    mainLayout->addLayout(xLayout);
    mainLayout->addLayout(commDebugLayout);
    setLayout(mainLayout);

    hidDev = new HidCan(65);
    hidThreads = new HidIOEvent(this);
    hidDev->OpenHidDev();
    registerUsbHotPlugEvent();
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));

    hidThreads->start();

    if (hidDev->connState == 1) {
        statsBar->showMessage(tr("设备连接成功"));
        hidDev->Recv(RecvBuff);//emit valueChanged(a);
        //hidDev->Recv(*(RecvCommData->toByteArray()));//emit valueChanged(int);
    }
    else {
        statsBar->showMessage(tr("设备未连接或打开失败"));
    }
}

Window::~Window() {}

QString Window::processText(const QByteArray &byte_array, const QString &in_out) {
    QString str;
    QDateTime current_date_time = QDateTime::currentDateTime();

    QString t_str;
    for (int i = 0; i < byte_array.size(); i++) {
        str.append(t_str.setNum(byte_array[i]));
        str.append(" ");
    }

    str = QString("[ %1 %2 ]: %3\n")
          .arg(in_out, current_date_time.toString("hh:mm:ss")).arg(str);

    return str;
}

void Window::setValue( int a) {
    QByteArray ba;
    QByteArray list;
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = "[ IN " + current_date_time.toString("hh:mm:ss") + "]: ";
    list.append(current_date);
    for (int i = 0; i < RecvBuff.size(); i++) {
        list.append(ba.setNum(RecvBuff[i]));           // ba == "63"
        list.append(" ");
    }
    list.append("\n");
    //
    QTextCursor cursor = textBrowser1->textCursor();
    if (!cursor.atEnd())
        cursor.movePosition( QTextCursor::End);

    cursor.insertText(QString(list));
    //
    //textBrowser1->insertPlainText(QString(list));
    qDebug() << "kkk" << (int)RecvBuff[5] << (int)RecvBuff[10];
    RecvCommData = new CommData(RecvBuff);
    qDebug() << "recvcommdata" << (int)RecvCommData->bs1_bs2_1 << RecvCommData->can1_start_sid
             << RecvCommData->can1_end_sid << (int)SendCommData->flag;

    if ((int)SendCommData->flag == -128)
        convertRecvData();

    RecvBuff.fill('0', 65);
    hidDev->Recv(RecvBuff);
    //        << (*(RecvCommData->toByteArray()))[2]
    //        << (*(RecvCommData->toByteArray()))[1];
    //hidDev->Recv(*(RecvCommData->toByteArray()));
}

void Window::parseInputText() {
    //checkInputText();
}

bool Window::checkInputText(const QString &inputText, QByteArray &inData) {
    QStringList numList;
    SendBuff.fill(0, 65);
    SendBuff.resize(65);
    if (inputText[0].isNumber()) {
        QStringList numList = inputText.split(QRegExp("[\\s|\,]+"));
        if (numList.size() > 64)
            return false;
        for (int i = 0; i < numList.size(); i++) {
            bool ok = true;
            int tmp = numList.at(i).toInt(&ok, 10);
            if (!ok || tmp > 127)
                return false;
            inData[i + 1] = (char)tmp;
        }
    }
    else if (inputText[0] == ':') {
        numList = inputText.split(QRegExp("[\\s]+"));
        QString key = numList[0].remove(0, 1);
        if (key == "SEQ") {
            QStringList paraList = numList[1].split("\:");
            int begin, end, step;
            bool ok1 = true, ok2 = true, ok3 = true;

            begin = paraList[0].toInt(&ok1, 10);

            if (paraList.size() == 3) {
                step = paraList[1].toInt(&ok2, 10);
                end = paraList[2].toInt(&ok3, 10);
            }
            else if (paraList.size() == 2) {
                end = paraList[1].toInt(&ok2, 10);
                step = 1;
            }
            else {
                return false;
            }

            if (!ok1 || !ok2 || !ok3 || begin > 127 || end > 127
                    || step > 127 || step == 0 || (end - begin) / step > 64)
                return false;

            for (int i = begin, j = 1; i < end; i += step, j++)
                inData[j] = i;
        }
        else if (key == "TIMER") {
            ;
        }
        else
            return false;
    }
    else {
        return false;
    }
    return true;
}

void Window::getLineEditText() {
    if (!checkInputText(textBrowser2->text(), SendBuff))
        return;
    textBrowser2->enqueue();
    QByteArray list = SendBuff;

    QByteArray alist;
    QByteArray ba;
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = "[ OUT " + current_date_time.toString("hh:mm:ss") + "]: ";
    alist.append(current_date);
    for (int i = 0; i < list.size(); i++) {
        alist.append(ba.setNum(list[i]));           // ba == "63"
        alist.append(" ");
    }
    alist.append("\n");
    //
    QTextCursor cursor = textBrowser1->textCursor();
    if (!cursor.atEnd())
        cursor.movePosition( QTextCursor::End);

    if (hidDev->connState == 1)
        cursor.insertText(QString(alist));

    qDebug() << SendBuff;
    //hidDev->Send(SendBuff);

    textBrowser2->setText("");
}

void Window::createCommDebug() {
    commDebugLayout = new QVBoxLayout;
    QLabel *formatLabel1 = new QLabel(tr("接收数据:"));
    textBrowser1 = new QTextEdit;
    textBrowser1->document()->setMaximumBlockCount (500);
    textBrowser1->setReadOnly(true);
    //self.textBrowser1.setTextColor( QtGui.QColor(255,0,0))
    QTextCharFormat *format = new QTextCharFormat;
    format->setForeground(QColor(255, 0, 0));
    QTextCursor cursor = textBrowser1->textCursor();
    cursor.setCharFormat( *format );
    // cursor.insertText("012123");
    QLabel *formatLabel2 = new QLabel(tr("发送数据:"));
    textBrowser2 = new KeyLineEdit;
    connect(textBrowser2, SIGNAL(returnPressed()), this, SLOT(getLineEditText()));
    commDebugLayout->addWidget(formatLabel1);
    commDebugLayout->addWidget(textBrowser1);
    commDebugLayout->addWidget(formatLabel2);
    commDebugLayout->addWidget(textBrowser2);
}

void Window::createButtons() {
    // status bar
    statsBar = new QStatusBar(this);
    statsBar->showMessage(tr("状态信息"));
    statsBar->setMaximumHeight(15);

    xLayout = new QHBoxLayout;
    QPushButton *readButton = new QPushButton(tr("参数读取"));
    connect(readButton, SIGNAL(clicked()), this, SLOT(onReadButtonClicked()));
    QPushButton *writeButton = new QPushButton(tr("参数下载"));
    connect(writeButton, SIGNAL(clicked()), this, SLOT(onWriteButtonClicked()));
    xLayout->setSpacing(20);
    xLayout->addWidget(readButton);
    xLayout->addWidget(writeButton);
    xLayout->addStretch();
    xLayout->addWidget(statsBar);
}

void Window::createSpinBoxes() {
    spinBoxesGroup = new QGroupBox(tr("CAN0"));

    QLabel *formatLabel = new QLabel(tr("波特率:"));
    formatLabel->setMaximumWidth(50);
    //formatLabel->setMaximumWidth(40);
    lFormatComboBox = new QComboBox;
    lFormatComboBox->setMinimumWidth(80);
    lFormatComboBox->addItem(QString(""));
    lFormatComboBox->addItem("1Mbps");
    lFormatComboBox->addItem("500Kbps");
    //connect(lFormatComboBox, SIGNAL(activated(int)), this, SLOT(setLBitRate(int)));
    connect(lFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setLBitRate(int)));

    QLabel *sjwLabel = new QLabel(tr("  采样点:"));
    lSjwSpinBox = new QComboBox;
    lSjwSpinBox->setMaxVisibleItems(4);
    lSjwSpinBox->clear();
    lSjwSpinBox->addItem(QString(""));
    for (int i = 0; i < r1m.size(); i++) {
        lSjwSpinBox->addItem(QString("").setNum(r1m[i].samplePos));
    }
    connect(lSjwSpinBox, SIGNAL(activated(int)), this, SLOT(setLSp(int)));

    QLabel *customLabel = new QLabel(tr("自定义波特率"));
    QCheckBox *customBrBox = new QCheckBox;
    connect(customBrBox, SIGNAL(clicked()), this, SLOT(lCustomBrCheck()));
    QLabel *filterLabel = new QLabel(tr("启动滤波"));
    QCheckBox *filterBrBox = new QCheckBox;
    connect(filterBrBox, SIGNAL(clicked()), this, SLOT(lFilterBrCheck()));

    QLabel *brpLabel = new QLabel(tr("  BRP:"));
    lBrpSpinBox = new QDoubleSpinBox;

    lBrpSpinBox->setRange(1, 16);
    lBrpSpinBox->setSingleStep(1);
    lBrpSpinBox->setValue(0);
    lBrpSpinBox->setDecimals(0);

    QLabel *bs1Label = new QLabel(tr("BS1:"));
    lBs1SpinBox = new QDoubleSpinBox;
    lBs1SpinBox->setRange(1, 16);
    lBs1SpinBox->setSingleStep(1);
    lBs1SpinBox->setValue(0);
    lBs1SpinBox->setDecimals(0);

    QLabel *bs2Label = new QLabel(tr("  BS2:"));
    lBs2SpinBox = new QDoubleSpinBox;
    lBs2SpinBox->setRange(1, 8);
    lBs2SpinBox->setSingleStep(1);
    lBs2SpinBox->setValue(0);
    lBs2SpinBox->setDecimals(0);

    QLabel *canIdLabel = new QLabel(tr("CAN ID 范围:"));
    lSIdLineEdit = new QLineEdit;
    lSIdLineEdit->setMaxLength(4);
    QRegExp sRegx("[0-9|a-f|A-F]+$");
    QValidator *validator = new QRegExpValidator(sRegx, lSIdLineEdit);
    lSIdLineEdit->setValidator(validator);

    QLabel *mCanIdLabel = new QLabel(tr("~"));
    lEIdLineEdit = new QLineEdit;
    lEIdLineEdit->setMaxLength(4);
    lEIdLineEdit->setValidator(validator);

    if (!lCustomBr) {
        lSIdLineEdit->setEnabled(false);
        lEIdLineEdit->setEnabled(false);
        lFormatComboBox->setEnabled(true);
        lSjwSpinBox->setEnabled(true);
    }

    if (!lFilterBr) {
        lBrpSpinBox->setEnabled(false);
        lBs1SpinBox->setEnabled(false);
        lBs2SpinBox->setEnabled(false);
    }

    QGridLayout *editsLayout = new QGridLayout;
    editsLayout->addWidget(formatLabel, 0, 0, Qt::AlignLeft);
    editsLayout->addWidget(lFormatComboBox, 0, 1);
    editsLayout->addWidget(sjwLabel, 0, 2);
    editsLayout->addWidget(lSjwSpinBox, 0, 3);

    editsLayout->addWidget(customLabel, 1, 0);
    editsLayout->addWidget(customBrBox, 1, 1);
    editsLayout->addWidget(brpLabel, 1, 2, Qt::AlignLeft);
    editsLayout->addWidget(lBrpSpinBox, 1, 3);
    editsLayout->addWidget(bs1Label, 2, 0, Qt::AlignLeft);
    editsLayout->addWidget(lBs1SpinBox, 2, 1);
    editsLayout->addWidget(bs2Label, 2, 2, Qt::AlignLeft);
    editsLayout->addWidget(lBs2SpinBox, 2, 3);

    editsLayout->addWidget(filterLabel, 3, 0);
    editsLayout->addWidget(filterBrBox, 3, 1);
    editsLayout->addWidget(canIdLabel, 4, 0);
    editsLayout->addWidget(lSIdLineEdit, 4, 1);
    editsLayout->addWidget(mCanIdLabel, 4, 2, Qt::AlignHCenter);
    editsLayout->addWidget(lEIdLineEdit, 4, 3);

    // editsLayout->addStretch();
    spinBoxesGroup->setLayout(editsLayout);
}

void Window::lCustomBrCheck() {
    if (lCustomBr) {
        lCustomBr = false;
        lBrpSpinBox->setEnabled(false);
        lBs1SpinBox->setEnabled(false);
        lBs2SpinBox->setEnabled(false);
        lFormatComboBox->setEnabled(true);
        lSjwSpinBox->setEnabled(true);
    }
    else {
        lCustomBr = true;
        lBrpSpinBox->setEnabled(true);
        lBs1SpinBox->setEnabled(true);
        lBs2SpinBox->setEnabled(true);
        lFormatComboBox->setEnabled(false);
        lSjwSpinBox->setEnabled(false);
    }
    qDebug() << "lCustomBrCheck" << lCustomBr;
}

void Window::rCustomBrCheck() {
    if (rCustomBr) {
        rCustomBr = false;
        rBrpSpinBox->setEnabled(false);
        rBs1SpinBox->setEnabled(false);
        rBs2SpinBox->setEnabled(false);
        rFormatComboBox->setEnabled(true);
        rSjwSpinBox->setEnabled(true);
    }
    else {
        rCustomBr = true;
        rBrpSpinBox->setEnabled(true);
        rBs1SpinBox->setEnabled(true);
        rBs2SpinBox->setEnabled(true);
        rFormatComboBox->setEnabled(false);
        rSjwSpinBox->setEnabled(false);
    }
    qDebug() << "rCustomBrCheck" << lCustomBr;
}

void Window::lFilterBrCheck() {
    if (lFilterBr) {
        lFilterBr = false;
        lSIdLineEdit->setEnabled(false);
        lEIdLineEdit->setEnabled(false);
    }
    else {
        lFilterBr = true;
        lSIdLineEdit->setEnabled(true);
        lEIdLineEdit->setEnabled(true);
    }
    qDebug() << "lFilterBrCheck" << lFilterBr;
}

void Window::rFilterBrCheck() {
    if (rFilterBr) {
        rFilterBr = false;
        rSIdLineEdit->setEnabled(false);
        rEIdLineEdit->setEnabled(false);
    }
    else {
        rFilterBr = true;
        rSIdLineEdit->setEnabled(true);
        rEIdLineEdit->setEnabled(true);
    }
    qDebug() << "rFilterBrCheck" << lFilterBr;
}

void Window::createDateTimeEdits() {
    editsGroup = new QGroupBox(tr("CAN1"));

    QLabel *formatLabel = new QLabel(tr("波特率:"));
    formatLabel->setMaximumWidth(50);
    //formatLabel->setMaximumWidth(40);
    rFormatComboBox = new QComboBox;
    rFormatComboBox->setMinimumWidth(80);
    rFormatComboBox->addItem(QString(""));
    rFormatComboBox->addItem("1Mbps");
    rFormatComboBox->addItem("500Kbps");
    //connect(lFormatComboBox, SIGNAL(activated(int)), this, SLOT(setLBitRate(int)));
    connect(rFormatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setRBitRate(int)));


    QLabel *sjwLabel = new QLabel(tr("  采样点:"));
    rSjwSpinBox = new QComboBox;
    rSjwSpinBox->setMaxVisibleItems(4);
    rSjwSpinBox->clear();
    rSjwSpinBox->addItem(QString(""));
    for (int i = 0; i < r1m.size(); i++) {
        rSjwSpinBox->addItem(QString("").setNum(r1m[i].samplePos));
    }
    connect(rSjwSpinBox, SIGNAL(activated(int)), this, SLOT(setRSp(int)));

    QLabel *customLabel = new QLabel(tr("自定义波特率"));
    QCheckBox *customBrBox = new QCheckBox;
    connect(customBrBox, SIGNAL(clicked()), this, SLOT(rCustomBrCheck()));
    QLabel *filterLabel = new QLabel(tr("启动滤波"));
    QCheckBox *filterBrBox = new QCheckBox;
    connect(filterBrBox, SIGNAL(clicked()), this, SLOT(rFilterBrCheck()));

    QLabel *brpLabel = new QLabel(tr("  BRP:"));
    rBrpSpinBox = new QDoubleSpinBox;

    rBrpSpinBox->setRange(1, 16);
    rBrpSpinBox->setSingleStep(1);
    rBrpSpinBox->setValue(0);
    rBrpSpinBox->setDecimals(0);

    QLabel *bs1Label = new QLabel(tr("BS1:"));
    rBs1SpinBox = new QDoubleSpinBox;
    rBs1SpinBox->setRange(1, 16);
    rBs1SpinBox->setSingleStep(1);
    rBs1SpinBox->setValue(0);
    rBs1SpinBox->setDecimals(0);

    QLabel *bs2Label = new QLabel(tr("  BS2:"));
    rBs2SpinBox = new QDoubleSpinBox;
    rBs2SpinBox->setRange(1, 8);
    rBs2SpinBox->setSingleStep(1);
    rBs2SpinBox->setValue(0);
    rBs2SpinBox->setDecimals(0);

    QLabel *canIdLabel = new QLabel(tr("CAN ID 范围:"));
    rSIdLineEdit = new QLineEdit;
    rSIdLineEdit->setMaxLength(4);
    QRegExp sRegx("[0-9|a-f|A-F]+$");
    QValidator *validator = new QRegExpValidator(sRegx, lSIdLineEdit);
    rSIdLineEdit->setValidator(validator);

    QLabel *mCanIdLabel = new QLabel(tr("~"));
    rEIdLineEdit = new QLineEdit;
    rEIdLineEdit->setMaxLength(4);
    rEIdLineEdit->setValidator(validator);

    if (!rCustomBr) {
        rSIdLineEdit->setEnabled(false);
        rEIdLineEdit->setEnabled(false);
        rFormatComboBox->setEnabled(true);
        rSjwSpinBox->setEnabled(true);
    }

    if (!rFilterBr) {
        rBrpSpinBox->setEnabled(false);
        rBs1SpinBox->setEnabled(false);
        rBs2SpinBox->setEnabled(false);
    }

    QGridLayout *editsLayout = new QGridLayout;
    editsLayout->addWidget(formatLabel, 0, 0, Qt::AlignLeft);
    editsLayout->addWidget(rFormatComboBox, 0, 1);
    editsLayout->addWidget(sjwLabel, 0, 2);
    editsLayout->addWidget(rSjwSpinBox, 0, 3);

    editsLayout->addWidget(customLabel, 1, 0);
    editsLayout->addWidget(customBrBox, 1, 1);
    editsLayout->addWidget(brpLabel, 1, 2, Qt::AlignLeft);
    editsLayout->addWidget(rBrpSpinBox, 1, 3);
    editsLayout->addWidget(bs1Label, 2, 0, Qt::AlignLeft);
    editsLayout->addWidget(rBs1SpinBox, 2, 1);
    editsLayout->addWidget(bs2Label, 2, 2, Qt::AlignLeft);
    editsLayout->addWidget(rBs2SpinBox, 2, 3);

    editsLayout->addWidget(filterLabel, 3, 0);
    editsLayout->addWidget(filterBrBox, 3, 1);
    editsLayout->addWidget(canIdLabel, 4, 0);
    editsLayout->addWidget(rSIdLineEdit, 4, 1);
    editsLayout->addWidget(mCanIdLabel, 4, 2, Qt::AlignHCenter);
    editsLayout->addWidget(rEIdLineEdit, 4, 3);

    // editsLayout->addStretch();
    editsGroup->setLayout(editsLayout);
}


void Window::setLBitRate(int br) {
    lBrNum = br;
    std::vector<Reg>::iterator begin, end, iter;
    if (br == 1) {
        begin = r1m.begin();
        end = r1m.end();
    }
    else if (br == 2) {
        begin = r5k.begin();
        end = r5k.end();
    }
    else {
        begin = end;
    }

    iter = begin;
    lSjwSpinBox->clear();
    lSjwSpinBox->addItem(QString(""));
    while (iter != end) {
        lSjwSpinBox->addItem(QString("").setNum(iter->samplePos));
        iter++;
    }
    qDebug() << "setLBitRate " << br;
}

void Window::setRBitRate(int br) {
    rBrNum = br;
    std::vector<Reg>::iterator begin, end, iter;
    if (br == 1) {
        begin = r1m.begin();
        end = r1m.end();
    }
    else if (br == 2) {
        begin = r5k.begin();
        end = r5k.end();
    }
    else {
        begin = end;
    }

    iter = begin;
    rSjwSpinBox->clear();
    rSjwSpinBox->addItem(QString(""));
    while (iter != end) {
        rSjwSpinBox->addItem(QString("").setNum(iter->samplePos));
        iter++;
    }
    qDebug() << "setRBitRate " << br;
}

void Window::setLSp(int sp) {
    lSpNum = sp;

    Reg lReg;
    if (lBrNum == 1) {
        lReg = r1m[sp];
    }
    else if (lBrNum == 2) {
        lReg = r5k[sp];
    }
    else {
        return;
    }

    lBs1SpinBox->setValue(lReg.TSG1);
    lBs2SpinBox->setValue(lReg.TSG2);
    QString str1;
    str1.setNum(0, 16);
    lSIdLineEdit->setText(str1);
    QString str2;
    str2.setNum(0x3FF, 16);
    lEIdLineEdit->setText(str2);

    qDebug() << "setlsp " << sp;
}

void Window::setRSp(int sp) {
    rSpNum = sp;

    Reg rReg;
    if (rBrNum == 1) {
        rReg = r1m[sp];
    }
    else if (rBrNum == 2) {
        rReg = r5k[sp];
    }
    else {
        return;
    }

    rBs1SpinBox->setValue(rReg.TSG1);
    rBs2SpinBox->setValue(rReg.TSG2);
    QString str1;
    str1.setNum(0, 16);
    rSIdLineEdit->setText(str1);
    QString str2;
    str2.setNum(0x3ff, 16);
    rEIdLineEdit->setText(str2);

    qDebug() << "setrsp " << sp;
}

bool Window::winEvent(MSG *msg, long *result) {
    int msgType = msg->message;
    PDEV_BROADCAST_HDR   lpdb = (PDEV_BROADCAST_HDR )msg->lParam;

    if (msgType == WM_DEVICECHANGE)
    {
        switch (msg->wParam)
        {
        case DBT_DEVICETYPESPECIFIC:
            qDebug() << "DBT_DEVICETYPESPECIFIC ";
            break;

        case DBT_DEVICEARRIVAL:
            if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
            {
                PDEV_BROADCAST_DEVICEINTERFACE lpdbIntf = (PDEV_BROADCAST_DEVICEINTERFACE)lpdb;
                hidDev->OpenHidDev();

                if (hidDev->connState == 1) {
                    int a;
                    hidDev->Recv(RecvBuff);//emit valueChanged(a);RecvCommData
                    //hidDev->Recv(*(RecvCommData->toByteArray()));//emit valueChanged(a);
                    qDebug() << "hid arrival" << lpdbIntf->dbcc_name[0];
                    statsBar->showMessage(tr("设备连接成功"));
                }
            }
            break;

        case DBT_DEVICEREMOVECOMPLETE:
            if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
            {
                PDEV_BROADCAST_DEVICEINTERFACE lpdbIntf = (PDEV_BROADCAST_DEVICEINTERFACE)lpdb;
                if (lpdbIntf->dbcc_name[0] == hidDev->hidDevPath)
                {
                    // hidDev->connState=2;
                    // CancelIo(hidDev->hReadHandle);
                    qDebug() << "hid remove";
                    statsBar->showMessage(tr("设备未连接或打开失败"));
                }
            }
            break;
        }
    }

    return false;
}

void Window::registerUsbHotPlugEvent() {
    qDebug() << "register hid event";
    GUID HidGuid;
    DEV_BROADCAST_DEVICEINTERFACE DevBroadcastDeviceInterface;
    HidD_GetHidGuid(&HidGuid);
    //设置DevBroadcastDeviceInterface结构体，用来注册设备改变时的通知
    DevBroadcastDeviceInterface.dbcc_size = sizeof(DevBroadcastDeviceInterface);
    DevBroadcastDeviceInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    DevBroadcastDeviceInterface.dbcc_classguid = HidGuid;
    //注册设备改变时收到通知
    RegisterDeviceNotification(winId(),
                               &DevBroadcastDeviceInterface,
                               DEVICE_NOTIFY_WINDOW_HANDLE);
}

bool Window::prepareSendData() {
    if (lSpNum == 0 || lBrNum == 0 || rSpNum == 0 || 0 == rBrNum)
        return false;

    SendCommData->num = 0;
    SendCommData->flag = 0x40;

    if (lFilterBr) {
        bool *ok = 0;
        SendCommData->can1_start_sid = lSIdLineEdit->text().toUShort(ok, 16);
        ok = 0;
        SendCommData->can1_end_sid = lEIdLineEdit->text().toUShort(ok, 16);
    }
    else {
        SendCommData->can1_start_sid = 0;
        SendCommData->can1_end_sid = 0x3ff;
    }

    if (rFilterBr) {
        bool *ok = 0;
        SendCommData->can2_start_sid = rSIdLineEdit->text().toUShort(ok, 16);
        ok = 0;
        SendCommData->can2_end_sid = rEIdLineEdit->text().toUShort(ok, 16);
    }
    else {
        SendCommData->can2_start_sid = 0;
        SendCommData->can2_end_sid = 0x3ff;
    }

    if (lCustomBr) {
        SendCommData->can1_bsp = 1;
        SendCommData->bs1_bs2_1 = (char)lBs1SpinBox->value() << 4 | (char)lBs2SpinBox->value();

        qDebug() << "debug input" << lBs1SpinBox->value()  << lBs2SpinBox->value()
                 << (int)SendCommData->bs1_bs2_1 << lSIdLineEdit->text() << lEIdLineEdit->text();
    }
    else {
        Reg lSelectSp;
        if (lBrNum == 1) {
            lSelectSp = r1m[lSpNum - 1];
        }
        else if (lBrNum == 2) {
            lSelectSp = r5k[lSpNum - 1];
        }
        else {
            return false;
        }

        SendCommData->can1_bsp = lSelectSp.BRP;
        SendCommData->bs1_bs2_1 = (lSelectSp.TSG1 << 4) | lSelectSp.TSG2;
    }

    if (rCustomBr) {
        SendCommData->can2_bsp = 1;
        SendCommData->bs1_bs2_2 = (char)rBs1SpinBox->value() << 4 | (char)rBs2SpinBox->value();

        qDebug() << "debug input" << lBs1SpinBox->value()  << lBs2SpinBox->value()
                 << (int)SendCommData->bs1_bs2_1 << lSIdLineEdit->text() << lEIdLineEdit->text();
    }
    else {
        Reg rSelectSp;

        if (rBrNum == 1) {
            rSelectSp = r1m[rSpNum - 1];
        }
        else if (rBrNum == 2) {
            rSelectSp = r5k[rSpNum - 1];
        }
        else {
            //return false;
        }

        SendCommData->can2_bsp = rSelectSp.BRP;
        SendCommData->bs1_bs2_2 = (rSelectSp.TSG1 << 4) | rSelectSp.TSG2;
    }

    return true;
}

long Window::findReg(const CommData &RecvData) {
    lBrNum = 0;
    lSpNum = 0;
    for (int i = 0; i < r1m.size(); i++) {
        if (r1m[i].BRP == RecvData.can1_bsp && r1m[i].TSG2 == (RecvData.bs1_bs2_1 & 0x0f)
                && r1m[i].TSG1 == ((RecvData.bs1_bs2_1 >> 4) & 0x0f))
        {
            lBrNum = 1;
            lSpNum = i + 1;
        }
        if (r1m[i].BRP == RecvData.can2_bsp && r1m[i].TSG2 == RecvData.bs1_bs2_2 & 0x0f
                && r1m[i].TSG1 == (RecvData.bs1_bs2_2 >> 4) & 0x0f)
        {
            rBrNum = 1;
            rSpNum = i + 1;
        }
        if (lBrNum != 0 && lSpNum != 0) // && rBrNum!=0 && rSpNum!=0)
        {
            qDebug() << "br and sp num" << (int)lBrNum << (int)lSpNum;
            return lBrNum << 24 | lSpNum << 16 | rBrNum << 8 | rSpNum;
        }
    }

    for (int i = 0; i < r5k.size(); i++) {
        if (r5k[i].BRP == RecvData.can1_bsp && r5k[i].TSG2 == RecvData.bs1_bs2_1 & 0x0f
                && r5k[i].TSG1 == (RecvData.bs1_bs2_1 >> 4) & 0x0f)
        {
            lBrNum = 2;
            lSpNum = i + 1;
        }
        if (r5k[i].BRP == RecvData.can2_bsp && r5k[i].TSG2 == RecvData.bs1_bs2_2 & 0x0f
                && r5k[i].TSG1 == (RecvData.bs1_bs2_2 >> 4) & 0x0f)
        {
            rBrNum = 2;
            rSpNum = i + 1;
        }
        if (lBrNum != 0 && lSpNum != 0 && rBrNum != 0 && rSpNum != 0)
            return lBrNum << 24 | lSpNum << 16 | rBrNum << 8 | rSpNum;
    }

    return 0;
}

void Window::convertRecvData() {
    findReg(*RecvCommData);
    lFormatComboBox->setCurrentIndex(lBrNum);
    lSjwSpinBox->setCurrentIndex(lSpNum);
    rFormatComboBox->setCurrentIndex(rBrNum);
    rSjwSpinBox->setCurrentIndex(rSpNum);

    lBs1SpinBox->setValue((RecvCommData->bs1_bs2_1 >> 4) & 0x0f);
    lBs2SpinBox->setValue(RecvCommData->bs1_bs2_1 & 0x0f);
    QString str1;
    str1.setNum(RecvCommData->can1_start_sid, 16);
    lSIdLineEdit->setText(str1);
    QString str2;
    str2.setNum(RecvCommData->can1_end_sid, 16);
    lEIdLineEdit->setText(str2);

    rBs1SpinBox->setValue((RecvCommData->bs1_bs2_2 >> 4) & 0x0f);
    rBs2SpinBox->setValue(RecvCommData->bs1_bs2_2 & 0x0f);
    str1.setNum(RecvCommData->can2_start_sid, 16);
    rSIdLineEdit->setText(str1);
    str2.setNum(RecvCommData->can2_end_sid, 16);
    rEIdLineEdit->setText(str2);

    qDebug() << "convertrecvdata" << str1  << str2 << (int)SendCommData->bs1_bs2_1;
}

void Window::onWriteButtonClicked() {
    if (!prepareSendData())
        return;

    qDebug() << "sendcommdata" << (int)SendCommData->can1_bsp
             << SendCommData->can1_start_sid << SendCommData->can1_end_sid
             << (int)(*(SendCommData->toByteArray()))[62];

    hidDev->Send(*(SendCommData->toByteArray()));

    QMessageBox msgBox;
    msgBox.setText(tr("The document has been modified."));
    msgBox.setInformativeText(tr("Do you want to save your changes?"));
    msgBox.setDetailedText(tr("Differences here..."));
    msgBox.setStandardButtons(QMessageBox::Save
                              | QMessageBox::Discard
                              | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int ret = msgBox.exec();
    switch (ret) {
    case QMessageBox::Save:
        qDebug() << "Save document!";
        break;
    case QMessageBox::Discard:
        qDebug() << "Discard changes!";
        break;
    case QMessageBox::Cancel:
        qDebug() << "Close document!";
        break;
    }

}

void Window::prepareReadData() {
    SendCommData->num = 0;
    SendCommData->flag = 0x80;
}

void Window::onReadButtonClicked() {
    prepareReadData();
    hidDev->Send(*(SendCommData->toByteArray()));
    //QMessageBox::information(NULL, "Title", "Conten222222222t", QMessageBox::NoButton);
    QMessageBox msgBox;
    msgBox.setText(tr("The document has been modified."));
    msgBox.setStandardButtons(QMessageBox::Abort);
    msgBox.setStyleSheet("background:transparent;");
    msgBox.exec();
}

void Window::runs() {
    HANDLE h[2];
    DWORD dw;
    int a;

    if (hidDev->connState == 1) {
        h[0] = hidDev->ReadOverlapped.hEvent;
        h[1] = hidDev->WriteOverlapped.hEvent;
        dw = WaitForMultipleObjects(2, h, FALSE, INFINITE);
        qDebug() << "WaitForMultipleObjects";

        if (hidDev->connState == 1) {
            switch (dw - WAIT_OBJECT_0) {
            case 0:
                DWORD undefined;
                if (!GetOverlappedResult(hidDev->hReadHandle,
                                         &hidDev->ReadOverlapped,
                                         &undefined, false))
                {
                    qDebug() << "ReadOverlapped Error" << hidDev->connState;
                    hidDev->connState = 2;
                    break;
                }

                ResetEvent(hidDev->ReadOverlapped.hEvent);
                emit valueChanged(a);
                qDebug() << "read completed";
                break;

            case 1:
                ResetEvent(hidDev->WriteOverlapped.hEvent);
                qDebug() << "write completed";
                break;

            default:
                qDebug() << "default completed";
                break;
            }
        }
    }

}

void Window::closeEvent(QCloseEvent *event) {
    hidThreads->stop();
    qDebug() << "closeEvent0";
    SetEvent(hidDev->ReadOverlapped.hEvent);
    SetEvent(hidDev->WriteOverlapped.hEvent);
    CancelIo(hidDev->hReadHandle);
    CancelIo(hidDev->hWriteHandle);
    hidThreads->wait();
    qDebug() << "closeEvent";
    event->accept();
}

int Window::calcRegs() {
    int ret = 0;
    Reg *pReg;
    for (int BRP = 1; BRP < 100; BRP++)
        for (int TSG1 = 1; TSG1 <= 16; TSG1++)
            for (int TSG2 = 1; TSG2 <= 8; TSG2++)
            {
                int temp = TSG1 + TSG2 + 1;
                if (temp >= 8 && temp <= 25 && TSG1 > TSG2 && TSG2 >= 1) {
                    long tmpBR = fclk_main / (1 * BRP * (TSG1 + TSG2 + 1));

                    if (tmpBR > BR5K * (1 - BRBIAS) && tmpBR < BR5K * (1 + BRBIAS)) {
                        pReg = new Reg(BRP, TSG1, TSG2, tmpBR);
                        r5k.push_back(*pReg);

                        delete pReg;
                        pReg = NULL;
                    }
                    else if (tmpBR > BR1M * (1 - BRBIAS) && tmpBR < BR1M * (1 + BRBIAS)) {
                        pReg = new Reg(BRP, TSG1, TSG2, tmpBR);
                        r1m.push_back(*pReg);

                        delete pReg;
                        pReg = NULL;
                    }
                    else {}
                }
            }

    if (r1m.size() == 0 || r5k.size() == 0)
        ret = 1; // flag

    return ret;
}

HidIOEvent::HidIOEvent(Window *wd) {
    stopped = false;
    pWd = wd;
}

HidIOEvent::~HidIOEvent() {}

void HidIOEvent::run() {
    while (!stopped)
    {
        pWd->runs();
    }
}

void KeyLineEdit::keyPressEvent(QKeyEvent *event ) {
    switch (event->key()) {
    case Qt::Key_Up:
        keyUp();
        break;
    case Qt::Key_Down:
        keyDown();
        break;
    default:
        QLineEdit::keyPressEvent(event );
        break;
    }
}

void KeyLineEdit::keyDown() {
    if (textMemo.size() == 0)
        return;
    setText(textMemo[memoIdx++]);
    if (memoIdx >= textMemo.size())
        memoIdx = 0;
    qDebug() << "key down";
}

void KeyLineEdit::keyUp() {
    if (textMemo.size() == 0)
        return;
    setText(textMemo[memoIdx--]);
    if (memoIdx <= 0)
        memoIdx = textMemo.size() - 1;
    qDebug() << "key up";
}

void KeyLineEdit::enqueue() {
    textMemo.enqueue(text());
    memoIdx = 0;
    if (textMemo.size() < maxMemoNum)
        return;
    dequeue();
}

void KeyLineEdit::dequeue() {
    textMemo.dequeue();
}

QByteArray* CommData::toByteArray() {
    if (canByteArray != NULL)
        ;//return canByteArray;

    canByteArray = new QByteArray;
    canByteArray->append(QByteArray::fromRawData((char*)this, EPSIZE));
    qDebug() << "sizeof(CommData)" << (*canByteArray)[2];
    return canByteArray;
}

CommData::CommData() {
    canByteArray = NULL;
    for (int i = 0; i < EPSIZE - 14; i++) {
        reserved[i] = 0;
    }
}

CommData::CommData(QByteArray &byteArray) {
    CommData *aCommData = (CommData *)byteArray.data();
    num = aCommData->num;
    flag = aCommData->flag;
    can1_bsp = aCommData->can1_bsp;
    bs1_bs2_1 = aCommData->bs1_bs2_1;
    can1_start_sid = aCommData->can1_start_sid;
    can1_end_sid = aCommData->can1_end_sid;
    can2_bsp = aCommData->can2_bsp;
    bs1_bs2_2 = aCommData->bs1_bs2_2;
    can2_start_sid = aCommData->can2_start_sid;
    can2_end_sid = aCommData->can2_end_sid;

    for (int i = 0; i < EPSIZE - 14; i++) {
        reserved[i] = aCommData->reserved[i];
    }
}
