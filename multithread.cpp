#include <QDebug>
MultiThread::MultiThread()
{

}

void MultiThread::run()
{
     while (true) {
         qDebug()<<"CSimpleThread run!";
         sleep(5);
     }
 }
