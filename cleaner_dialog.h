#ifndef CLEANER_DIALOG_H
#define CLEANER_DIALOG_H

#include <QDialog>
#include <QThread>

namespace Ui {
class Cleaner_Dialog;
class CleanThread;
}

class CleanThread : public QThread
{
    Q_OBJECT
signals:
        void updateText(QString msg);
        void updateProgressBar(int value);
        void cleanFinish();
public:
    CleanThread(QStringList rubbish):rubbish(rubbish){}
    void setTarget(QStringList target);
    bool FindFile(const QString & path, int mode);
    void run();
private:
    QStringList target;
    QStringList rubbish;
 };


class Cleaner_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Cleaner_Dialog(QWidget *parent = 0);
    ~Cleaner_Dialog();

private:
    Ui::Cleaner_Dialog *ui;

public:
    void get_driver_information();
    void changeTest(int row, int col);
    void changeOther(int row, int col);
public:
    QStringList driver;
    QStringList rubbish;
    QStringList allDriver;
    QStringList otherClean;
    QStringList cleanTarget;
    CleanThread *pThread;

private slots:
    void on_button_clean_clicked();
    void on_update_text(QString msg);
    void on_update_progress_bar(int value);
    void on_clean_finish();
};

#endif // CLEANER_DIALOG_H
