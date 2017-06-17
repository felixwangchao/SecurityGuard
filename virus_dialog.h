#ifndef VIRUS_DIALOG_H
#define VIRUS_DIALOG_H

#include <QDir>
#include <QLabel>
#include <QDialog>
#include <QWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDebug>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QtNetwork>
#include <windows.h>
#include <WinSock2.h>
#include <QThread>


namespace Ui {
class Virus_Dialog;
class ScanThread;
}

class ScanThread : public QThread
{
    Q_OBJECT
signals:
        void updateTable(QStringList msg);
public:
    ScanThread(QStringList md5List,
               QStringList whiteList,
               SOCKET stClient):md5List(md5List),whiteList(whiteList),stClient(stClient)
                {}
    void run();
    void setPathMode(QString path, int mode);
    void setPathListMode(QStringList path, int mode);
    bool FindFile(const QString & path, int mode);
    bool MD5Scan(QString path, QFileInfo fileInfo);
    bool WhiteListScan(QString filename, QFileInfo fileInfo);
    bool remoteCheck(QString md5);
    bool CloudScan(QString path, QFileInfo fileInfo);

public:
    QStringList path;
    int mode;
    QStringList drivers;
    QStringList md5List;
    QStringList whiteList;
    SOCKET stClient;
 };

class Virus_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Virus_Dialog(QWidget *parent = 0);
    ~Virus_Dialog();

public:
    QButtonGroup *scanType;
    QStringList drivers;
    QStringList md5List;
    QStringList whiteList;
    HANDLE g_WaitForRemoteEvent = 0;
    SOCKET stClient;
    ScanThread *pThread;

public:

    void connect_to_server();

private slots:
    void on_button_search_clicked();
    void on_button_reconnect_clicked();
    void on_update(QStringList msg);
    void OnUpdateTable(QStringList msg);

private:
    Ui::Virus_Dialog *ui;
};

#endif // VIRUS_DIALOG_H
