#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <windows.h>
#include <PowrProf.h>
#include <QDragEnterEvent>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QMimeData>
#include <QDateTime>
#include <QMessageBox>
#include "qxtglobalshortcut.h"
#include <windows.h>
#include <vector>
#include <QDebug>
#include <QMenu>
#include <Winuser.h>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

public:
    QString filename;
    QMenu *popMenu;
    QAction *removeAction;
    QMenu *StartingMenu;
    QAction *removeStarting;

public:
    void UninstallList();
    void VisitRegister(QStringList list, HKEY rootKey);
    void StartUpManage();

private slots:
    void on_button_PowerOff_clicked();
    void on_buttton_Restart_clicked();
    void on_button_Logoff_clicked();
    void on_button_Sleep_clicked();
    void toggle();
    void rightClick(QPoint pos);
    void StartRightClick(QPoint);
    void rightClickedRemoveOperation();
    void rightClickedRemoveStartingOperation();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);

private:
    Ui::Dialog *ui;
};

#endif // DIALOG_H
