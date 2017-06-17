#include "service_dialog.h"
#include "ui_service_dialog.h"
#include <windows.h>
#include <QDebug>
#include <QMenu>

Service_Dialog::Service_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Service_Dialog)
{
    ui->setupUi(this);

    // 初始化右键菜单
    popMenu = new QMenu(ui->table_service);
    OpenAction = new QAction("开启服务",this);
    CloseAction = new QAction("关闭服务",this);

    // 连接信号
    connect(ui->table_service,&QTableWidget::customContextMenuRequested,this,&Service_Dialog::rightClick);
    connect(OpenAction,&QAction::triggered,this,&Service_Dialog::rightClickedOpenOperation);
    connect(CloseAction,&QAction::triggered,this,&Service_Dialog::rightClickedCloseOperation);

    init_service_list();
}

Service_Dialog::~Service_Dialog()
{
    delete ui;
}

void Service_Dialog::rightClick(QPoint pos)
{
    popMenu->addAction(OpenAction);
    popMenu->addAction(CloseAction);
    popMenu->exec(QCursor::pos());
}

bool Service_Dialog::OpenCloseService(QString name, int mode)
{
    //
    //  mode 0   ---> 暂停      mode 1  ---> 开启
    //

    const wchar_t *m_ServiceName = name.toStdWString().data();


    //启动服务
     SC_HANDLE hSCM = OpenSCManager(      //打开服务控制管理器
       NULL, NULL, SC_MANAGER_ALL_ACCESS);

     if(mode == 0)
     {
         SC_HANDLE hService = OpenService(hSCM,  //打开服务
           m_ServiceName, SERVICE_STOP);

         SERVICE_STATUS status;

         ControlService(hService,        //结束服务
           SERVICE_CONTROL_STOP,
           &status);  \
     }
     else
     {
        SC_HANDLE hService = OpenService(hSCM,  //打开服务
           m_ServiceName, SERVICE_START);

        StartService(hService, 0, 0);      //启动服务
     }

    // 刷新服务列表
    init_service_list();
}

void Service_Dialog::rightClickedOpenOperation()
{
        //do something
    int row = ui->table_service->currentRow();
    OpenCloseService(ui->table_service->item(row,0)->text(), 1);
}

void Service_Dialog::rightClickedCloseOperation()
{
        // do something
    int row = ui->table_service->currentRow();
    OpenCloseService(ui->table_service->item(row,0)->text(), 0);
}


void Service_Dialog::init_service_list()
{
    // 初始化table
    qDebug() << "初始化服务列表";
    ui->table_service->clear();

    ui->table_service->setSelectionBehavior ( QAbstractItemView::SelectRows);  //设置选择行为，以行为单位
    ui->table_service->setSelectionMode ( QAbstractItemView::SingleSelection); //设置选择模式，选择单行
    ui->table_service->setEditTriggers(QAbstractItemView::NoEditTriggers);     //关闭编辑

    ui->table_service->setColumnCount(5);
    QStringList header;
    header <<"服务名"<<"服务状态"<<"服务类型"<<"服务启动类型"<<"服务路径";
    ui->table_service->setHorizontalHeaderLabels(header);

    //
    //  设置右键菜单
    //
    ui->table_service->setContextMenuPolicy(Qt::CustomContextMenu);

    // 打开计算机服务控制管理器
    SC_HANDLE hSCM = OpenSCManagerW(NULL,NULL,SC_MANAGER_ALL_ACCESS);

    // 第一次调用，获取需要的内存大小
    DWORD dwServiceNum = 0;
    DWORD dwSize = 0;
    EnumServicesStatusEx(hSCM,
                         SC_ENUM_PROCESS_INFO,
                         SERVICE_WIN32,
                         SERVICE_STATE_ALL,
                         NULL,
                         0,
                         &dwSize,
                         &dwServiceNum,
                         NULL,
                         NULL);

    // 申请需要的内存，第二次调用
    LPENUM_SERVICE_STATUS_PROCESS pEnumService = (LPENUM_SERVICE_STATUS_PROCESS)new char[dwSize];

    // 第二次枚举
    bool bStatus = FALSE;
    bStatus = EnumServicesStatusEx(hSCM,
                                   SC_ENUM_PROCESS_INFO,
                                   SERVICE_WIN32,
                                   SERVICE_STATE_ALL,
                                   (PBYTE)pEnumService,
                                   dwSize,
                                   &dwSize,
                                   &dwServiceNum,
                                   NULL,NULL);

    // 遍历信息
    for(DWORD i=0; i<dwServiceNum;i++)
    {
        // 获取基础信息
        // 服务名
        QString msg;

        ui->table_service->setRowCount(i+1);

        ui->table_service->setItem(i,0,new QTableWidgetItem(QString::fromWCharArray(pEnumService[i].lpServiceName)));

        switch(pEnumService[i].ServiceStatusProcess.dwCurrentState)
        {
            case SERVICE_CONTINUE_PENDING:
                msg = "SERVICE_CONTINUE_PENDING";
            break;
            case SERVICE_PAUSE_PENDING:
                msg = "SERVICE_PAUSE_PENDING";
            break;
            case SERVICE_PAUSED:
                msg = "SERVICE_PAUSED";
            break;
            case SERVICE_RUNNING:
                msg = "SERVICE_RUNNING";
            break;
            case SERVICE_START_PENDING:
                msg = "SERVICE_START_PENDING";
            break;
            case SERVICE_STOP_PENDING:
                msg = "SERVICE_STOP_PENDING";
            break;
            case SERVICE_STOPPED:
                msg = "SERVICE_STOPPED";
            break;
        }

        ui->table_service->setItem(i,1,new QTableWidgetItem(msg));

        // 服务类型
        msg = "";
        switch(pEnumService[i].ServiceStatusProcess.dwServiceType)
        {

            case SERVICE_FILE_SYSTEM_DRIVER:
            msg = "SERVICE_FILE_SYSTEM_DRIVER";
            break;

            case SERVICE_KERNEL_DRIVER:
            msg = "SERVICE_KERNEL_DRIVER";
            break;

            case SERVICE_WIN32_OWN_PROCESS:
            msg = "SERVICE_WIN32_OWN_PROCESS";
            break;

            case SERVICE_WIN32_SHARE_PROCESS:
            msg = "SERVICE_WIN32_SHARE_PROCESS";
            break;

            case 0x50:
            msg = "SERVICE_USER_OWN_PROCESS";
            break;

            case 0x60:
            msg = "SERVICE_USER_SHARE_PROCESS";
            break;

        default:
            msg = QString("0x%1").arg(pEnumService[i].ServiceStatusProcess.dwServiceType,0,16);
        }

        ui->table_service->setItem(i,2,new QTableWidgetItem(msg));

        // 打开服务
        SC_HANDLE hService = OpenService(hSCM,
                                         pEnumService[i].lpServiceName,
                                         SERVICE_QUERY_CONFIG);

        // 第一次调用获取需要的缓冲区大小
        QueryServiceConfig(hService, NULL, 0 ,&dwSize);

        // 分配内存
        LPQUERY_SERVICE_CONFIG pServiceConfig = \
                (LPQUERY_SERVICE_CONFIG)new char[dwSize];

        // 第二次调用，获取信息
        QueryServiceConfig(hService, pServiceConfig, dwSize, &dwSize);

        // 通过上面获取到的结构体信息具体得到想要的值
        // 获取启动类型
        msg = "";
        switch(pServiceConfig->dwStartType)
        {

        case SERVICE_AUTO_START:
            msg = "SERVICE_AUTO_START";
        break;
        case SERVICE_BOOT_START:
            msg = "SERVICE_BOOT_START";
        break;
        case SERVICE_DEMAND_START:
            msg = "SERVICE_DEMAND_START";
        break;
        case SERVICE_DISABLED:
            msg = "SERVICE_DISABLED";
        break;
        case SERVICE_SYSTEM_START:
            msg = "SERVICE_SYSTEM_START";
        break;
        }
        ui->table_service->setItem(i,3,new QTableWidgetItem(msg));
        ui->table_service->setItem(i,4,new QTableWidgetItem(QString::fromWCharArray(pServiceConfig->lpBinaryPathName)));
    }

    ui->table_service->show();
    ui->table_service->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_service->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}
