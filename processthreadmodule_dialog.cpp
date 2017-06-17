#include "processthreadmodule_dialog.h"
#include "ui_processthreadmodule_dialog.h"
#include <tlhelp32.h>
#include <Psapi.h>
#include <QMessageBox>
#include <windows.h>
#include <shlobj.h>
#include <QDebug>

#define WINDOW_TEXT_LENGTH 256

typedef struct _para{
    DWORD dwPid;
    QTreeWidgetItem *point;
    QTreeWidget *wid;
}Param;

BOOL    AddPrivilege( HANDLE hProcess , const TCHAR* pszPrivilegeName );
BOOL    IsAdmin( HANDLE hProcess );
BOOL    CALLBACK EnumChildWindowCallBack(HWND hWnd, LPARAM lParam);
BOOL    CALLBACK EnumChildWindowCallBack(HWND hWnd, LPARAM lParam);

BOOL SetPrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES NewState;
    LUID luidPrivilegeLUID;

    //获取进程令牌
    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)||!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidPrivilegeLUID))
    {
        qDebug() << "SetPrivilege Error\n";
        return FALSE;
    }
    NewState.PrivilegeCount = 1;
    NewState.Privileges[0].Luid = luidPrivilegeLUID;
    NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    //提示进程权限，注意该函数也可以改变线程的权限，如果hToken指向一个线程的令牌
    if(!AdjustTokenPrivileges(hToken, FALSE, &NewState, NULL, NULL, NULL))
    {
        qDebug() << "AdjustTokenPrivilege Errro\n";
        return FALSE;
    }

    qDebug()<<"权限提升成功！";
    return TRUE;
}

ProcessThreadModule_Dialog::ProcessThreadModule_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProcessThreadModule_Dialog)
{
    ui->setupUi(this);

    if( !AddPrivilege( GetCurrentProcess( ) , SE_DEBUG_NAME/*字符串形式的权限名*/ ) )
        QMessageBox::information(NULL,"info", "提升权限失败\n" );

    SetPrivilege();

    ui->table_ProcessList->setSelectionBehavior ( QAbstractItemView::SelectRows);  //设置选择行为，以行为单位
    ui->table_ProcessList->setSelectionMode ( QAbstractItemView::SingleSelection); //设置选择模式，选择单行
    ui->table_ProcessList->setEditTriggers(QAbstractItemView::NoEditTriggers);     //关闭编辑
    ui->table_ProcessList->setColumnCount(4);

    QStringList header;
    header<<"进程名"<<"进程ID"<<"子进程数量"<<"进程优先级";
    ui->table_ProcessList->setHorizontalHeaderLabels(header);
    ui->table_ProcessList->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_ProcessList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->table_ProcessList,&QTableWidget::cellDoubleClicked,this,&ProcessThreadModule_Dialog::SelectProcess);

    header.clear();
    header << "模块名"<<"模块加载基地址"<<"模块占用内存大小"<<"模块的入口地址";
    ui->table_ModuleList->setColumnCount(4);
    ui->table_ModuleList->setHorizontalHeaderLabels(header);

    header.clear();
    header<<"线程ID"<<"线程优先级"<<"进程ID";
    ui->table_ThreadList->setColumnCount(3);
    ui->table_ThreadList->setHorizontalHeaderLabels(header);

    if(ui->tree_WindowList->headerItem())
    {
        ui->tree_WindowList->headerItem()->setText(0, "窗口文本");
    }

    // 初始化线程
    ProcessUpdateThread* pThread = new ProcessUpdateThread();
    connect(pThread,SIGNAL(updateProcess(QStringList)),this,SLOT(updateProcess(QStringList)));
    connect(pThread,SIGNAL(initProcess(QStringList)),this,SLOT(initProcess(QStringList)));
    connect(pThread,SIGNAL(cleanContent(int)),this,SLOT(cleanContent(int)));
    pThread->start();
}

ProcessThreadModule_Dialog::~ProcessThreadModule_Dialog()
{
    delete ui;
}

void ProcessThreadModule_Dialog::on_button_PTM_return_clicked()
{
    this->close();
}

void ProcessThreadModule_Dialog::SelectProcess(int row, int col)
{
    DWORD dwPid = ui->table_ProcessList->item(row,1)->text().toLongLong();
    QString msg;

    //
    // 遍历线程
    //
    init_thread_list(dwPid);

    //
    // 遍历模块
    //
    init_module_list(dwPid);

    //
    // 遍历窗口
    //
    init_window_list(dwPid);
}

void ProcessThreadModule_Dialog::updateProcess(QStringList msg)
{
    int row = msg.at(0).toInt();

    //
    //  判断当前行是否已经存在，
    //  如果已经存在：判断是否需要更新，如果需要就更新，不需要就不更新
    //  如果不存在： 则插入新的QTableWidgetItem
    //

    if(row >= ui->table_ProcessList->rowCount())
    {
        row = ui->table_ProcessList->rowCount();
        ui->table_ProcessList->setRowCount(row + 1);

        for(int i=0; i<4; i++)
        {
            ui->table_ProcessList->setItem(row,i,new QTableWidgetItem(msg.at(i+1)));
        }
    }
    else
    {
        for(int i=0; i<4; i++)
        {
            QString current = ui->table_ProcessList->item(row,i)->text();
            if(QString::compare(current,msg.at(i+1))==0)
            {
                continue;
            }
            else
            {
                ui->table_ProcessList->item(row,i)->setText(msg.at(i+1));
            }
        }
    }
}

void ProcessThreadModule_Dialog::initProcess(QStringList msg)
{
    int row = ui->table_ProcessList->rowCount();


    ui->table_ProcessList->setRowCount(row + 1);


    //qDebug()<<msg;

    for(int i=0; i<4; i++)
    {
        ui->table_ProcessList->setItem(row,i,new QTableWidgetItem(msg.at(i)));
    }
}

void ProcessThreadModule_Dialog::cleanContent(int count)
{
    //qDebug()<< "clean count" << count;
    ui->table_ProcessList->setRowCount(count+1);
}

void ProcessThreadModule_Dialog::init_process_list()
{
    //
    //  首先建立进程列表
    //

    ui->table_ProcessList->clear();

    ui->table_ProcessList->setColumnCount(4);

    QString msg;
    QStringList header;
    header<<"进程名"<<"进程ID"<<"子进程数量"<<"进程优先级";

    ui->table_ProcessList->setHorizontalHeaderLabels(header);


    HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(INVALID_HANDLE_VALUE == hProcSnap)
    {
        msg = "Create process snap failed";
        QMessageBox::information(NULL,"Info",msg,QMessageBox::Yes);
        return;
    }

    PROCESSENTRY32 processInfo ;
    processInfo.dwSize = sizeof(processInfo);
    Process32First(hProcSnap, &processInfo);

    DWORD count = 0;
    do {

        ui->table_ProcessList->setRowCount(count+1);

        // 插入进程名
        msg = QString("%1").arg(QString::fromWCharArray(processInfo.szExeFile));
        ui->table_ProcessList->setItem(count,0,new QTableWidgetItem(msg));

        // 插入进程ID
        msg = QString("%1").arg(processInfo.th32ProcessID);
        ui->table_ProcessList->setItem(count,1,new QTableWidgetItem(msg));

        // 插入进程的线程数量
        msg = QString("%1").arg(processInfo.cntThreads);
        ui->table_ProcessList->setItem(count,2,new QTableWidgetItem(msg));

        // 插入进程的优先级
        msg = QString("%1").arg(processInfo.pcPriClassBase);
        ui->table_ProcessList->setItem(count,3,new QTableWidgetItem(msg));

        count++;

    }while( Process32Next( hProcSnap , &processInfo ) );

    ui->table_ProcessList->show();
    ui->table_ProcessList->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_ProcessList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void ProcessThreadModule_Dialog::init_thread_list(DWORD dwPid)
{
    // 首先清空线程table
    ui->table_ThreadList->clearContents();

    QString msg;

    HANDLE hThreadSnap = 0;
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
    if(hThreadSnap == INVALID_HANDLE_VALUE)
    {
        msg = QString("创建线程快照失败！");
        QMessageBox::information(NULL,"Info",msg,QMessageBox::Yes);
        return;
    }

    THREADENTRY32 stcTe32 = {sizeof(THREADENTRY32)};
    Thread32First(hThreadSnap, &stcTe32);
    DWORD count = 0;
    do{


        if(stcTe32.th32OwnerProcessID == dwPid)
        {
            ui->table_ThreadList->setRowCount(count+1);

            msg = QString("%1").arg(stcTe32.th32ThreadID);
            ui->table_ThreadList->setItem(count,0,new QTableWidgetItem(msg));

            msg = QString("%1").arg(stcTe32.tpBasePri);
            ui->table_ThreadList->setItem(count,1,new QTableWidgetItem(msg));

            msg = QString("%1").arg(dwPid);
            ui->table_ThreadList->setItem(count,2,new QTableWidgetItem(msg));

            count++;
        }
    }while(Thread32Next(hThreadSnap,&stcTe32));

    ui->table_ThreadList->show();
    ui->table_ThreadList->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_ThreadList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void ProcessThreadModule_Dialog::init_module_list(DWORD dwPid)
{

    ui->table_ModuleList->clearContents();
    ui->table_ModuleList->setRowCount(0);

    QString msg;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwPid);

    if(hProcess == 0)
    {
        QMessageBox::information(NULL,"Info",QString("%1").arg(GetLastError()));
    }

    DWORD dwBuffSize = 0;
    EnumProcessModulesEx(hProcess,NULL,0,&dwBuffSize,LIST_MODULES_ALL);
    qDebug()<<"GetLastError 1 "<<GetLastError();
    HMODULE* pModuleHandler = (HMODULE*)new char[dwBuffSize];

    EnumProcessModulesEx(hProcess, pModuleHandler,dwBuffSize,&dwBuffSize,LIST_MODULES_ALL);
    qDebug()<<"GetLastError 2 "<<GetLastError();

    TCHAR szModuleName[MAX_PATH];
    for(int i=0;i < dwBuffSize/sizeof(HMODULE);i++)
    {
        ui->table_ModuleList->setRowCount(i+1);
        MODULEINFO stcModuleInfo = {0};
        GetModuleInformation(hProcess,pModuleHandler[i],&stcModuleInfo,sizeof(stcModuleInfo));
        //QMessageBox::information(NULL,"Info",QString("GetModuleError%1").arg(GetLastError()));
        GetModuleFileNameEx(hProcess,pModuleHandler[i],szModuleName,MAX_PATH);

        // 插入模块名
        msg = QString("%1").arg(QString::fromWCharArray(szModuleName));
        ui->table_ModuleList->setItem(i,0,new QTableWidgetItem(msg));

        // 插入模块基址
        msg = QString("0x%1").arg((DWORD)(stcModuleInfo.lpBaseOfDll),0,16);
        ui->table_ModuleList->setItem(i,1,new QTableWidgetItem(msg));

        // 插入模块的映像大小
        msg = QString("0x%1").arg(stcModuleInfo.SizeOfImage,0,16);
        ui->table_ModuleList->setItem(i,2,new QTableWidgetItem(msg));

        // 插入模块的入口点
        msg = QString("0x%1").arg((DWORD)(stcModuleInfo.EntryPoint),0,16);
        ui->table_ModuleList->setItem(i,3,new QTableWidgetItem(msg));
    }

    ui->table_ModuleList->show();
    ui->table_ModuleList->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_ModuleList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    CloseHandle(hProcess);
    hProcess = 0;
}

BOOL CALLBACK EnumWindowCallBack(HWND hWnd, LPARAM lParam)
{
    DWORD dwPid = 0;
    QString msg;
    Param *p = (Param*)lParam;

    GetWindowThreadProcessId(hWnd, &dwPid);
    if(dwPid == p->dwPid)
    {

        TCHAR buf[WINDOW_TEXT_LENGTH];
        SendMessage(hWnd, WM_GETTEXT, WINDOW_TEXT_LENGTH,(LPARAM)buf);
        msg = QString::fromWCharArray(buf);
        qDebug()<<"window text" << msg;

        QTreeWidgetItem *root = new QTreeWidgetItem(p->wid,QStringList()<<msg/*<<QString("0x%1").arg((DWORD)hWnd,0,16)*/);

        p->point= root;
        EnumChildWindows(hWnd,EnumChildWindowCallBack,(LPARAM)p);
    }
    return true;
}

void ProcessThreadModule_Dialog::init_window_list(DWORD dwPid)
{
    ui->tree_WindowList->clear();

    ui->tree_WindowList->setColumnCount(1);
    QStringList header;
    header << "窗口文本";
    ui->tree_WindowList->setHeaderLabels(header);


    Param *p = new Param;
    p->dwPid = dwPid;
    p->wid = ui->tree_WindowList;
    p->point = NULL;
    EnumWindows(EnumWindowCallBack,(LPARAM)p);

    ui->tree_WindowList->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tree_WindowList->expandAll();
    delete p;
}

BOOL CALLBACK EnumChildWindowCallBack(HWND hWnd, LPARAM lParam)
{
    DWORD dwPid = 0;
    QString msg;
    Param *p = (Param*)lParam;

    GetWindowThreadProcessId(hWnd,&dwPid);
    if(dwPid == p->dwPid)
    {

        TCHAR buf[WINDOW_TEXT_LENGTH];
        SendMessage(hWnd, WM_GETTEXT, WINDOW_TEXT_LENGTH,(LPARAM)buf);
        msg = QString::fromWCharArray(buf);
        qDebug()<<"window text" << msg;

        if(msg.size() == 0)
        {
            msg = "无标题";
        }

        QTreeWidgetItem *root = new QTreeWidgetItem(p->point,QStringList(msg));
        p->point = root;
        EnumChildWindows(hWnd,EnumChildWindowCallBack,(LPARAM)p);
    }
    return TRUE;
}

BOOL    AddPrivilege( HANDLE hProcess , const TCHAR* pszPrivilegeName )
{
    // 进程的特权使用LUID值来表示, 因此, 需要先获取传入的权限名对应的LUID值


    // 0. 获取特权对应的LUID值
    LUID privilegeLuid;
    if( !LookupPrivilegeValue( NULL , pszPrivilegeName , &privilegeLuid ) )
        return FALSE;


    // 1. 获取本进程令牌
    HANDLE hToken = NULL;
    // 打开令牌时, 需要加上TOKEN_ADJUST_PRIVILEGES 权限(这个权限用于修改令牌特权)
    if( !OpenProcessToken( GetCurrentProcess( ) , TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY , &hToken ) ) {
        printf( "错误码:%x\n" , GetLastError( ) );
        return 0;
    }

    // 2. 使用令牌特权修改函数将SeDebug的LUID特权值添加到本进程令牌
    TOKEN_PRIVILEGES tokenPrivieges; // 新的特权

    // 使用特权的LUID来初始化结构体.
    tokenPrivieges.PrivilegeCount = 1; // 特权个数
    tokenPrivieges.Privileges[ 0 ].Luid = privilegeLuid; // 将特权LUID保存到数组中
    tokenPrivieges.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;// 将属性值设为启用(有禁用,移除等其他状态)



    // 调用函数添加特权
    return AdjustTokenPrivileges( hToken ,              // 要添加特权的令牌
                                  FALSE ,               // TRUE是移除特权, FALSE是添加特权
                                  &tokenPrivieges ,     // 要添加的特权数组
                                  sizeof( tokenPrivieges ) ,// 整个特权数组的大小
                                  NULL ,                // 旧的特权数组
                                  NULL                  // 旧的特权数组的长度
                                  );
}

BOOL    IsAdmin( HANDLE hProcess )
{
    HANDLE hToken = NULL;
    OpenProcessToken( hProcess , TOKEN_QUERY , &hToken );

    TOKEN_ELEVATION_TYPE tokenType = TokenElevationTypeDefault ; // 用于接收令牌类型

    DWORD dwRetSize = 0; // 用于接收函数输出信息的字节数

    // 2. 查询进程令牌中的权限提升值.( 这个值会记录当前的令牌是何种类型( 细节在17_权限管理_令牌的获取.cpp ) )
    GetTokenInformation( hToken ,
                         TokenElevationType ,// 获取令牌的当前提升等级
                         &tokenType ,
                         sizeof( tokenType ) ,
                         &dwRetSize // 所需缓冲区的字节数
                         );


    // 根据令牌的类型来输出相应的信息
    if( TokenElevationTypeFull == tokenType ) {
        // 3. 如果令牌是TokenElevationTypeFull , 则拥有至高无上的能力,可以给令牌添加任何特权
        qDebug()<< "管理员账户,并拥有全部的权限,可以给令牌添加任何特权\n" ;
        return TRUE;
    }
    // 4. 如果是其他的, 则需要以管理员身份重新运行本进程. 这样就能以第三步的方法解决剩下的问题.
    else if( TokenElevationTypeDefault == tokenType ) {
        qDebug()<< "默认用户, 可能是一个普通用户, 可能是关闭UAC时登录的管理员用户\n" ;
        // 调用系统函数IsUserAnAdmin, 进一步确定是普通用户还是管理员用户
        return IsUserAnAdmin();
    }
    else if( TokenElevationTypeLimited == tokenType ) {

        // 判断受限制的用户是管理员
        // 如果是管理员, 则这个令牌中会保存有管理员的SID

        // 1. 获取系统内键管理员用户的SID
        SID adminSid;
        DWORD dwSize = sizeof( adminSid );
        CreateWellKnownSid( WinBuiltinAdministratorsSid , // 获取SID的类型,这里是系统内键管理员
                            NULL , // 传NULL,获取本地计算机的管理员
                            &adminSid ,// 函数输出的管理员SID
                            &dwSize // 输入结构的大小,也作为输出
                            );

        // 获取本令牌的连接令牌(受限制的令牌都会有一个连接的令牌,受限制的令牌正式由主令牌所创建的. )
        TOKEN_LINKED_TOKEN linkToken;
        GetTokenInformation( hToken ,
                             TokenLinkedToken , // 获取连接的令牌句柄
                             &linkToken ,
                             sizeof( linkToken ) ,
                             &dwSize
                             );

        // 在连接的令牌中查找是否具有管理员的SID
        BOOL    bIsContain = FALSE; // 用于保存是否包含.
        CheckTokenMembership( linkToken.LinkedToken , // 在这个令牌中检查
                              &adminSid ,             // 检查令牌中是否包含此SID
                              &bIsContain );           // 输出TRUE则包含,反之不包含



        if( bIsContain ) {
            qDebug()<< "权限被阉割的受限制管理员账户, 部分权限被移处理\n";
        }
        return FALSE; // 不是以管理员权限运行
    }
    return FALSE;
}

ProcessUpdateThread::ProcessUpdateThread()
{
    //qDebug()<<"process Thread init";
}

void ProcessUpdateThread::update()
{
    QStringList msg;
    HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(INVALID_HANDLE_VALUE == hProcSnap)
    {
        QString info = "Create process snap failed";
        QMessageBox::information(NULL,"Info",info,QMessageBox::Yes);
        return;
    }

    PROCESSENTRY32 processInfo ;
    processInfo.dwSize = sizeof(processInfo);
    Process32First(hProcSnap, &processInfo);
    int count = 0;
    do {
        msg.clear();

        // 插入进程名
        msg << QString("%1").arg(count);

        msg << QString("%1").arg(QString::fromWCharArray(processInfo.szExeFile));

        // 插入进程ID
        msg << QString("%1").arg(processInfo.th32ProcessID);

        // 插入进程的线程数量
        msg << QString("%1").arg(processInfo.cntThreads);

        // 插入进程的优先级
        msg << QString("%1").arg(processInfo.pcPriClassBase);

        //qDebug()<<msg;
        emit updateProcess(msg);
        count++;

    }while( Process32Next( hProcSnap , &processInfo ) );

    //qDebug()<<"线程"<<count;
    emit cleanContent(count-1);
}

void ProcessUpdateThread::init()
{
    QStringList msg;
    HANDLE hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(INVALID_HANDLE_VALUE == hProcSnap)
    {
        QString info = "Create process snap failed";
        QMessageBox::information(NULL,"Info",info,QMessageBox::Yes);
        return;
    }

    PROCESSENTRY32 processInfo ;
    processInfo.dwSize = sizeof(processInfo);
    Process32First(hProcSnap, &processInfo);

    do {
        msg.clear();

        // 插入进程名
        msg << QString("%1").arg(QString::fromWCharArray(processInfo.szExeFile));

        // 插入进程ID
        msg << QString("%1").arg(processInfo.th32ProcessID);

        // 插入进程的线程数量
        msg << QString("%1").arg(processInfo.cntThreads);

        // 插入进程的优先级
        msg << QString("%1").arg(processInfo.pcPriClassBase);

        //qDebug()<<msg;
        emit initProcess(msg);

    }while( Process32Next( hProcSnap , &processInfo ) );
}


void ProcessUpdateThread::run()
{
    //qDebug()<<"Process Thread run!";
    init();
    while(true)
    {
        update();
        Sleep(1000);
    }
}
