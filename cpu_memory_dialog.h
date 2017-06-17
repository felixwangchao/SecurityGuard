#ifndef CPU_MEMORY_DIALOG_H
#define CPU_MEMORY_DIALOG_H

#include <QDialog>
#include <windows.h>
#include <process.h>
#include<psapi.h>
#include <QDebug>
#include <QLabel>
#include <QMovie>
#include <QThread>
#include <qpropertyanimation.h>

namespace Ui {
class CPU_MEMORY_Dialog;
class ProgressBarThread;
}

class ProgressBarThread : public QThread
{
    Q_OBJECT
signals:
        void updateProgressBar_1(int);
        void updateProgressBar_2(int);
        void updateLabel_1(QString msg);
        void updateLabel_2(QString msg);
public:
    ProgressBarThread();
    void run();
 };


class CPU_MEMORY_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit CPU_MEMORY_Dialog(QWidget *parent = 0);
    ~CPU_MEMORY_Dialog();

public:
    QMovie* movie;
    QPropertyAnimation *animation;

private slots:
    void on_button_accelerate_clicked();

    void on_button_CPUMM_return_clicked();

    void Stopframe(int frame);

    void OnNotify_1(int i);

    void OnNotify_2(int i);

    void OnLabel_1(QString msg);

    void OnLabel_2(QString msg);

private:
    Ui::CPU_MEMORY_Dialog *ui;

};

#endif // CPU_MEMORY_DIALOG_H
