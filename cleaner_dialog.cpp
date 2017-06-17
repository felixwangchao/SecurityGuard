#include "cleaner_dialog.h"
#include "ui_cleaner_dialog.h"
#include <windows.h>
#include <QProcess>
#include <QDebug>
#include <QDir>

Cleaner_Dialog::Cleaner_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Cleaner_Dialog)
{
    ui->setupUi(this);


    //
    // 初始化table
    //

    //driver.clear();
    ui->table_driver->clear();
    ui->table_driver->setColumnCount(3);
    QStringList header;
    header<<"盘符"<<"剩余大小"<<"总大小";
    ui->table_driver->setHorizontalHeaderLabels(header);
    connect(ui->table_driver,&QTableWidget::cellChanged,this,&Cleaner_Dialog::changeTest);

    header.clear();
    header << "选项";
    ui->table_rubbish->setColumnCount(1);
    ui->table_rubbish->setHorizontalHeaderLabels(header);
    connect(ui->table_rubbish,&QTableWidget::cellChanged,this,&Cleaner_Dialog::changeOther);

    get_driver_information();

    //
    //  初始化要删除的文件后缀
    //

    rubbish << "tlog" << "obj" << "log" << "pch" << "ilk" << "pdb"<<"avi"<<"mp4";

    //
    // 初始化进度条
    //

    ui->clean_progress->setValue(0);
    ui->clean_progress->hide();

    //
    //  初始化textedit
    //

    ui->text_fileDelete->setText("");
    ui->text_fileDelete->hide();

    // 初始化线程
    pThread = new CleanThread(rubbish);
    QObject::connect(pThread,SIGNAL(updateText(QString)),this,SLOT(on_update_text(QString)));
    QObject::connect(pThread,SIGNAL(updateProgressBar(int)),this,SLOT(on_update_progress_bar(int)));
    QObject::connect(pThread,SIGNAL(cleanFinish()),this,SLOT(on_clean_finish()));
}

Cleaner_Dialog::~Cleaner_Dialog()
{
    delete pThread;
    delete ui;
}

void Cleaner_Dialog::changeTest(int row, int col)
{

    if(ui->table_driver->item(row,col)->checkState() == Qt::Checked)
    {
        QString tmp = ui->table_driver->item(row,0)->text();
        driver << tmp;
    }
    if(ui->table_driver->item(row,col)->checkState() == Qt::Unchecked)
    {
        QString tmp = ui->table_driver->item(row,0)->text();
        int index = driver.indexOf(tmp);
        if(index != -1)
            driver.removeAt(index);
    }
}

void Cleaner_Dialog::changeOther(int row, int col)
{
    if(ui->table_rubbish->item(row,col)->checkState() == Qt::Checked)
    {
        QString tmp = ui->table_rubbish->item(row,0)->text();
        otherClean << tmp;
    }
    if(ui->table_rubbish->item(row,col)->checkState() == Qt::Unchecked)
    {
        QString tmp = ui->table_rubbish->item(row,0)->text();
        int index = otherClean.indexOf(tmp);
        if(index != -1)
            otherClean.removeAt(index);
    }
}

void Cleaner_Dialog::get_driver_information()
{
    allDriver.clear();
    driver.clear();
    otherClean.clear();

    // 获取所有驱动器
    WCHAR buff[ MAX_PATH ];
    GetLogicalDriveStrings(MAX_PATH , buff);
    WCHAR* pTemp = buff;

    TCHAR	volumeName[ MAX_PATH ];
    TCHAR	fileSystemName[ 50 ];

    DWORD count = 0;
    bool flag = false;

    do{

        flag = false;

        wprintf(L"%s" , pTemp);
        //qDebug() << QString::fromWCharArray(pTemp);

        // 获取卷的详细信息
        GetVolumeInformation(pTemp ,
                             volumeName , MAX_PATH ,/*卷的标题名*/
                             0 , 0 , 0 ,
                             fileSystemName , 50/*卷的文件系统名*/
                             );

        wprintf(L" %s %s " , volumeName , fileSystemName);

        // 获取驱动器类型
        UINT uType = GetDriveType(pTemp);
        switch(uType)
        {
            case DRIVE_NO_ROOT_DIR:qDebug()<<" 无效的根路径 "; break;
            case DRIVE_REMOVABLE:qDebug()<<" 可移动磁盘 "; break;
            case DRIVE_FIXED:
            //qDebug()<<" 硬盘 ";
            {
                flag = true;
                ui->table_driver->setRowCount(count+1);
                QTableWidgetItem *checkBox = new QTableWidgetItem(QString::fromWCharArray(pTemp));
                checkBox->setCheckState(Qt::Unchecked);
                ui->table_driver->setItem(count, 0 , checkBox);
                allDriver << QString::fromWCharArray(pTemp);
            }
            break;
            case DRIVE_REMOTE:qDebug()<<" 网络磁盘 "; break;
            case DRIVE_CDROM:
            {
                //qDebug()<<" 光盘驱动器 ";
                GetVolumeInformation(pTemp , 0 , 0 , 0 , 0 , 0 , 0 , 0);
                if(21 == GetLastError()) // 获取最后的错误信息
                {
                    qDebug()<<"(无光盘)";
                }
                else
                {
                    qDebug()<<"(有光盘)";
                }
            }
            break;
            case DRIVE_RAMDISK:printf(" RAM磁盘"); break;
            case DRIVE_UNKNOWN:printf(" 未知驱动器 "); break;
        }

        // 驱动器空间信息
        // 总空间信息, 剩余的空间信息
        DWORD dwSectorsPerCluster ;
        DWORD dwBytesPerSector;
        DWORD dwNumberOfFreeClusters ;
        DWORD dwTotalNumberOfClusters;
        GetDiskFreeSpace(pTemp ,
                         &dwSectorsPerCluster ,// 每簇扇区数
                         &dwBytesPerSector , // 每个扇区的字节数
                         &dwNumberOfFreeClusters ,// 剩余空间的簇的个数
                         &dwTotalNumberOfClusters// 总空间的簇的个数
                         );

        // 计算出剩余空间的字节数
        UINT64 freeByte = 1;
        freeByte = dwNumberOfFreeClusters;
        freeByte *= dwSectorsPerCluster;
        freeByte *= dwBytesPerSector;
        // 计算出总空间的字节数
        UINT64 totalByte = 1;
        totalByte = dwTotalNumberOfClusters;
        totalByte *= dwSectorsPerCluster;
        totalByte *= dwBytesPerSector;

        if(flag == true)
        {
            ui->table_driver->setItem(count,2,new QTableWidgetItem(QString("%1GB").arg(totalByte/1024/1024/1024.0)));
            ui->table_driver->setItem(count,1,new QTableWidgetItem(QString("%1GB").arg(freeByte/1024/1024/1024.0)));
            count++;
        }
        //qDebug()<<QString("(有光盘) 剩余: %1GB , 总:%2GB").arg(freeByte/1024/1024/1024.0).arg(totalByte/1024/1024/1024.0);
        pTemp += 4; //字符串的结构: C:\0D:\0E:\00
    } while(*pTemp != 0);

    ui->table_driver->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_driver->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    //
    // 获取回收站信息
    //

    SHQUERYRBINFO RecyleBinInfo = {};
    RecyleBinInfo.cbSize = sizeof(RecyleBinInfo);

    SHQueryRecycleBin(NULL,&RecyleBinInfo);

    ui->table_rubbish->setRowCount(2);
    QTableWidgetItem *checkBox = new QTableWidgetItem("回收站");
    checkBox->setCheckState(Qt::Unchecked);
    ui->table_rubbish->setItem(0, 0, checkBox);

    QTableWidgetItem *checkBox2 = new QTableWidgetItem("系统垃圾");
    checkBox2->setCheckState(Qt::Unchecked);
    ui->table_rubbish->setItem(1, 0, checkBox2);

    ui->table_rubbish->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_rubbish->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void Cleaner_Dialog::on_button_clean_clicked()
{

    cleanTarget.clear();
    for(int i=0; i<driver.size();i++)
    {
        cleanTarget<<driver.at(i);
    }
    for(int i=0; i<otherClean.size();i++)
    {
        cleanTarget<<otherClean.at(i);
    }
    qDebug()<<cleanTarget;
    ui->clean_progress->setValue(0);
    ui->clean_progress->show();
    ui->text_fileDelete->show();

    pThread->setTarget(cleanTarget);
    pThread->start();

    //get_driver_information();
}

void Cleaner_Dialog::on_update_text(QString msg)
{
    ui->text_fileDelete->append(msg);
}

void Cleaner_Dialog::on_update_progress_bar(int value)
{
    ui->clean_progress->setValue(value);
}

void Cleaner_Dialog::on_clean_finish()
{
    get_driver_information();
}

bool CleanThread::FindFile(const QString & path, int mode)
{
    //
    //  mode = 0 ==> global search
    //  mode = 1 ==> system rubbish
    //

    QDir dir(path);

    if (!dir.exists())
    {
        return false;
    }

    dir.setFilter(QDir::Dirs|QDir::Files);//除了目录或文件，其他的过滤掉
    dir.setSorting(QDir::DirsFirst);//优先显示目录
    QFileInfoList list = dir.entryInfoList();//获取文件信息列表

    int i = 0;
    bool bIsDir;
    do{
           if(list.size() == 0)
               break;
           int rate = (int)(i * 100 / list.size());

           QFileInfo fileInfo = list.at(i);
           if(fileInfo.fileName()=="."|fileInfo.fileName()=="..")
           {
                i++;
                continue;
           }

           bIsDir = fileInfo.isDir();
           if (bIsDir)
           {
                FindFile(fileInfo.filePath(),mode);
           }
           else
           {
               if(mode == 1)
               {
                   qDebug()<<fileInfo.filePath();
                   emit updateText(fileInfo.filePath());
               }
               else if(rubbish.indexOf(fileInfo.suffix())!= -1)
               {
                   qDebug() << fileInfo.filePath();
                   emit updateText(fileInfo.filePath());
               }
           }
           i++;
    }while(i<list.size());
}

void CleanThread::run()
{
    QStringList cleanTarget = this->target;

    int current = 0;

    QList<QString>::Iterator it = cleanTarget.begin(),itend = cleanTarget.end();
    int i = 0;
    emit updateProgressBar(10);
    for (;it != itend; it++,i++){

     qDebug() << *it;
     if(!(QString::compare(*it,"回收站")))
     {
         qDebug()<<"清空回收站";
         SHEmptyRecycleBin(NULL,NULL,SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
         //ui->text_fileDelete->append("回收站清理完毕");
     }
     else if(!(QString::compare(*it,"系统垃圾")))
     {
         qDebug()<< "系统垃圾";
         QString strWindows = QProcessEnvironment::systemEnvironment().value("windir");
         QString path1 = strWindows + "\\temp";
         QString path2 = strWindows + "\\prefetch";

         qDebug()<<path1;
         qDebug()<<path2;

         FindFile(path1,1);
         FindFile(path2,1);
     }
     else
     {
         FindFile(*it,0);
     }
     i++;
     current = (int)(100 / cleanTarget.size()) * i;
     if(current<100)
     emit updateProgressBar(current);
    }
    emit updateProgressBar(100);
    emit cleanFinish();
}

void CleanThread::setTarget(QStringList target)
{
    this->target = target;
}
