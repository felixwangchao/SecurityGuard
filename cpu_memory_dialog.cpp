#include "cpu_memory_dialog.h"
#include "ui_cpu_memory_dialog.h"

QStringList get_cpu_info();
QStringList get_pyMM_info();

CPU_MEMORY_Dialog::CPU_MEMORY_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPU_MEMORY_Dialog)
{
    ui->setupUi(this);

    // 初始化gif
    movie = new QMovie("./rocket.gif");
    ui->label_rocket->setMovie(movie);
    connect(movie,SIGNAL(frameChanged(int)),this,SLOT(Stopframe(int)));
    ui->label_rocket->show();

    // 初始化动画
    animation = new QPropertyAnimation(ui->label_rocket,"geometry");
    animation->setDuration(4000);
    animation->setStartValue(QRect(0, 380, 611, 371));
    animation->setEndValue(QRect(0, 0, 611, 371));

    // 初始化progressbar线程
    ProgressBarThread* pThread = new ProgressBarThread();
    connect(pThread,SIGNAL(updateProgressBar_1(int)),this,SLOT(OnNotify_1(int)));
    connect(pThread,SIGNAL(updateProgressBar_2(int)),this,SLOT(OnNotify_2(int)));
    connect(pThread,SIGNAL(updateLabel_1(QString)),this,SLOT(OnLabel_1(QString)));
    connect(pThread,SIGNAL(updateLabel_2(QString)),this,SLOT(OnLabel_2(QString)));
    pThread->start();
}

CPU_MEMORY_Dialog::~CPU_MEMORY_Dialog()
{
    delete ui;
}

double timeChange(FILETIME f)
{
    return (double)(f.dwHighDateTime*4.294967296E9)+(double)f.dwLowDateTime;
}


// 显示cpu使用率
QStringList get_cpu_info()
{
    QStringList msg;

    // 第一次获取处理器时间
    FILETIME preidleTime, prekernelTime, preuserTime;
    GetSystemTimes(&preidleTime, &prekernelTime, &preuserTime);

    Sleep(1000);


    // 第二次获得处理器时间
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    // 获取转换后的时间
    double itime,ktime,utime,pitime,pktime,putime;
    itime =     timeChange(idleTime);
    ktime =     timeChange(kernelTime);
    utime =     timeChange(userTime);
    pitime =    timeChange(preidleTime);
    pktime =    timeChange(prekernelTime);
    putime =    timeChange(preuserTime);

    // 计算CPU使用率
    int dbCpuUsage = (int)(100.0 - (itime - pitime)/(ktime - pktime + utime - putime)*100.0);
    // 返回使用率

    msg << QString("%1").arg(dbCpuUsage)
        << QString("CPU使用率 %1").arg(dbCpuUsage)
        << QString("抽样间隔 1秒");

    return msg;
}

QStringList get_pyMM_info()
{
    QStringList msg;
    MEMORYSTATUSEX stcMemStatusEx = { 0 };
    stcMemStatusEx.dwLength = sizeof(stcMemStatusEx);
    GlobalMemoryStatusEx(&stcMemStatusEx);

    //progress_mm->setValue(stcMemStatusEx.dwMemoryLoad);
    msg <<  QString("%1").arg(stcMemStatusEx.dwMemoryLoad)
        <<  QString("内存使用率：%1\%").arg(stcMemStatusEx.dwMemoryLoad)
        <<  QString("物理内存总量：%1MB").arg((int)(stcMemStatusEx.ullTotalPhys/1024/1024))
        <<  QString("可使用物理内存：%1MB").arg((int)(stcMemStatusEx.ullAvailPhys/1024/1024))
        <<  QString("已使用的物理内存：%1MB").arg((int)((stcMemStatusEx.ullTotalPhys - stcMemStatusEx.ullAvailPhys)/1024/1024));
    return msg;
}


void mm_optimize()
{
    DWORD dwPIDList[1000] = {0};
    DWORD bufSize = sizeof(dwPIDList);
    DWORD dwNeedSize = 0;
    EnumProcesses(dwPIDList,bufSize,&dwNeedSize);
    for(DWORD i=0; i < dwNeedSize / sizeof(DWORD); i++)
    {
        HANDLE hProcess = OpenProcess(PROCESS_SET_QUOTA,false,dwPIDList[i]);
        SetProcessWorkingSetSize(hProcess,-1,-1);
    }
}



void CPU_MEMORY_Dialog::on_button_accelerate_clicked()
{
    movie->start();
    animation->start();
    mm_optimize();
}

void CPU_MEMORY_Dialog::on_button_CPUMM_return_clicked()
{
    this->close();
}

void CPU_MEMORY_Dialog::Stopframe(int frame)
{
    if(frame == 52)
    {
        movie->stop();
    }
}

void CPU_MEMORY_Dialog::OnNotify_1(int i)
{
    ui->progressBar_1->setValue(i);
}

void CPU_MEMORY_Dialog::OnNotify_2(int i)
{
    ui->progressBar_2->setValue(i);
}

void CPU_MEMORY_Dialog::OnLabel_1(QString msg)
{
    ui->label_cpuUsage->setText(msg);
}

void CPU_MEMORY_Dialog::OnLabel_2(QString msg)
{
    ui->label_mmUsage->setText(msg);
}

ProgressBarThread::ProgressBarThread()
{
    qDebug()<<"Thread init!";
}

void ProgressBarThread::run()
{
    qDebug()<<"Thread start!";
    while(true)
    {
        QString msg;
        QStringList cpu = get_cpu_info();
        QStringList mm = get_pyMM_info();
        emit updateProgressBar_1(cpu.at(0).toInt());
        emit updateProgressBar_2(mm.at(0).toInt());

        QStringList tmp;

        for(int i=1; i<cpu.size();i++)
        {
            tmp << cpu.at(i);
        }
        msg = tmp.join('\n');
        emit updateLabel_1(msg);

        tmp.clear();

        for(int i=1; i<mm.size();i++)
        {
            tmp << mm.at(i);
        }
        msg = tmp.join('\n');
        emit updateLabel_2(msg);
    }
}
