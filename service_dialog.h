#ifndef SERVICE_DIALOG_H
#define SERVICE_DIALOG_H

#include <QDialog>
#include <QMenu>

namespace Ui {
class Service_Dialog;
}

class Service_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Service_Dialog(QWidget *parent = 0);
    ~Service_Dialog();

private:
    Ui::Service_Dialog *ui;
    QMenu *popMenu;
    QAction *OpenAction;
    QAction *CloseAction;

public:
    void init_service_list();
    void rightClick(QPoint pos);
    void rightClickedOpenOperation();
    void rightClickedCloseOperation();
    bool OpenCloseService(QString name, int mode);
};

#endif // SERVICE_DIALOG_H
