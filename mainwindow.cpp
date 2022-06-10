#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "absequipment.h"
#include "connections.h"
#include "abntframe.h"

#include <QObject>
#include <QModbusDataUnit>
#include <QModbusTcpClient>
#include <QModbusRtuSerialMaster>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QTcpSocket>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_status(new QLabel),
    m_console(new Console),
    m_settings(new SettingsDialog),
    m_conn(new Connections),
    m_serial(new QSerialPort(this)),
    m_abs(new AbsEquipment),
    m_abnt(new AbntFrame)
{
    m_ui->setupUi(this);
    m_console->setEnabled(false);

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    m_ui->actionABS->setEnabled(true);
    m_ui->statusBar->addWidget(m_status);

    m_modbusClient = new QModbusRtuSerialMaster(this);

    connect(m_modbusClient, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        showStatusMessage(m_modbusClient->errorString());
    });

    if (!m_modbusClient) {
        this->showStatusMessage(tr("Could not create Modbus master."));
    } else {
        connect(m_modbusClient, &QModbusClient::stateChanged,this, &MainWindow::onStateChanged);
    }

    initActionsConnections();

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);

    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(m_console, &Console::getData, this, &MainWindow::writeData);
    connect(m_abnt, &AbntFrame::getData, this, &MainWindow::writeData);
    connect(m_abs, &AbsEquipment::sendReadRequest, this, &MainWindow::on_sendReadRequest);
    connect(m_abs, &AbsEquipment::sendWriteRequest, this, &MainWindow::on_sendWriteRequest);
    connect(m_abs, &AbsEquipment::sendReadWriteRequest, this, &MainWindow::on_sendReadWriteRequest);
    connect(m_conn, &Connections::recvData, m_abs, &AbsEquipment::processModbusResponseTcp);
    connect(m_conn, &Connections::message, this, &MainWindow::showStatusMessage);
    connect(m_conn, &Connections::recvRaw, this, &MainWindow::onTcpRawData);
}

MainWindow::~MainWindow()
{
    if (m_modbusClient) {
        m_modbusClient->disconnectDevice();
        delete m_modbusClient;
    }

    delete m_conn;
    delete m_settings;
    delete m_ui;
    delete m_abs;
    delete m_abnt;
}


void MainWindow::openSerialPort()
{
    const SettingsDialog::Settings p = m_settings->settings();

    //! For modbus serial connection
    if (m_settings->isModbusSerial()) {
        if (m_modbusClient->state() != QModbusDevice::ConnectedState) {

            m_modbusClient->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
                                                   p.name);
            m_modbusClient->setConnectionParameter(QModbusDevice::SerialParityParameter,
                                                   p.parity);
            //        modbusClient->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
            //                                             m_settingsDialog->settings().baudRateEnum);
            m_modbusClient->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
                                                   QSerialPort::Baud9600);
            m_modbusClient->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
                                                   p.dataBits);
            m_modbusClient->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
                                                   p.stopBits);
            m_modbusClient->setTimeout(p.responseTime);

            m_modbusClient->setNumberOfRetries(p.numberOfRetries);

            if (!m_modbusClient->connectDevice()) {
                showStatusMessage("Connected failed : " + m_modbusClient->errorString());
            } else {

                m_ui->actionConnect->setEnabled(false);
                m_ui->actionDisconnect->setEnabled(true);
                m_ui->actionConfigure->setEnabled(false);
                m_ui->actionABS->setEnabled(true);

                showStatusMessage("Modbus Serial connected!");
            }
        }
    }

    //! For serial raw connection
    else {
        m_serial->setPortName(p.name);
        m_serial->setBaudRate(p.baudRate);
        m_serial->setDataBits(p.dataBits);
        m_serial->setParity(p.parity);
        m_serial->setStopBits(p.stopBits);
        m_serial->setFlowControl(p.flowControl);
        if (m_serial->open(QIODevice::ReadWrite)) {
            m_console->setEnabled(true);
            m_console->setLocalEchoEnabled(p.localEchoEnabled);
            m_console->setShowHexEnable(p.showHexEnable);
            m_console->setAddLineFeed(p.addLineFeed);
            m_ui->actionConnect->setEnabled(false);
            m_ui->actionDisconnect->setEnabled(true);
            m_ui->actionConfigure->setEnabled(false);
            m_ui->actionABS->setEnabled(true);
            showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                              .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                              .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        } else {
            QMessageBox::critical(this, tr("Error"), m_serial->errorString());

            showStatusMessage(tr("Open error"));
        }
    }
}

void MainWindow::closeSerialPort()
{

    if (m_modbusClient->state() == QModbusDevice::ConnectedState) {
        m_modbusClient->disconnectDevice();
        m_ui->actionConnect->setEnabled(true);
        m_ui->actionDisconnect->setEnabled(false);
        m_ui->actionConfigure->setEnabled(true);
        m_ui->actionABS->setEnabled(false);
    }

    if (m_serial->isOpen())
        m_serial->close();
    m_console->setEnabled(false);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    m_ui->actionABS->setEnabled(false);

    showStatusMessage(tr("Disconnected"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Simple Terminal"),
                       tr("The <b>Simple Terminal</b> example demonstrates how to "
                          "use the Qt Serial Port module in modern GUI applications "
                          "using Qt, with a menu bar, toolbars, and a status bar."));
}

void MainWindow::writeData(const QByteArray &data)
{
    if (m_conn->isConnected())
        m_conn->sendData(data);
    else
        m_serial->write(data);
}

void MainWindow::readData()
{
    //    timer->start(250);

    const QByteArray data = m_serial->readAll();
    m_console->putData(data);
}

void MainWindow::timerTimeout()
{
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections()
{
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
//    connect(m_ui->actionSocketConnect, &QAction::triggered, m_conn, &Connections::show);
    connect(m_ui->actionClear, &QAction::triggered, m_console, &Console::clear);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}

void MainWindow::onStateChanged(int state)
{
    if (state == QModbusDevice::UnconnectedState)
        showStatusMessage(tr("Modbus Connected"));
    else if (state == QModbusDevice::ConnectedState)
        showStatusMessage(tr("Modbus Disconnected"));
}

void MainWindow::on_sendReadRequest(const QModbusDataUnit &read, int serverAddress)
{
    if ((m_modbusClient) && (m_modbusClient->state() == QModbusDevice::ConnectedState)) {
        if (auto *reply = m_modbusClient->sendReadRequest(read,serverAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, m_abs, &AbsEquipment::processModbusResponse);
                connect(reply, &QModbusReply::errorOccurred, m_abs, &AbsEquipment::processModbusErrorResponse);
            }
            else
                delete reply; // broadcast replies return immediately
        } else {
            showStatusMessage(tr("Read error: ") + m_modbusClient->errorString());
        }
    }

    else if ((m_conn) && (m_conn->isConnected())) {
        m_conn->sendReadRequest(read,serverAddress);
    }
    else
        showStatusMessage("There is no connections");
}

void MainWindow::on_sendWriteRequest(const QModbusDataUnit &write, int serverAddress)
{
    if ((m_modbusClient) && (m_modbusClient->state() == QModbusDevice::ConnectedState)) {
        if (auto *reply = m_modbusClient->sendWriteRequest(write,serverAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, m_abs, &AbsEquipment::processModbusResponse);
                connect(reply, &QModbusReply::errorOccurred, m_abs, &AbsEquipment::processModbusErrorResponse);
            }
            else
                delete reply; // broadcast replies return immediately
        } else {
            showStatusMessage(tr("Read error: ") + m_modbusClient->errorString());
        }
    }

    else if ((m_conn) && (m_conn->isConnected())) {
        m_conn->sendWriteRequest(write,serverAddress);
    }
    else
        showStatusMessage("There is no connections");
}

void  MainWindow::on_sendReadWriteRequest(const QModbusDataUnit &read, const QModbusDataUnit &write,
                                          int serverAddress)
{
    if ((m_modbusClient) && (m_modbusClient->state() == QModbusDevice::ConnectedState)) {
        if (auto *reply = m_modbusClient->sendReadWriteRequest(read,write,serverAddress)) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, m_abs, &AbsEquipment::processModbusResponse);
                connect(reply, &QModbusReply::errorOccurred, m_abs, &AbsEquipment::processModbusErrorResponse);
            }
            else
                delete reply; // broadcast replies return immediately
        } else {
            showStatusMessage(tr("Read error: ") + m_modbusClient->errorString());
        }
    }
    else if ((m_conn) && (m_conn->isConnected())) {
        m_conn->sendReadWriteRequest(read,write,serverAddress);
    }
    else
        showStatusMessage("There is no connections");
}

QModbusDataUnit MainWindow::readRequest(
        QModbusDataUnit::RegisterType regType,int address, int size) const
{
    int startAddress = address;
    quint16 numberOfEntries = static_cast<quint16> (size);

    Q_ASSERT(startAddress >= 0 && startAddress < MAX_ADD_INT16);
    Q_ASSERT(numberOfEntries >= 0 && numberOfEntries < MAC_BUFF_SIZE);

    return QModbusDataUnit(regType, startAddress, numberOfEntries);
}

QModbusDataUnit MainWindow::writeRequest(
        QModbusDataUnit::RegisterType regType, int address, int size) const
{
    int startAddress = address;
    quint16 numberOfEntries = static_cast<quint16> (size);

    Q_ASSERT(startAddress >= 0 && startAddress < MAX_ADD_INT16);
    Q_ASSERT(numberOfEntries >= 0 && numberOfEntries < MAC_BUFF_SIZE);

    return QModbusDataUnit(regType, startAddress, numberOfEntries);
}

void  MainWindow::readReady()
{
    auto reply = qobject_cast<QModbusReply *>(sender());

    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        for (uint i = 0; i < unit.valueCount(); i++) {
            const QString entry = tr("Address: %1, Value: %2").arg(unit.startAddress() + i)
                    .arg(QString::number(unit.value(i),
                                         unit.registerType() <= QModbusDataUnit::Coils ? 10 : 16));
            qWarning() << "Entry " << entry;
            //            ui->readValue->addItem(entry);
        }
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        showStatusMessage(tr("Read response error: %1 (Mobus exception: 0x%2)").
                          arg(reply->errorString()).
                          arg(reply->rawResult().exceptionCode(), -1, 16));
    } else {
        showStatusMessage(tr("Read response error: %1 (code: 0x%2)").
                          arg(reply->errorString()).
                          arg(reply->error(), -1, 16));
    }

    reply->deleteLater();
}

void MainWindow::tcpReadReady()
{

}

void MainWindow::on_actionConsole_triggered()
{
    takeCentralWidget();
    setCentralWidget(m_console);
//    m_console->setEnabled(true);
}

void MainWindow::on_actionABS_triggered()
{
    takeCentralWidget();
    setCentralWidget(m_abs);
//    m_console->setEnabled(false);
}

void MainWindow::onTcpRawData(QByteArray data)
{
    if (m_console)
        m_console->putData(data);
}

void MainWindow::on_actionSocketConnect_triggered()
{
    takeCentralWidget();
    setCentralWidget(m_conn);
}

void MainWindow::on_actionCODI_triggered()
{
    takeCentralWidget();
    setCentralWidget(m_abnt);
}

void MainWindow::on_actionDesbloquear_ABS_triggered()
{
    m_abs->unlockWriteCommands();
}

