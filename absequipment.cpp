#include "absequipment.h"
#include "ui_absequipment.h"
#include "settingsdialog.h"
#include "database.h"

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QModbusReply>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QDebug>
#include <QTimer>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QtMath>

AbsEquipment::AbsEquipment(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AbsEquipment)
{
    ui->setupUi(this);

    ui->downloadButton->setEnabled(false);

    ui->orderComboBox->addItem("Descendente", ABS_ORDER_DESCEND);
    ui->orderComboBox->addItem("Ascendente", ABS_ORDER_ASCEND);

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(100);
    ui->demandProgress->setValue(0);
    ui->demandProgress->setMinimum(0);
    ui->demandProgress->setMaximum(900);

    ui->ereasePushButton->setEnabled(false);
    ui->updateClockButton->setEnabled(false);

    ui->activeLcdNumber->setPalette(Qt::black);
    ui->reactiveLcdNumber->setPalette(Qt::darkGray);
    ui->lastActiveLcdNumber->setPalette(Qt::black);
    ui->lastReactiveLcdNumber->setPalette(Qt::darkGray);

    registersToRead = 0;
    indiceProxBloco = 0;
    isDownloading = false;
    startRegister = 0;
    isDesdendOrder = false;
    isCheckingPulse = false;
}

AbsEquipment::~AbsEquipment()
{

    indiceProxBloco = 0;
    isDownloading = false;
    startRegister = 0;
    isDesdendOrder = false;

    delete ui;
}

void AbsEquipment::processModbusResponse()
{
    auto reply = qobject_cast<QModbusReply *>(sender());

    if (!reply) {
        qWarning() << "Failed to read QModbusReply";
        return;
    }

    if (reply->error() == QModbusDevice::ProtocolError){
        qWarning() << "Protocol error!";
        return;
    }

    if (reply->error() == QModbusDevice::NoError) {

        const QModbusDataUnit unit = reply->result();

        if ((ABS_CLOCK_ADDR == reply->result().startAddress())
                && (ABS_CLOCK_SIZE ==  unit.valueCount())) {
            processClock(unit);
        }

        if ((ABS_HIST_ADDR == reply->result().startAddress())
                && (ABS_HIST_SIZE ==  unit.valueCount())) {
            processHistoric(unit);
        }

        if ((ABS_DEMAND_ADDR == reply->result().startAddress())
                && (ABS_DEMAND_SIZE ==  unit.valueCount())) {

            processDemandOnline(unit);

            if (isCheckingPulse)
                QTimer::singleShot(100, this, SLOT(requestNewPulse()));

        }

        if ((ABS_DEMAND_DATA_ADDRESS == reply->result().startAddress())
                && (ABS_DEMAND_DATA_SIZE ==  unit.valueCount())) {
           processDemand(unit);
        }
    }

    reply->deleteLater();
}

void AbsEquipment::processModbusErrorResponse(QModbusDevice::Error error)
{
    auto reply = qobject_cast<QModbusReply *>(sender());

    qWarning() << "error: " << error;

    reply->deleteLater();

}

void AbsEquipment::processModbusResponseTcp(QModbusDataUnit data)
{
//    qWarning() << "processModbusResponseTcp";
//    qWarning() << "start address " << data.startAddress();
//    qWarning() << "value count " << data.valueCount();

    if ((ABS_CLOCK_ADDR == data.startAddress())
            && (ABS_CLOCK_SIZE ==  data.valueCount())) {
        processClock(data);
    }

    if ((ABS_HIST_ADDR == data.startAddress())
            && (ABS_HIST_SIZE ==  data.valueCount())) {
        processHistoric(data);
    }

    if ((ABS_DEMAND_ADDR == data.startAddress())
            && (ABS_DEMAND_SIZE ==  data.valueCount())) {
        processDemandOnline(data);
        if (isCheckingPulse)
            QTimer::singleShot(10, this, SLOT(requestNewPulse()));
    }

    if ((ABS_DEMAND_DATA_ADDRESS == data.startAddress())
            && (ABS_DEMAND_DATA_SIZE ==  data.valueCount())) {
       processDemand(data);
    }
}

void AbsEquipment::on_clockPushButton_clicked()
{
    emit sendReadRequest(QModbusDataUnit(
                             QModbusDataUnit::InputRegisters,
                             ABS_CLOCK_ADDR, ABS_CLOCK_SIZE),ABS_SSU_ADDRESS);

}

void AbsEquipment::on_actutalPushButton_clicked()
{
    if (isCheckingPulse == false) {

        emit sendReadRequest(QModbusDataUnit(
                                 QModbusDataUnit::InputRegisters,
                                 ABS_DEMAND_ADDR, ABS_DEMAND_SIZE),ABS_SSU_ADDRESS);
        isCheckingPulse = true;
        ui->actutalPushButton->setText("...");
    }
    else {
        isCheckingPulse = false;
        ui->actutalPushButton->setText("Integração");
    }
}


void AbsEquipment::on_downloadButton_clicked()
{
    if (isDownloading == false) {

        if (indiceProxBloco == 0) {
            QMessageBox msgBox;
            msgBox.setText("Numero do bloco inválido!");
            msgBox.exec();
            return;
        }

        ui->downloadButton->setText("...");
        isDownloading = true;

        if (ui->orderComboBox->currentData().toInt() == ABS_ORDER_DESCEND) {
//            startRegister = ultimaDemanda;
//            finishRegister = primeiraDemanda;
            startRegister = ui->initialSpinBox->value();
            finishRegister =  ui->finalSspinBox->value();
            registersToRead = startRegister - finishRegister;
            isDesdendOrder = true;
        }
        else {
//            startRegister = primeiraDemanda;
//            finishRegister = ultimaDemanda;
            startRegister = ui->initialSpinBox->value();
            finishRegister = ui->finalSspinBox->value();
            registersToRead = finishRegister - startRegister;
            isDesdendOrder = false;
        }

        ui->progressBar->setValue(0);

        requestNewDemand();
    }
    else {
        db.commit();
        ui->downloadButton->setText("Download");
        isDownloading = false;
    }
}

void AbsEquipment::requestNewDemand(void)
{
    QVector<quint16> writeRegisters = QVector<quint16>(1);
    writeRegisters[0] = static_cast<quint16>(startRegister);

    QModbusDataUnit read = QModbusDataUnit(
                QModbusDataUnit::HoldingRegisters,ABS_DEMAND_DATA_ADDRESS, ABS_DEMAND_DATA_SIZE);
    QModbusDataUnit write = QModbusDataUnit(
                QModbusDataUnit::HoldingRegisters,ABS_SELECT_REGISTER_ADDRESS, 1);
    write.setValue(0,writeRegisters[0]);

    emit sendReadWriteRequest(read,write,ABS_SSU_ADDRESS);
}

void AbsEquipment::processDemand(const QModbusDataUnit unit)
{
    int block = unit.value(0);
    QTime time = QTime(unit.value(3),unit.value(2),unit.value(1));
    QDate date = QDate(unit.value(6) + 2000,unit.value(5),unit.value(4));
    int abntBits = unit.value(7);
    int activePulse = unit.value(8);
    int reactivePulse = unit.value(9);
    double powerFactor = static_cast<double>(calcPowerFactor(activePulse,reactivePulse));
    QString obs = QString("");
    QString sPosto = QString("");
    QString sTarifa = QString("");
    QString sUFER = QString("");
    QString sHora = QString("");
    QString sFatura = QString("");
    QString sBytes = QString("");
    QString sQ = QString("");

    QString sTime = time.toString("HH:mm:ss");
    QString sDate = date.toString("yyyy-MM-dd");


    sBytes.asprintf("{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x} ",
                              unit.value(0),unit.value(1),unit.value(2),unit.value(3),unit.value(4),
                              unit.value(5),unit.value(6),unit.value(7),unit.value(8),unit.value(9));

    /*  ABNT BITS
     *  |   HIGH    |    LOW    |
     *  | 5432 1098 | 7654 3210 |
     *    Posto||||   |||| ||||
     *    Ponta0001   |||| 0001 - Check sum erro
     *    F Po.0010   |||| 0100 - Falta de energia
     *    Rese.1000   |||| 1000 - Retorno de energia
     *    00|| Azul   |||| 1101 - Inicio comunicação com medidor
     *    01|| Verde  |||| 1100 - Perda de comunicação medidor
     *    10|| Irrig  |||| - Reposição de demanda (fatura)
     *    11|| Outra  |||- Indicador de intervalo reativo
     *     | NU       ||- UFER capacitivo
     *    | Tarifa.A  |- UFER indutivo
     */

    if ((abntBits & 0x0F) == 0x01)
        obs = QString("Erro check");
    else if ((abntBits & 0x0F) == 0x04)
        obs = QString("Falta energia");
    else if ((abntBits & 0x0F) == 0x08)
        obs = QString("Retorno energia");
    else if ((abntBits & 0x0F) == 0x0D)
        obs = QString("Retorno pulso");
    else if ((abntBits & 0x0F) == 0x0C)
        obs = QString("Falta Pulso");
//    else if ((abntBits & 0xFFFF) == 0x0000) // Bug fix
//        obs = QString("Falta energia");
    else {



//        if (((abntBits & 0x0F00) >> 8) == 0x01) {
//           sPosto = QString("PONTA");
//        }
//        else if (((abntBits & 0x0F00) >> 8) == 0x02) {
//            sPosto = QString("FORA_PONTA");
//        }
//        else if (((abntBits & 0x0F00) >> 8) == 0x08) {
//            sPosto = QString("RESERVADO");
//         }

//         qWarning() << "UFER: " << QString::number(abntBits, 16);

        if (((abntBits & 0x0300) >> 8) == 0x01) {
            sPosto = QString("PONTA");
        }
        else if (((abntBits & 0x0300) >> 8) == 0x02) {
            sPosto = QString("FORA_PONTA");
        }
        else if (((abntBits & 0x0300) >> 8) == 0x00) {
            sPosto = QString("RESERVADO");
        }
        else if (((abntBits & 0x0300) >> 8) == 0x00) {
            sPosto = QString("4 Posto");
        }

        if (((abntBits & 0x3000) >> 12) == 0x00) {
            sQ = QString("Q1");
        }
        else if (((abntBits & 0x3000) >> 12) == 0x01) {
            sQ = QString("Q4");
        }
        else if (((abntBits & 0x3000) >> 12) == 0x02) {
            sQ = QString("Q2");
        }
        else if (((abntBits & 0x3000) >> 12) == 0x03) {
            sQ = QString("Q3");
        }

        if (((abntBits & 0x80) > 0))
            sUFER = QString("IND");
        else
            sUFER = QString("CAP");

        if (((abntBits & 0x20) > 0))
            sHora = QString("X");
        else
            sHora = QString(" ");

        if (((abntBits & 0x10) > 0))
            sFatura = QString("X");
        else
            sFatura = QString(" ");
    }



    QSqlQuery query(db);
    QString sqlQuery = QString("insert into demanda values(%1, '%2', '%3', "
                               "'%4', %5, %6, %7, '%8', '%9', '%10', '%11', '%12', '%13')").arg(block).arg(sDate).
            arg(sTime).arg(sPosto).arg(activePulse).arg(reactivePulse).arg(powerFactor).arg(sUFER).arg(sQ).
            arg(sHora).arg(sFatura).arg(obs).arg(sBytes);
    query.exec(sqlQuery);

    model->select();
    ui->demandTableView->resizeColumnsToContents();
    ui->demandTableView->update();

    if (isDownloading)
    {
        if (startRegister == finishRegister) {
            isDownloading = false;
            ui->progressBar->setValue(100);
            ui->downloadButton->setText("Download");
        }
        else {

            if (isDesdendOrder) {
                ui->progressBar->setValue(getPercentage(false));
                startRegister--;
                ui->initialSpinBox->setValue(startRegister);
            }
            else {
                ui->progressBar->setValue(getPercentage(true));
                startRegister++;
                ui->initialSpinBox->setValue(startRegister);
            }

            QTimer::singleShot(50, this, SLOT(requestNewDemand()));
        }
    }
}

void AbsEquipment::processDemandOnline(const QModbusDataUnit unit)
{
    int timer = unit.value(0);
    int protocol = unit.value(4);
    int nextD = unit.value(5);
    int active = unit.value(7);
    int reactive = unit.value(8);
    float fp = calcPowerFactor(active,reactive);
    QString ufer;
    QString posto;
    QString hora;
    QString quadrante = "";

    if (protocol == 8) {
        if ((unit.value(6) & 0x40)> 0){
            ufer = "UFER CAP";
            ui->fpLcdNumber->setPalette(Qt::blue);
        }
        else {
            ufer = "UFER IND";
            ui->fpLcdNumber->setPalette(Qt::green);
        }

        if (((unit.value(6) & 0x0F00) >> 8) == 0x01) {
           posto = QString("PONTA");
        }
        else if (((unit.value(6) & 0x0F00) >> 8) == 0x02) {
            posto = QString("FORA_PONTA");
        }
        else if (((unit.value(6) & 0x0F00) >> 8) == 0x08) {
            posto = QString("RESERVADO");
         }
        else {
            posto = "";
        }
    }

    else if (protocol == 9) {

        if ((unit.value(6) & 0x40)> 0){
            ufer = "UFER CAP";
            ui->fpLcdNumber->setPalette(Qt::blue);
        }
        else {
            ufer = "UFER IND";
            ui->fpLcdNumber->setPalette(Qt::green);
        }


        if (((unit.value(6) & 0x0300) >> 8) == 0x01) {
           posto = QString("PONTA");
        }
        else if (((unit.value(6) & 0x0300) >> 8) == 0x02) {
            posto = QString("FORA_PONTA");
        }
        else if (((unit.value(6) & 0x0300) >> 8) == 0x03) {
            posto = QString("4 POSTO");
         }
        else {
            posto = "RESERVADO";
        }

        if (((unit.value(6) & 0x3000) >> 12) == 0x00) {
           quadrante = QString("Q1");
        }

        else if (((unit.value(6) & 0x3000) >> 12) == 0x01) {
            quadrante = QString("Q4");
         }

        else if (((unit.value(6) & 0x3000) >> 12) == 0x02) {
            quadrante = QString("Q2");
         }

        else if (((unit.value(6) & 0x3000) >> 12) == 0x03) {
            quadrante = QString("Q3");
         }
    }


    if ((unit.value(6) & 0x20) > 0) {
       hora = QString("1");
    }
    else  {
        hora = QString("0");
    }

    QString lastUfer;
    QString lastPosto;
    QString lastHora;

    if ((unit.value(15) & 0x40)> 0){
        lastUfer = "CAP";
        ui->lastFpLcdNumber->setPalette(Qt::blue);
    }
    else {
        lastUfer = "IND";
        ui->lastFpLcdNumber->setPalette(Qt::green);
    }

    if (((unit.value(15) & 0x0F00) >> 8) == 0x01) {
       lastPosto = QString("PONTA");
    }
    else if (((unit.value(15) & 0x0F00) >> 8) == 0x02) {
        lastPosto = QString("FORA_PONTA");
    }
    else if (((unit.value(15) & 0x0F00) >> 8) == 0x08) {
        lastPosto = QString("RESERVADO");
     }
    else {
        lastPosto = "";
    }

    if ((unit.value(15) & 0x20) > 0) {
       lastHora = QString("1");
    }
    else  {
        lastHora = QString("0");
    }

    QString stringInt = QString("Timer:%1, Protocolo:%2, Ufer:%3, Posto:%4, Hora: %5, Q:%6").
            arg(timer).arg(protocol == 8 ? "Normal" : "Estenddido").arg(ufer).arg(posto).arg(hora).arg(quadrante);

    QString stringLast = QString("Ufer:%3, Posto:%4, Hora: %5 ").
            arg(lastUfer).arg(lastPosto).arg(lastHora);


    ui->activeLcdNumber->display(active);
    ui->reactiveLcdNumber->display(reactive);
    ui->fpLcdNumber->display(static_cast<double>(fp));
    ui->demandProgress->setValue(900 - nextD);
    ui->historicLineEdit->setText(stringInt);
    ui->lastDemandLineEdit->setText(stringLast);
    ui->lastActiveLcdNumber->display(unit.value(16));
    ui->lastReactiveLcdNumber->display(unit.value(17));
    ui->lastFpLcdNumber->display(static_cast<double>(calcPowerFactor(unit.value(16),unit.value(17))));

    qWarning() <<  QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss") << active << ":"
                << reactive << " (" << nextD << ") " << quadrante;
}

void AbsEquipment::processHistoric(const QModbusDataUnit unit)
{
    indiceProxBloco = unit.value(4);
    primeiraDemanda = 0;
    ultimaDemanda = indiceProxBloco -1;

    QString string = QString("TamanhoBloco:%1, NumBloco:%2, ProximoBloco:%3, Registros:%4").
            arg(unit.value(0)).arg(unit.value(1)).arg(unit.value(2)).arg(indiceProxBloco);

    ui->lineEdit->setText(string);

    if (ui->orderComboBox->currentData().toInt() == ABS_ORDER_DESCEND){
        ui->initialSpinBox->setValue(ultimaDemanda);
        ui->finalSspinBox->setValue(primeiraDemanda);
    }
    else{
        ui->initialSpinBox->setValue(primeiraDemanda);
        ui->finalSspinBox->setValue(ultimaDemanda);
    }
}

void AbsEquipment::processClock(const QModbusDataUnit unit)
{
    QTime time = QTime(unit.value(2),unit.value(1),unit.value(0));
    QDate date = QDate(unit.value(5) + 2000,unit.value(4),unit.value(3));
    QDateTime dateTime = QDateTime(date,time);
    QString s = dateTime.toString("HH:mm:ss dd-MM-yy");

    ui->clockDateTimeEdit->setDisplayFormat("HH:mm:ss dd-MM-yy");
    ui->clockDateTimeEdit->setDate(date);
    ui->clockDateTimeEdit->setTime(time);
}

void AbsEquipment::requestNewPulse(void)
{
    emit sendReadRequest(QModbusDataUnit(
                             QModbusDataUnit::InputRegisters,
                             ABS_DEMAND_ADDR, ABS_DEMAND_SIZE),ABS_SSU_ADDRESS);
}

int AbsEquipment::getPercentage(bool isAcend)
{
    int ret;
    float start = static_cast<float>(startRegister);
    float regis = static_cast<float>(registersToRead);

    if (isAcend) {
        ret =  static_cast<int>((1 - ((regis-start)/regis) ) * 100);
    }
    else {
        ret = static_cast<int>((1 - (start/regis)) * 100);
    }
    return ret;
}

float AbsEquipment::calcPowerFactor(int a, int r)
{
    float active = static_cast<float>(a);
    float reactive = static_cast<float>(r);

    if (a == 0 )
        return 1;

    return active/sqrt(active*active + reactive*reactive);
}


void AbsEquipment::on_historicPushButton_clicked()
{
    emit sendReadRequest(QModbusDataUnit(
                             QModbusDataUnit::InputRegisters,
                             ABS_HIST_ADDR, ABS_HIST_SIZE),ABS_SSU_ADDRESS);
}

void AbsEquipment::on_exportButton_clicked()
{
    if (db.isValid() && db.isOpen()) {
        db.open();
        db.close();
    }

    QString fileName = QFileDialog::getSaveFileName(
                this, tr("Salvar arquivo de dados"),
                QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                tr("DB Files (*.db)"));

    db = QSqlDatabase::addDatabase("QSQLITE");

    if (fileName == "") {
        qWarning() << "foi";
        ui->fileLineEdit->setText("Não salvo em arquivo!");
        db.setDatabaseName(":memory:");
    }
    else {
        ui->fileLineEdit->setText(fileName);
        db.setDatabaseName(fileName);
    }


    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QObject::tr("Unable to establish a database connection.\n"
                        "This example needs SQLite support. Please read "
                        "the Qt SQL driver documentation for information how "
                        "to build it.\n\n"
                        "Click Cancel to exit."), QMessageBox::Cancel);
        return;
    }

    QSqlQuery query;
    query.exec("create table demanda (id int primary key, "
               "data varchar(10), hora varchar(10), posto varchar(10), "
               "ativa INTEGER, reativa INTEGER, fp REAL, ufer varchar(3), quadrant varchar (2),"
               "intRea varchar(1),repD varchar(1), obs varchar(10), bytes varchar(50))");

    model = new QSqlTableModel(this);
    model->setTable("demanda");
    model->setEditStrategy(QSqlTableModel::OnRowChange);
    model->setSort(0,Qt::DescendingOrder);
    model->select();

    model->setHeaderData(0, Qt::Horizontal, tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, tr("Data"));
    model->setHeaderData(2, Qt::Horizontal, tr("Hora"));
    model->setHeaderData(3, Qt::Horizontal, tr("Posto"));
    model->setHeaderData(4, Qt::Horizontal, tr("Ativa"));
    model->setHeaderData(5, Qt::Horizontal, tr("Reativa"));
    model->setHeaderData(6, Qt::Horizontal, tr("FatorP"));
    model->setHeaderData(7, Qt::Horizontal, tr("UFER"));
    model->setHeaderData(8, Qt::Horizontal, tr("Quadrante"));
    model->setHeaderData(9, Qt::Horizontal, tr("Hora"));
    model->setHeaderData(10, Qt::Horizontal, tr("RepDem"));
    model->setHeaderData(11, Qt::Horizontal, tr("Obs"));

    ui->demandTableView->setModel(this->model);
    ui->demandTableView->resizeColumnsToContents();
    ui->demandTableView->update();

    ui->downloadButton->setEnabled(true);
}

void AbsEquipment::on_ereasePushButton_clicked()
{
    QVector<quint16> writeRegisters = QVector<quint16>(2);
    writeRegisters[0] = static_cast<quint16>(1);
    writeRegisters[1] = static_cast<quint16>(14);

    QModbusDataUnit write = QModbusDataUnit(
                QModbusDataUnit::HoldingRegisters,ABS_EREASE_MEMORY_ADDRESS, 2);
    write.setValue(0,writeRegisters[0]);
    write.setValue(1,writeRegisters[1]);


    emit sendWriteRequest(write,ABS_SSU_ADDRESS);
}

void AbsEquipment::on_updateClockButton_clicked()
{
    uint8_t i;

    QDate date = ui->clockDateTimeEdit->dateTime().date();
    QTime time = ui->clockDateTimeEdit->dateTime().time();
    QVector<quint16> writeRegisters = QVector<quint16>(10);
    writeRegisters[0] = static_cast<quint16>(time.second());
    writeRegisters[1] = static_cast<quint16>(time.minute());
    writeRegisters[2] = static_cast<quint16>(time.hour());
    writeRegisters[3] = static_cast<quint16>(date.day());
    writeRegisters[4] = static_cast<quint16>(date.month());
    writeRegisters[5] = static_cast<quint16>(date.year() - 2000);
    writeRegisters[6] = static_cast<quint16>(0);
    writeRegisters[7] = static_cast<quint16>(0);
    writeRegisters[8] = static_cast<quint16>(1);

    QModbusDataUnit write = QModbusDataUnit(
                QModbusDataUnit::HoldingRegisters,ABS_CLOCK_SET_ADDRESS, ABS_CLOCK_SET_SIZE);

    for (i = 0; i < 9; i++)
        write.setValue(i,writeRegisters[i]);

    emit sendWriteRequest(write,ABS_SSU_ADDRESS);
}

void AbsEquipment::on_orderComboBox_activated(int index)
{
    (void)index;

    if (ui->orderComboBox->currentData().toInt() == ABS_ORDER_DESCEND){
        ui->initialSpinBox->setValue(ultimaDemanda);
        ui->finalSspinBox->setValue(primeiraDemanda);
    }
    else{
        ui->initialSpinBox->setValue(primeiraDemanda);
        ui->finalSspinBox->setValue(ultimaDemanda);
    }
}
