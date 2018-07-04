/****************************************************************************
**
**  PainlessMesh MQTT client
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

void MainWindow::loopMeshNodes(QJsonArray a, int iDepth)
{
    qDebug() << a;
    for(const QJsonValue& val: a) {
        QJsonObject loopObj = val.toObject();
        int nodeId = loopObj["nodeId"].toInt();
        _listNodes << QString::number(nodeId);
        QJsonArray array = loopObj["subs"].toArray();
        qDebug() << "[" << iDepth << "] nodeId: "   << nodeId;
        qDebug() << "[" << iDepth << "] subs  : "   << array;
        if (array.count()) loopMeshNodes(array, iDepth+1);
        }
}

void MainWindow::findNodes(QString s)
{
    QStringList sl = s.split(": ");
    if (sl.count()>1 && sl.at(2).startsWith("[{")) {
            QString sJson = sl.at(2);
            qDebug() << "---------------------------------";
            QJsonDocument doc = QJsonDocument::fromJson(sJson.toUtf8());
            qDebug() << doc.toJson(QJsonDocument::Compact);
            _listNodes.clear();
            loopMeshNodes(doc.array(), 1);
            int iCount = _listNodes.count();
            if (iCount) {
                QString s = "Got Nodes:" + QString::number(iCount) + " > "+_listNodes.join(",")+"\n";
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
    QString sState;
    switch (m_client->state()) {
        case QMqttClient::Disconnected: sState="Disconnected";break;
        case QMqttClient::Connecting: sState="Connecting";break;
        case QMqttClient::Connected: sState="Connected";
        }
    const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(": State Change")
                    //+ QString::number(m_client->state())
                    + ": "+sState
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
