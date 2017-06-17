#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "processthreadmodule_dialog.h"
#include "cpu_memory_dialog.h"
#include "cleaner_dialog.h"
#include "service_dialog.h"
#include "pe_dialog.h"
#include "virus_dialog.h"
#include "dialog.h"
#include <QMovie>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_button_PTM_show_clicked();

    void on_button_CPUMM_show_clicked();

    void on_button_Clean_show_clicked();

    void on_button_Service_show_clicked();

    void on_button_PE_show_clicked();

    void on_button_Virus_show_clicked();

    void on_button_other_show_clicked();

private:
    Ui::MainWindow *ui;
    ProcessThreadModule_Dialog *pPTMDialog = new ProcessThreadModule_Dialog;
    CPU_MEMORY_Dialog *pCPUMMDialog = new CPU_MEMORY_Dialog;
    Cleaner_Dialog *pCleanDialog = new Cleaner_Dialog;
    Service_Dialog *pServiceDialog = new Service_Dialog;
    PE_Dialog *pPEDialog = new PE_Dialog;
    Virus_Dialog *pVirusDialog = new Virus_Dialog;
    Dialog *pDialog = new Dialog;
};

#endif // MAINWINDOW_H
