#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    pVirusDialog->drivers =  pCleanDialog->allDriver;

    QMovie *movie = new QMovie("./cat.gif");
    ui->label->setMovie(movie);
    ui->label->show();
    movie->start();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_button_PTM_show_clicked()
{
    this->hide();
    pPTMDialog->show();
    pPTMDialog->exec();
    this->show();
}

void MainWindow::on_button_CPUMM_show_clicked()
{
    this->hide();
    pCPUMMDialog->show();
    pCPUMMDialog->exec();
    this->show();
}

void MainWindow::on_button_Clean_show_clicked()
{
    this->hide();
    pCleanDialog->show();
    pCleanDialog->exec();
    this->show();
}

void MainWindow::on_button_Service_show_clicked()
{
    this->hide();
    pServiceDialog->show();
    pServiceDialog->exec();
    this->show();
}

void MainWindow::on_button_PE_show_clicked()
{
    this->hide();
    pPEDialog->show();
    pPEDialog->exec();
    this->show();
}

void MainWindow::on_button_Virus_show_clicked()
{
    this->hide();
    pVirusDialog->show();
    pVirusDialog->exec();
    this->show();
}

void MainWindow::on_button_other_show_clicked()
{
    this->hide();
    pDialog->show();
    pDialog->exec();
    this->show();
}
