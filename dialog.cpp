#include "dialog.h"
#include "ui_dialog.h"
#include <Winuser.h>
#include <shellapi.h>

using std::vector;

typedef struct _SOFTINFO
{
    WCHAR szSoftName[50];           // 软件名称
    WCHAR szSoftVer[50];            // 软件版本号
    WCHAR szSoftDate[20];           // 软件安装日期
    WCHAR szSoftSize[MAX_PATH];     // 软件大小
    WCHAR strSoftInsPath[MAX_PATH]; // 软件安装路径
    WCHAR strSoftUniPath[MAX_PATH]; // 软件卸载路径
    WCHAR strSoftVenRel[50];        // 软件发布厂商
    WCHAR strSoftIco[MAX_PATH];     // 软件图标路径
}SOFTINFO, *PSOFTINFO;

typedef struct _STARTINGINFO
{
    WCHAR lpKeyValueName[100];
    WCHAR lpSubKey[100];
    HKEY rootKey;
}STARTINGINFO,*PSTARTINGINFO;

vector<SOFTINFO> m_vectSoftInfo;
vector<STARTINGINFO> m_vectStartUpInfo;

uint MSGFLT_ADD = 1;
uint WM_COPYGLOBALDATA = 0x0049;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setAcceptDrops(true);

   typedef BOOL (WINAPI *PtrChangeWindowMessageFilterEx)(HWND hWnd, UINT message, DWORD action, void* pChangeFilterStruct);
   HMODULE hDLL = LoadLibraryA("User32.dll");

   PtrChangeWindowMessageFilterEx changeWindowMessageFilterEx;
   changeWindowMessageFilterEx=(PtrChangeWindowMessageFilterEx)GetProcAddress(hDLL, "ChangeWindowMessageFilterEx");

   if (changeWindowMessageFilterEx != NULL){
       (*changeWindowMessageFilterEx)((HWND)this->winId(),WM_DROPFILES,1,NULL);
       (*changeWindowMessageFilterEx)((HWND)this->winId(),0x0049,1,NULL);
   }
   else{
       qDebug() << "NULL FUNCTION POINTER";
   }
   if (hDLL != NULL){
      FreeLibrary(hDLL);
   }

   // -------------------------------------------------------------------------------
   //
   //                                  软件卸载
   //
   // -------------------------------------------------------------------------------

    UninstallList();
    ui->table_software->setContextMenuPolicy(Qt::CustomContextMenu);
    popMenu = new QMenu(ui->table_software);
    removeAction = new QAction("卸载",this);
    connect(ui->table_software,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(rightClick(QPoint)));
    connect(removeAction,SIGNAL(triggered()),this,SLOT(rightClickedRemoveOperation()));

    // -------------------------------------------------------------------------------
    //
    //                                  启动项
    //
    // -------------------------------------------------------------------------------

    ui->table_startup->setContextMenuPolicy(Qt::CustomContextMenu);
    StartingMenu = new QMenu(ui->table_startup);
    removeStarting = new QAction("删除启动项",this);
    connect(ui->table_startup,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(StartRightClick(QPoint)));
    connect(removeStarting,SIGNAL(triggered()),this,SLOT(rightClickedRemoveStartingOperation()));

    StartUpManage();
    // -------------------------------------------------------------------------------
    //
    //                                  老板键与系统控制
    //
    // -------------------------------------------------------------------------------

    connect(ui->button_toggle,&QPushButton::clicked,this, &Dialog::toggle);

    QxtGlobalShortcut *sc_gentleman = new QxtGlobalShortcut(this);
    sc_gentleman->setShortcut(QKeySequence("Alt+Shift+1"));
    connect(sc_gentleman, &QxtGlobalShortcut::activated,this,&Dialog::toggle);

    QxtGlobalShortcut *sc_powerOff = new QxtGlobalShortcut(this);
    sc_powerOff->setShortcut(QKeySequence("Alt+Shift+2"));
    connect(sc_powerOff, &QxtGlobalShortcut::activated,this,&Dialog::on_button_PowerOff_clicked);

    QxtGlobalShortcut *sc_reboot = new QxtGlobalShortcut(this);
    sc_reboot->setShortcut(QKeySequence("Alt+Shift+3"));
    connect(sc_reboot, &QxtGlobalShortcut::activated,this,&Dialog::on_button_PowerOff_clicked);

    QxtGlobalShortcut *sc_logOff = new QxtGlobalShortcut(this);
    sc_logOff->setShortcut(QKeySequence("Alt+Shift+4"));
    connect(sc_logOff, &QxtGlobalShortcut::activated,this,&Dialog::on_button_Logoff_clicked);

    QxtGlobalShortcut *sc_sleep = new QxtGlobalShortcut(this);
    sc_sleep->setShortcut(QKeySequence("Alt+Shift+5"));
    connect(sc_sleep, &QxtGlobalShortcut::activated,this,&Dialog::on_button_Sleep_clicked);


}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::toggle()
{
    QMessageBox::information(NULL,"info","老板键");
}

void Dialog::on_button_PowerOff_clicked()
{
    ExitWindowsEx(EWX_POWEROFF | EWX_FORCE,0);
}

void Dialog::on_buttton_Restart_clicked()
{
    ExitWindowsEx(EWX_REBOOT | EWX_FORCE,0);
}

void Dialog::on_button_Logoff_clicked()
{
    ExitWindowsEx(EWX_LOGOFF | EWX_FORCE,0);
}

void Dialog::on_button_Sleep_clicked()
{
    SetSuspendState(TRUE,FALSE,FALSE);
}

void Dialog::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}


void Dialog::dropEvent(QDropEvent *e)
{
    QList<QUrl> droppedUrls = e->mimeData()->urls();
    QString localPath = droppedUrls[0].toLocalFile();
    QFileInfo fileInfo(localPath);
    if(fileInfo.isFile())
    {
        ui->table_fileInfo->clear();
        ui->table_fileInfo->setColumnCount(1);
        ui->table_fileInfo->setRowCount(5);
        QStringList header;
        header << "文件名" << "文件路径"
               << "创建时间" << "修改时间"
               << "md5";

        ui->table_fileInfo->setVerticalHeaderLabels(header);
        header.clear();
        header << "文件信息";
        ui->table_fileInfo->setHorizontalHeaderLabels(header);

        QFile theFile(fileInfo.filePath());
        theFile.open(QIODevice::ReadOnly);
        QByteArray ba = QCryptographicHash::hash(theFile.readAll(), QCryptographicHash::Md5);
        theFile.close();
        QString md5 = ba.toHex().constData();


        ui->table_fileInfo->setItem(0,0,new QTableWidgetItem(fileInfo.fileName()));
        ui->table_fileInfo->setItem(1,0,new QTableWidgetItem(fileInfo.filePath()));
        ui->table_fileInfo->setItem(2,0,new QTableWidgetItem(fileInfo.created().toString("yyyy-MM-dd hh:mm:ss")));
        ui->table_fileInfo->setItem(3,0,new QTableWidgetItem(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
        ui->table_fileInfo->setItem(4,0,new QTableWidgetItem(md5));

        ui->table_fileInfo->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ui->table_fileInfo->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
}

void Dialog::dragMoveEvent(QDragMoveEvent *e)
{
    e->acceptProposedAction();
}

void Dialog::dragLeaveEvent(QDragLeaveEvent *e)
{

}


void Dialog::StartUpManage()
{
    m_vectStartUpInfo.clear();

    QStringList header;
    header << "启动项";
    ui->table_startup->clear();
    ui->table_startup->setColumnCount(1);
    ui->table_startup->setRowCount(0);
    ui->table_startup->setHorizontalHeaderLabels(header);

    QStringList hLocal;
    QStringList hCurrt;

    hLocal << "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"
           << "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce";

    hCurrt << "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
           << "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce";

    HKEY rootKey_hlm = HKEY_LOCAL_MACHINE;
    HKEY rootKey_hcu = HKEY_CURRENT_USER;

    VisitRegister(hLocal,rootKey_hlm);
    VisitRegister(hCurrt,rootKey_hcu);

    ui->table_startup->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_startup->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void Dialog::VisitRegister(QStringList list, HKEY rootKey)
{
    QString application;
    for(int i=0; i< list.size();i++)
    {
        DWORD wStrSize = 512;
        WCHAR *wStr = new WCHAR[wStrSize];
        DWORD dwCountOfSubKey;
        DWORD dwCountOfValue;
        DWORD dwType;
        LONG lResult;
        HKEY hkResult;

        LPCTSTR lpSubKey = reinterpret_cast<CONST WCHAR *>(list.at(i).utf16());
        lResult = RegOpenKeyEx(rootKey,
                    lpSubKey,
                    0,
                    KEY_QUERY_VALUE,
                    &hkResult);
        //qDebug() << list.at(i);
        //qDebug() << QString::fromWCharArray(lpSubKey);

        if(lResult != ERROR_SUCCESS)
        {
            qDebug()<< "error 0" << GetLastError() << "lresult" << lResult;
            break;
        }


        lResult = RegQueryInfoKey(hkResult,NULL,NULL,NULL,&dwCountOfSubKey,NULL,NULL,&dwCountOfValue,NULL,NULL,NULL,NULL);

        if(lResult != ERROR_SUCCESS)
        {
            qDebug()<< "error 1" <<  "lresult" << lResult;
            break;
        }

        for(int j=0; j<dwCountOfValue; j++)
        {
            wStrSize = 512;
            lResult = RegEnumValue(hkResult,j,wStr,&wStrSize,NULL,&dwType,NULL,NULL);

            if(lResult != ERROR_SUCCESS)
            {
                qDebug()<<"error 2";
                break;
            }

            application = QString::fromWCharArray(wStr);
            int rowCount = ui->table_startup->rowCount();
            ui->table_startup->setRowCount(rowCount+1);
            ui->table_startup->setItem(rowCount,0,new QTableWidgetItem(application));

            STARTINGINFO m_startup = {0};

            wcscpy_s(m_startup.lpKeyValueName,wStr);
            //qDebug()<<QString::fromWCharArray(m_startup.lpKeyValueName);
            wcscpy_s(m_startup.lpSubKey,lpSubKey);
            //qDebug()<<QString::fromWCharArray(m_startup.lpSubKey);
            m_startup.rootKey = rootKey;
            m_vectStartUpInfo.push_back(m_startup);
        }
        delete [] wStr;
    }
}

void Dialog::UninstallList()
{
    m_vectSoftInfo.clear();

    QStringList header;
    header << "软件名";
    ui->table_software->clear();
    ui->table_software->setColumnCount(1);
    ui->table_software->setRowCount(0);
    ui->table_software->setHorizontalHeaderLabels(header);

    HKEY RootKey = HKEY_LOCAL_MACHINE;
    LPCTSTR lpSubKey = L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    HKEY hkResult = 0;

    // 1. 打开一个已经存在的注册表键
    LONG lReturn = RegOpenKeyEx(RootKey,
                                lpSubKey,
                                0,
                                KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                                &hkResult);

    // 2. 循环遍历Uninstall目录下的子键
    DWORD dwIndex = 0;
    DWORD dwCount = 0;
    while(true)
    {

        DWORD dwKeyLen = 255;
        DWORD dwType = 0;
        WCHAR szNewKeyName[MAX_PATH] = {};
        LONG lReturn = RegEnumKeyEx(hkResult,
                                    dwIndex,
                                    szNewKeyName,
                                    &dwKeyLen,
                                    0,
                                    NULL,
                                    NULL,
                                    NULL);


        if(lReturn != 0)
        {
            break;
        }

        WCHAR strMidReg[MAX_PATH] = {};
        swprintf_s(strMidReg, L"%s%s%s",lpSubKey,L"\\",szNewKeyName);

        HKEY hkValueKey = 0;
        RegOpenKeyEx(RootKey, strMidReg, 0, KEY_QUERY_VALUE, &hkValueKey);

        DWORD dwNameLen = 255;
        PSOFTINFO m_SoftInfo = new SOFTINFO{0};
        RegQueryValueEx(hkValueKey, L"DisplayName", 0, &dwType, (LPBYTE)m_SoftInfo->szSoftName, &dwNameLen);
        dwNameLen = 255;
        RegQueryValueEx(hkValueKey, L"UninstallString",0,&dwType,(LPBYTE)m_SoftInfo->strSoftUniPath, &dwNameLen);

        QString displayName = QString::fromWCharArray(m_SoftInfo->szSoftName);

        if(displayName.size() == 0)
        {
            dwNameLen = 255;
            dwIndex++;
            continue;
        }

        ui->table_software->setRowCount(dwCount+1);
        ui->table_software->setItem(dwCount,0,new QTableWidgetItem(QString::fromWCharArray(m_SoftInfo->szSoftName)));

        m_vectSoftInfo.push_back(*m_SoftInfo);

        dwNameLen = 255;
        dwIndex++;
        dwCount++;
    }

    ui->table_software->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->table_software->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void Dialog::rightClick(QPoint pos)
{
    popMenu->addAction(removeAction);
    popMenu->exec(QCursor::pos());
}

void Dialog::StartRightClick(QPoint)
{
    StartingMenu->addAction(removeStarting);
    StartingMenu->exec(QCursor::pos());
}

void Dialog::rightClickedRemoveOperation()
{
        //do something
    int row = ui->table_software->currentRow();
    //qDebug() << "软件名" << QString::fromWCharArray(m_vectSoftInfo[row].szSoftName);
    //qDebug() << "卸载路径" << QString::fromWCharArray(m_vectSoftInfo[row].strSoftUniPath);

    if(wcscmp(m_vectSoftInfo[row].strSoftUniPath,L"")==0)
    {
        QMessageBox::information(NULL,"Info","找不到该程序的卸载路径，改程序可能已经被删除！");
        return;
    }

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = TRUE;
    //qDebug()<<"1"<<GetLastError();
    CreateProcessW(NULL,m_vectSoftInfo[row].strSoftUniPath,NULL,NULL,FALSE,CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    //qDebug()<<"2"<<GetLastError();

    if(GetLastError()==2)
    {
        QMessageBox::information(NULL,"Info","找不到该程序的卸载路径，改程序可能已经被删除！");
        return;
    }

    UninstallList();
}

void Dialog::rightClickedRemoveStartingOperation()
{
    int row = ui->table_startup->currentRow();
    //qDebug() << "启动项" << QString::fromWCharArray(m_vectStartUpInfo[row].lpKeyValueName);
    HKEY hSubKey;
    RegOpenKeyEx(m_vectStartUpInfo[row].rootKey,m_vectStartUpInfo[row].lpSubKey,0,KEY_SET_VALUE,&hSubKey);
    RegDeleteValue(hSubKey,m_vectStartUpInfo[row].lpKeyValueName);
    StartUpManage();
}
