#include "mainwindow.h"
#include "ui_mainwindow.h"

#include<QToolBar>
#include<QToolButton>
#include<QLabel>

class sinfile
{
public:
    QString filename,filepath,context;
    int fileindex;
};

void MainWindow::fib_clicked()
{
    ui->groupBox->setHidden(!ui->groupBox->isHidden());
    ui->groupBox_2->hide();
    ui->groupBox_3->hide();
}

void MainWindow::edb_clicked()
{
    ui->groupBox_2->setHidden(!ui->groupBox_2->isHidden());
    ui->groupBox->hide();
    ui->groupBox_3->hide();
}
void MainWindow::vib_clicked()
{
    ui->groupBox_3->setHidden(!ui->groupBox_3->isHidden());
    ui->groupBox->hide();
    ui->groupBox_2->hide();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->groupBox->hide();
    ui->groupBox->setGeometry(0,0,100,110);
    ui->groupBox_2->hide();
    ui->groupBox_2->setGeometry(35,0,110,90);
    ui->groupBox_3->hide();
    ui->groupBox_3->setGeometry(70,0,180,120);

    QToolButton *fib=new QToolButton(this);
    QToolButton *edb=new QToolButton(this);
    QToolButton *vib=new QToolButton(this);
    QToolButton *fileft=new QToolButton(this);
    QToolButton *firight=new QToolButton(this);
    QLabel *l1=new QLabel(this);

    fib->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    edb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    vib->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    fib->resize(60,20);
    edb->resize(60,20);
    vib->resize(60,20);

    fib->setText("File");
    edb->setText("Edit");
    vib->setText("View");
    l1->setText("file1: hullo.txt");
    fileft->setText("<");
    firight->setText(">");

    ui->toolBar->addWidget(fib);
    ui->toolBar->addWidget(edb);
    ui->toolBar->addWidget(vib);
    ui->toolBar->addWidget(fileft);
    ui->toolBar->addWidget(firight);
    ui->toolBar->addWidget(l1);
    ui->toolBar->setMovable(false);

    connect(fib,&QToolButton::clicked,this,[=](){fib_clicked();});
    connect(edb,&QToolButton::clicked,this,[=](){edb_clicked();});
    connect(vib,&QToolButton::clicked,this,[=](){vib_clicked();});
}


MainWindow::~MainWindow()
{
    delete ui;
}
