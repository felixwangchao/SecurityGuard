#ifndef PE_DIALOG_H
#define PE_DIALOG_H

#include <windows.h>
#include <QDialog>
#include <QTreeWidget>

namespace Ui {
class PE_Dialog;
}

class PE_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit PE_Dialog(QWidget *parent = 0);
    ~PE_Dialog();

public:
    int  ParsePE(HANDLE hFile);
    void parseResourceTable(DWORD dwResRootDirAddr ,/*根目录的首地址*/
                            IMAGE_RESOURCE_DIRECTORY* pResDir ,/*需要解析的资源目录*/
                            int nDeep /*记录这是第几层目录*/,
                            DWORD pFileBuff,
                            QTreeWidgetItem* leaf);

private slots:
    void on_button_openFile_clicked();

private:
    Ui::PE_Dialog *ui;
};

#endif // PE_DIALOG_H
