/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtWidgets/QMessageBox>

void MainWindow::findNodes(QString s)
{
    QStringList sl = s.split(": ");
    if (sl.count()>1 && sl.at(2).startsWith("[{")) {
            QString sJson = sl.at(2);
            sJson = sJson.mid(1,sJson.length()-3); //remove first []
            //qDebug() << "\n" << sJson.toUtf8();
            qDebug() << "---------------------------------";

               QJsonDocument doc = QJsonDocument::fromJson(sJson.toUtf8());

               //QJsonDocument doc = QJsonDocument::fromJson(
               //            "{\"nodeId\":885916069,\"subs\":[{\"nodeId\":885914535,\"subs\":[{\"nodeId\":885914562,\"subs\":[]}]}]}" );


               qDebug() << doc.toJson(QJsonDocument::Compact);
               // This part you have covered
               QJsonObject jObj = doc.object();
               _listNodes.clear();
               _listNodes << QString::number(jObj["nodeId"].toInt());
               //qDebug() << "nodeId:" << jObj["nodeId"].toInt();
               //qDebug() << "subs:" << jObj["subs"].toArray();

            QJsonObject jsonObject = doc.object();
            QJsonArray array = jsonObject["subs"].toArray();

            int idx = 0;
            for(const QJsonValue& val: array) {
                QJsonObject loopObj = val.toObject();
                _listNodes << QString::number(loopObj["nodeId"].toInt());
                //qDebug() << "[" << idx << "] nodeId   : " << loopObj["nodeId"].toInt();
                //qDebug() << "[" << idx << "] subs     : " << loopObj["subs"].toArray();
                ++idx;
                }
            qDebug() << "Nodes:" <<_listNodes;
            if (_listNodes.count()) {
                QString s;
                s += "Got Nodes:" + QString::number(_listNodes.count())+" > "+_listNodes.join(",")+"\n";
                ui->editLog->moveCursor(QTextCursor::End);
                ui->editLog->insertPlainText(s);
                ui->editLog->moveCursor(QTextCursor::End);
                }
            }


}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_client = new QMqttClient(this);
    m_client->setHostname(ui->lineEditHost->text());
    m_client->setPort(ui->spinBoxPort->value());

    connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::updateLogStateChange);
    connect(m_client, &QMqttClient::disconnected, this, &MainWindow::brokerDisconnected);

    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(" Received Topic: ")
                    + topic.name()
                    + QLatin1String(" Message: ")
                    + message
                    + QLatin1Char('\n');
        ui->editLog->moveCursor(QTextCursor::End);
        ui->editLog->insertPlainText(content);
        ui->editLog->moveCursor(QTextCursor::End);
        findNodes(content);
    });

    connect(m_client, &QMqttClient::pingResponseReceived, this, [this]() {
        ui->buttonPing->setEnabled(true);
        const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(" PingResponse")
                    + QLatin1Char('\n');
        ui->editLog->moveCursor(QTextCursor::End);
        ui->editLog->insertPlainText(content);
        ui->editLog->moveCursor(QTextCursor::End);
    });

    connect(ui->lineEditHost, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(ui->spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setClientPort);
    updateLogStateChange();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_buttonConnect_clicked()
{
    if (m_client->state() == QMqttClient::Disconnected) {
        ui->lineEditHost->setEnabled(false);
        ui->spinBoxPort->setEnabled(false);
        ui->buttonConnect->setText(tr("Disconnect"));
        m_client->connectToHost();
    } else {
        ui->lineEditHost->setEnabled(true);
        ui->spinBoxPort->setEnabled(true);
        ui->buttonConnect->setText(tr("Connect"));
        m_client->disconnectFromHost();
    }
}

void MainWindow::on_buttonQuit_clicked()
{
    QApplication::quit();
}

void MainWindow::updateLogStateChange()
{
    const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(": State Change")
                    + QString::number(m_client->state())
                    + QLatin1Char('\n');
    ui->editLog->moveCursor(QTextCursor::End);
    ui->editLog->insertPlainText(content);
    ui->editLog->moveCursor(QTextCursor::End);
}

void MainWindow::brokerDisconnected()
{
    ui->lineEditHost->setEnabled(true);
    ui->spinBoxPort->setEnabled(true);
    ui->buttonConnect->setText(tr("Connect"));
}

void MainWindow::setClientPort(int p)
{
    m_client->setPort(p);
}

void MainWindow::on_buttonPublish_clicked()
{
    QString sTopic = ui->comboBoxTopic->currentText();
    QString sMessage = ui->comboBoxMessage->currentText();
    if (m_client->publish(sTopic, sMessage.toUtf8()) == -1)
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
}

void MainWindow::on_buttonSubscribe_clicked()
{
    QString sTopic = ui->comboBoxTopic->currentText();
    auto subscription = m_client->subscribe(sTopic);
    if (!subscription) {
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not subscribe. Is there a valid connection?"));
        return;
    }
}

void MainWindow::on_buttonPing_clicked()
{
    ui->buttonPing->setEnabled(false);
    m_client->requestPing();
}

void MainWindow::on_buttonClear_clicked()
{
    ui->editLog->clear();
}
