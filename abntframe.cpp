#include "abntframe.h"
#include "ui_abntframe.h"

#include <QTimer>
#include <QDebug>

#define TIMER_INTERVAL 1000

AbntFrame::AbntFrame(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AbntFrame)
{
    ui->setupUi(this);

    isCommunicating = false;

    abnt.remainingSeconds = 15;
    abnt.bDemandRepo = TRUE;
    abnt.bReactiveInterval = TRUE;
    abnt.bUferCap = TRUE;
    abnt.bUferInd = FALSE;
    abnt.horoType = FORA_PONTA;
    abnt.taxType = IRRIGANTES;
    abnt.bTaxActive = TRUE;
    abnt.activePulses = 0;
    abnt.reactivePulses = 0;
}

AbntFrame::~AbntFrame()
{
    delete ui;
}

void AbntFrame::on_connButton_clicked()
{
    if (isCommunicating) {
        ui->connButton->setText("Iniciar");
        isCommunicating = false;
    }

    else {
        ui->connButton->setText("Parar");
        isCommunicating = true;

        QTimer::singleShot(TIMER_INTERVAL, this, SLOT(onTimer()));
    }
}

 void AbntFrame::onTimer()
 {
    if (isCommunicating == false)
        return;

    uint8_t buffer[8];

    abnt.remainingSeconds--;
    abnt.activePulses += ui->activeBar->value();
    abnt.reactivePulses += ui->reactiveBar->value();

    if (abnt.remainingSeconds <= 0) {
        abnt.remainingSeconds = 899;
        abnt.activePulses = 0;
        abnt.reactivePulses = 0;
    }

    abntToByte(abnt,  &buffer[0]);

    qWarning() << "data: " << QByteArray::fromRawData(reinterpret_cast<char *>(buffer),8);

    emit getData(QByteArray::fromRawData(reinterpret_cast<char *>(buffer),8));

    QTimer::singleShot(TIMER_INTERVAL, this, SLOT(onTimer()));
 }


