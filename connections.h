#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <QWidget>
#include <QModbusDataUnit>
#include <qabstractsocket.h>

namespace Ui {
class Connections;

}

class QTcpSocket;

class Connections : public QWidget
{
    Q_OBJECT

public:
    explicit Connections(QWidget *parent = nullptr);
    ~Connections();

    bool isConnected();

    void sendReadRequest(const QModbusDataUnit &read, int serverAddress);
    void sendWriteRequest(const QModbusDataUnit &write, int serverAddress);
    void sendReadWriteRequest(const QModbusDataUnit &read, const QModbusDataUnit &write,
                              int serverAddress);
private slots:
    void readReady();
    void connected();
    void disconnected();
    void displayError(QAbstractSocket::SocketError error);

    void on_enviarButton_clicked();

    void on_infoButton_clicked();

public slots:
    void sendData(QByteArray data);

    void calcCRC();

    void on_connectButton_clicked();

signals:
    void recvData(QModbusDataUnit data);
    void message(QString msg);
    void recvRaw(QByteArray data);

private:
    Ui::Connections *ui;
    QTcpSocket *socket = nullptr;
    QDataStream input;
    uint8_t buffer[256];
    uint8_t bufferSize;
    int lasStartAddress;
    uint8_t lastValueCount;
};

#endif // CONNECTIONS_H
