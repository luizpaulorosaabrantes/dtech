#include "connections.h"
#include "ui_connections.h"

extern "C" {
#include "crc.h"
}

#include <QTcpSocket>
#include <QtNetwork>
#include <QNetworkConfiguration>
#include <QDebug>
#include <QIcon>

Connections::Connections(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Connections),
    socket(new QTcpSocket(this))
{
//    QNetworkConfiguration config = 	QNetworkConfiguration();
//    config.setConnectTimeout(10000);
    ui->setupUi(this);

    input.setDevice(socket);
    input.setVersion(QDataStream::Qt_5_10);

    connect(socket, &QIODevice::readyRead, this, &Connections::readReady);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &Connections::displayError);
    connect(socket, &QTcpSocket::connected, this, &Connections::connected);
    connect(socket, &QTcpSocket::disconnected, this, &Connections::disconnected);

    ui->connectButton->setText("Offline!");
}

Connections::~Connections()
{
    if (socket->isOpen())
        socket->abort();

    delete socket;
    delete ui;
}

bool Connections::isConnected()
{
    return socket->state() == QAbstractSocket::ConnectedState;
}


void Connections::readReady()
{
   QByteArray bytes = socket->readAll();
   QModbusDataUnit data;

   bufferSize = 0;

   for(int i = 0; i < bytes.size() - 2; i++)
       buffer[bufferSize++] = static_cast<uint8_t>(bytes.at(i));

   calcCRC();

   if (buffer[0] == 200) {

       if (buffer[1] == 4) {
           data = QModbusDataUnit(QModbusDataUnit::InputRegisters,lasStartAddress, buffer[2]/2);

           for (uint8_t i = 0; i < buffer[2]/2 ; i++) {
               data.setValue(i, static_cast<quint16>(buffer[2 * i + 3] << 8) +
                       static_cast<quint16>(buffer[2 * i + 4]));

           }
           recvData(data);
       }
       else if (buffer[1] == 3) {
           data = QModbusDataUnit(QModbusDataUnit::HoldingRegisters,lasStartAddress, buffer[2]/2);

           for (uint8_t i = 0; i < buffer[2]/2 ; i++) {
               data.setValue(i, static_cast<quint16>(buffer[2 * i + 3] << 8) +
                       static_cast<quint16>(buffer[2 * i + 4]));
           }
           recvData(data);
       }
       else if (buffer[1] == 16) {
           data = QModbusDataUnit(QModbusDataUnit::HoldingRegisters,lasStartAddress, buffer[2]/2);
       }
       else if (buffer[1] == 23) {
           data = QModbusDataUnit(QModbusDataUnit::HoldingRegisters,lasStartAddress, buffer[2]/2);

           for (uint8_t i = 0; i < buffer[2]/2 ; i++) {
               data.setValue(i, static_cast<quint16>(buffer[2 * i + 3] << 8) +
                       static_cast<quint16>(buffer[2 * i + 4]));
           }
           recvData(data);
       }
       else {
           qWarning() << "Data type error: " << buffer[1];
       }
   }
   else {
       ui->textEdit->setText(bytes);
       recvRaw(bytes);
   }
}

void Connections::connected()
{
    ui->connectButton->setText("Online");
    ui->connectButton->setEnabled(true);

    QString msg  = QString("Connected: %1:%2! ").
            arg(ui->hostLineEdit->text()).arg(ui->portSpinBox->value());

    message(msg);
}

void Connections::disconnected()
{
    ui->connectButton->setText("Offline");
    ui->connectButton->setEnabled(true);

    QString msg = QString("Disconnected!");

    message(msg);
}

void Connections::displayError(QAbstractSocket::SocketError error)
{
    if (error == QAbstractSocket::ConnectionRefusedError){
        ui->connectButton->setText("Offline");
        message("Timeout!");
    }

    ui->connectButton->setEnabled(true);
}

void Connections::sendData(QByteArray data)
{
    if (!isConnected())
        return;

    socket->write(data);
    socket->flush();
}

void Connections::sendReadRequest(const QModbusDataUnit &read, int serverAddress)
{
    if (!isConnected())
        return;

    lasStartAddress = read.startAddress();
    lastValueCount = static_cast<uint8_t>(read.valueCount());

    bufferSize = 0;
    buffer[bufferSize++] = static_cast<uint8_t>(serverAddress);
    if (read.registerType() == QModbusDataUnit::InputRegisters)
        buffer[bufferSize++] = 4;
    else if (read.registerType() == QModbusDataUnit::HoldingRegisters)
        buffer[bufferSize++] = 3;
    else
        return;
    buffer[bufferSize++] = static_cast<uint8_t>(read.startAddress() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(read.startAddress() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(read.valueCount() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(read.valueCount() & 0xFF);

    calcCRC();

    QByteArray array = QByteArray();
    for (int i = 0; i < bufferSize; i++)
        array.append(static_cast<char>(buffer[i]));

    socket->write(array);
    socket->flush();
}

void Connections::sendWriteRequest(const QModbusDataUnit &write, int serverAddress)
{
    if (!isConnected())
        return;

    lasStartAddress = write.startAddress();
    lastValueCount = static_cast<uint8_t>(write.valueCount());

    bufferSize = 0;
    buffer[bufferSize++] = static_cast<uint8_t>(serverAddress);
    buffer[bufferSize++] = 16;
    buffer[bufferSize++] = static_cast<uint8_t>(write.startAddress() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(write.startAddress() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(write.valueCount() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(write.valueCount() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(2 * write.valueCount());

    for (int i = 0; i < static_cast<uint8_t>(write.valueCount()); i++) {
        buffer[bufferSize++] = static_cast<uint8_t>(write.value(i) >> 8);
        buffer[bufferSize++] = static_cast<uint8_t>(write.value(i) & 0xFF);
    }

    calcCRC();

    QByteArray array = QByteArray();
    for (int i = 0; i < bufferSize; i++)
        array.append(static_cast<char>(buffer[i]));

    socket->write(array);
    socket->flush();
}

void Connections::sendReadWriteRequest(const QModbusDataUnit &read, const QModbusDataUnit &write, int serverAddress)
{
    if (!isConnected())
        return;

    lasStartAddress = read.startAddress();
    lastValueCount = static_cast<uint8_t>(read.valueCount());

    bufferSize = 0;
    buffer[bufferSize++] = static_cast<uint8_t>(serverAddress);
    buffer[bufferSize++] = 23;
    buffer[bufferSize++] = static_cast<uint8_t>(read.startAddress() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(read.startAddress() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(read.valueCount() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(read.valueCount() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(write.startAddress() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(write.startAddress() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(write.valueCount() >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(write.valueCount() & 0xFF);
    buffer[bufferSize++] = static_cast<uint8_t>(2 * write.valueCount());

    for (int i = 0; i < static_cast<uint8_t>(write.valueCount()); i++) {
        buffer[bufferSize++] = static_cast<uint8_t>(write.value(i) >> 8);
        buffer[bufferSize++] = static_cast<uint8_t>(write.value(i) & 0xFF);
    }

    calcCRC();

    QByteArray array = QByteArray();
    for (int i = 0; i < bufferSize; i++)
        array.append(static_cast<char>(buffer[i]));

    socket->write(array);
    socket->flush();
}

void Connections::calcCRC()
{
    uint16_t crc = CRC16(buffer, bufferSize);

    buffer[bufferSize++] = static_cast<uint8_t>(crc >> 8);
    buffer[bufferSize++] = static_cast<uint8_t>(crc & 0xFF);
}

void Connections::on_connectButton_clicked()
{
    if (isConnected()) {
        qWarning() << "Disconnecting";
        socket->abort();
        ui->connectButton->setText("Offline");
        message("");
    }

    else {
        qWarning() << "Connecting";
        socket->abort();
        socket->connectToHost(ui->hostLineEdit->text(),
                              static_cast<quint16>(ui->portSpinBox->value()));
        ui->connectButton->setText("...");
        ui->connectButton->setEnabled(false);
    }
}

void Connections::on_enviarButton_clicked()
{
//    QByteArray byteArray = ui->sendLineEdit->text().toUtf8();

   if (isConnected()) {
       socket->write(ui->sendLineEdit->text().toUtf8());
       socket->write("\n");
       socket->flush();
   }
}

void Connections::on_infoButton_clicked()
{
    if (isConnected()) {
        socket->write("extcmd=getinfo\r\n");
        socket->flush();

        qWarning() << "Enviou!!";
    }
}
