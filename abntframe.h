#ifndef ABNTFRAME_H
#define ABNTFRAME_H

#include <QWidget>

#include "abnt-14522.h"

namespace Ui {
class AbntFrame;
}

class AbntFrame : public QWidget
{
    Q_OBJECT

public:
    explicit AbntFrame(QWidget *parent = nullptr);
    ~AbntFrame();

signals:
    void getData(const QByteArray &data);

private slots:
    void on_connButton_clicked();
    void onTimer();

private:
    Ui::AbntFrame *ui;
    bool isCommunicating;
    QTimer *timer;
    abnt14522_t abnt;
};

#endif // ABNTFRAME_H
