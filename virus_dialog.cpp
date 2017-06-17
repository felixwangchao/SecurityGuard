#include "virus_dialog.h"
#include "ui_virus_dialog.h"

Virus_Dialog::Virus_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Virus_Dialog)
{
    ui->setupUi(this);

    whiteList<<"yrdy.exe"<<"helloworld.txt";
    md5List<<"0647abfd64b09748fd33de9ef2e03261"<<"cb85af2db7f433b8e5ef371f9767d773";

    // 初始化QButtonGroup
    scanType = new QButtonGroup(this);
    scanType->setExclusive(true);
    scanType->addButton(ui->radioButton_fullPath,0x01);
    scanType->addButton(ui->radioButton_partPath,0x02);
    ui->radioButton_fullPath->setChecked(true);

    ui->table_result->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_result->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置event
    g_WaitForRemoteEvent = CreateEvent( NULL ,  /*安全描述符*/
                                     FALSE , /*是否设置手动状态*/
                                     FALSE , /*是否有信号*/
                                     NULL ); /*事件对象的名字*/

    // 连接服务器
    connect_to_server();

    // 设置线程
    pThread = new ScanThread(md5List,whiteList,stClient);
    QObject::connect(pThread,SIGNAL(updateTable(QStringList)),this,SLOT(on_update(QStringList)));
}

Virus_Dialog::~Virus_Dialog()
{
    delete pThread;
    delete ui;
}

void Virus_Dialog::on_button_search_clicked()
{
    int scan = 0;
    if(ui->radioButton_fullPath->isChecked())
        scan = 1;
    else
        scan = 2;

    qDebug() << "QCheckBox_md5" << ui->checkBox_MD5->isChecked();
    qDebug() << "QCheckBox_whiteList" << ui->checkBox_WHITELIST->isChecked();

    if(!(ui->checkBox_MD5->isChecked())&&
       !(ui->checkBox_WHITELIST->isChecked())&&
       !(ui->checkBox_cloud->isChecked()))
    {
        QMessageBox::information(NULL,"Attention!","必须选择最少一种查杀方式");
        return;
    }

    int mode = 0;
    if(ui->checkBox_MD5->isChecked())
    {
        mode |= 1;

    }

    if(ui->checkBox_WHITELIST->isChecked())
    {
        mode |= 2;
    }

    if(ui->checkBox_cloud->isChecked())
    {
        mode |= 4;
    }

    if(scan == 2)
    {
        if(QString::compare(ui->line_path->text(),"")==0)
        {
            QMessageBox::information(NULL,"Attention!","您需要指定一个路径");
        }
        else
        {
            ui->table_result->clearContents();
            ui->table_result->setRowCount(0);
            QString path = ui->line_path->text();
            pThread->setPathMode(path,mode);
            pThread->start();
        }
    }
    else if(scan == 1)
    {
        QList<QString>::Iterator it = drivers.begin(),itend = drivers.end();

        ui->table_result->clearContents();
        ui->table_result->setRowCount(0);
        pThread->setPathListMode(drivers,mode);
        pThread->start();
    }
}

void ScanThread::run()
{
    for(int i=0; i<this->path.size();i++)
    {
        FindFile(this->path.at(i),this->mode);
    }
}

void ScanThread::setPathListMode(QStringList path, int mode)
{
    this->path.clear();
    for(int i=0; i<path.size();i++)
    {
        this->path<<path.at(i);
    }
    this->mode = mode;
}

void ScanThread::setPathMode(QString path, int mode)
{
    this->path.clear();
    this->path << path;
    this->mode = mode;
}

bool ScanThread::FindFile(const QString & path, int mode)
{
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

           if(i>=list.size())
           {
               break;
           }
           QFileInfo fileInfo = list.at(i);
           if(fileInfo.fileName()=="."|fileInfo.fileName()=="..")
           {
                i++;
                continue;
           }

           bIsDir = fileInfo.isDir();
           if (bIsDir)  // 找到的是文件夹
           {
                    FindFile(fileInfo.filePath(),mode);
           }
           else         // 找到的是文件
           {
                   qDebug()<<fileInfo.filePath();
                   bool flag = false;
                   if((mode & 1) == 1)
                   {
                        if(MD5Scan(fileInfo.filePath(),fileInfo))
                        {
                            flag == true;

                        }
                   }
                   if((mode & 2) == 2 && flag == false)
                   {

                       if(WhiteListScan(fileInfo.fileName(),fileInfo))
                       {
                           flag = true;
                       }
                   }
                   if((mode & 4) == 4 && flag == false)
                   {

                       if(CloudScan(fileInfo.filePath(),fileInfo))
                       {
                           flag = true;
                       }
                   }

           }
           i++;
    }while(i<list.size());
}

bool ScanThread::MD5Scan(QString path, QFileInfo fileInfo)
{

    QStringList msg;
    QFile theFile(path);
    theFile.open(QIODevice::ReadOnly);
    QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Md5);
    theFile.close();
    qDebug() << ba.toHex().constData();
    QString md5 = ba.toHex().constData();

    for (int i = 0; i < this->md5List.size(); i++)
    {
      if (QString::compare(this->md5List.at(i),md5)==0)
      {
            msg << fileInfo.fileName() << fileInfo.filePath() << md5 << "md5" << "删除";
            qDebug()<<msg;
            emit updateTable(msg);
            return true;
      }
    }
  return false;
}

bool ScanThread::CloudScan(QString path, QFileInfo fileInfo)
{
        QStringList msg;
        QFile theFile(path);
        theFile.open(QIODevice::ReadOnly);
        QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Md5);
        theFile.close();
        QString md5 = ba.toHex().constData();

        if (remoteCheck(md5))
        {

            msg << fileInfo.fileName() << fileInfo.filePath() << md5 << "cloud" << "删除";
            qDebug()<<msg;
            emit updateTable(msg);
            return true;
        }

        return false;
}

bool ScanThread::remoteCheck(QString md5)
{

    qDebug() << "md5" << md5;
    qDebug() << "remote check"<<md5.toLatin1().data();

    int nRet;

    nRet = send(this->stClient, md5.toLatin1().data(), strlen(md5.toLatin1().data()), 0);
    qDebug() << "nRet" <<nRet;
    CHAR recvBuf[1024] = {};
    recv(this->stClient, recvBuf, 1024, 0);
    qDebug()<<"recv"<<recvBuf;
    QString msg = QString(recvBuf);
    qDebug()<<"msg" <<msg <<"compare"<<QString::compare(msg, "TRUE");
    if(QString::compare(msg, "TRUE") == 0)
        return true;
    else
        return false;
}

bool ScanThread::WhiteListScan(QString filename, QFileInfo fileInfo)
{
    QStringList msg;
    for (int i = 0; i < this->whiteList.size(); i++)
    {
      if (QString::compare(this->whiteList.at(i),filename) == 0)
      {     /*
            int row = ui->table_result->rowCount();
            ui->table_result->setRowCount(row+1);
            ui->table_result->setItem(row,0,new QTableWidgetItem(fileInfo.fileName()));
            ui->table_result->setItem(row,1,new QTableWidgetItem(fileInfo.filePath()));
            ui->table_result->setItem(row,2,new QTableWidgetItem());
            ui->table_result->setItem(row,3,new QTableWidgetItem("白名单"));
            ui->table_result->setItem(row,4,new QTableWidgetItem("删除"));
            */
            msg << fileInfo.fileName() << fileInfo.filePath() << "" << "白名单" << "删除";
            qDebug()<<msg;
            emit updateTable(msg);
            return true;
      }
    }
  return false;
}

void Virus_Dialog::connect_to_server()
{
        ui->text_message->append("尝试连接服务器！");

    // 1. 信号检测(初始化编程环境  WSAStartup)
        WSADATA wsd = {};
        if (WSAStartup(MAKEWORD(2, 2), &wsd)) {
            printf("初始化环境失败！");
            return;
        }
        // 1.1 判断版本号是否通过
        if (!(LOBYTE(wsd.wVersion) == 2 &&
            HIBYTE(wsd.wVersion) == 2)) {
            printf("版本号检测不通过！");
            WSACleanup();
            return;
        }
        // 2. 买手机(创建套接字  socket)
        stClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (stClient == INVALID_SOCKET) {
            printf("创建套接字失败！");
            WSACleanup();
            return;
        }
        // 3. 连接服务器
        SOCKADDR_IN addrServer = {};
        addrServer.sin_family = AF_INET;
        addrServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        addrServer.sin_port = htons(7788);
        int nRet = ::connect(stClient, (sockaddr*)&addrServer, sizeof(sockaddr));
        if (nRet == SOCKET_ERROR)
        {
            ui->text_message->append("连接失败！");
        }
        else
        {
            ui->text_message->append("连接成功！");
        }
}

void Virus_Dialog::on_button_reconnect_clicked()
{
    connect_to_server();
    pThread->stClient = this->stClient;
}

void Virus_Dialog::on_update(QStringList msg)
{
    qDebug()<<"slot" << msg;
    int row = ui->table_result->rowCount();
    ui->table_result->setRowCount(row + 1);
    for(int i=0;i<5;i++)
    {
        ui->table_result->setItem(row,i,new QTableWidgetItem(msg.at(i)));
    }
}

void Virus_Dialog::OnUpdateTable(QStringList msg)
{
    qDebug()<<"slot" << msg;
    int row = ui->table_result->rowCount();
    ui->table_result->setRowCount(row + 1);
    for(int i=0;i<5;i++)
    {
        ui->table_result->setItem(row,i,new QTableWidgetItem(msg.at(i)));
    }
}
