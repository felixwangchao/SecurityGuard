#ifndef PROCESSTHREADMODULE_DIALOG_H
#define PROCESSTHREADMODULE_DIALOG_H

#include <QDialog>
#include <windows.h>
#include <QThread>

namespace Ui {
class ProcessThreadModule_Dialog;
class ProcessUpdateThread;
}

class ProcessUpdateThread : public QThread
{
    Q_OBJECT
signals:
        void updateProcess(QStringList msg);
        void initProcess(QStringList msg);
        void cleanContent(int count);

public:
    ProcessUpdateThread();
    void update();
    void init();
    void run();
 };

class ProcessThreadModule_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessThreadModule_Dialog(QWidget *parent = 0);
    ~ProcessThreadModule_Dialog();

private slots:
    void on_button_PTM_return_clicked();
    void SelectProcess(int row, int col);
    void updateProcess(QStringList msg);
    void initProcess(QStringList msg);
    void cleanContent(int count);

public:
    void init_process_list();
    void init_thread_list(DWORD dwPid);
    void init_module_list(DWORD dwPid);
    void init_window_list(DWORD dwPid);

private:
    Ui::ProcessThreadModule_Dialog *ui;
};

#endif // PROCESSTHREADMODULE_DIALOG_H
