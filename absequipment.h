#ifndef ABSEQUIPMENT_H
#define ABSEQUIPMENT_H

#include <QWidget>
#include <QModbusDataUnit>
#include <QModbusDevice>
#include <QSqlDatabase>
#include <QTimer>

#define MAX_ADD_INT16       65500
#define MAC_BUFF_SIZE       32
#define ABS_SSU_ADDRESS     200

#define ABS_CLOCK_ADDR      64015
#define ABS_CLOCK_SIZE      6
#define ABS_DEMAND_ADDR     64100
#define ABS_DEMAND_SIZE     18
#define ABS_HIST_ADDR       64200
#define ABS_HIST_SIZE       5
#define ABS_SELECT_REGISTER_ADDRESS     64511
#define ABS_DEMAND_DATA_ADDRESS         64205
#define ABS_DEMAND_DATA_SIZE            10
#define ABS_ORDER_ASCEND    0
#define ABS_ORDER_DESCEND   1
#define ABS_EREASE_MEMORY_ADDRESS 64509
#define ABS_CLOCK_SET_ADDRESS   64502
#define ABS_CLOCK_SET_SIZE      10

QT_BEGIN_NAMESPACE

namespace Ui {
class AbsEquipment;
}

QT_END_NAMESPACE

class QSqlTableModel;
class QTimer;

class AbsEquipment : public QWidget
{
    Q_OBJECT

public:
    explicit AbsEquipment(QWidget *parent = nullptr);
    ~AbsEquipment();

public slots:
    void processModbusResponse();
    void processModbusErrorResponse(QModbusDevice::Error error);
    void processModbusResponseTcp(QModbusDataUnit data);
    void unlockWriteCommands();

private slots:
    void on_clockPushButton_clicked();
    void on_actutalPushButton_clicked();
    void on_downloadButton_clicked();

    void requestNewDemand(void);
    void requestNewPulse(void);

    void on_historicPushButton_clicked();

    void on_exportButton_clicked();

    void on_ereasePushButton_clicked();

    void on_updateClockButton_clicked();

    void on_orderComboBox_activated(int index);

    void lockWriteCommands();

signals:
    void sendReadRequest(const QModbusDataUnit &read, int serverAddress);
    void sendWriteRequest(const QModbusDataUnit &write, int serverAddress);
    void sendReadWriteRequest(const QModbusDataUnit &read, const QModbusDataUnit &write,
                              int serverAddress);

private:
    void processDemand(const QModbusDataUnit unit);
    void processDemandOnline(const QModbusDataUnit unit);
    void processHistoric(const QModbusDataUnit unit);
    void processClock(const QModbusDataUnit unit);
    int getPercentage(bool isAcend);
    float calcPowerFactor(int a, int r);

private:
    Ui::AbsEquipment *ui;
    QSqlTableModel *model;
    QSqlDatabase db;
    int indiceProxBloco;
    int primeiraDemanda;
    int ultimaDemanda;
    bool isDownloading;
    int startRegister;
    int finishRegister;
    bool isDesdendOrder;
    int registersToRead;
    bool isCheckingPulse;
    QTimer *timerLockWriteCmds = nullptr;
};

#endif // ABSEQUIPMENT_H
