#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include <QThread>

class MultiThread : public QThread
{
    Q_OBJECT
public:
    MultiThread();
    void run();
 };

#endif // MULTITHREAD_H
