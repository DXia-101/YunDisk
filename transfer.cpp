#include "transfer.h"
#include "ui_transfer.h"
#include "common/downloadlayout.h"
#include "common/uploadlayout.h"
#include "common/logininfoinstance.h"
#include <QFile>

Transfer::Transfer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Transfer)
{
    ui->setupUi(this);

    UploadLayout *uploadLayout = UploadLayout::getInstance();
    uploadLayout->setUploadLayout(ui->upload_scroll);

    DownloadLayout *downloadLayout = DownloadLayout::getInstance();
    downloadLayout->setDownloadLayout(ui->download_scroll);

    ui->tabWidget->setCurrentIndex(0);

    connect(ui->tabWidget, &QTabWidget::currentChanged, [=](int index)
    {
        if(index == 0) 
        {
            emit currentTabSignal("正在上传");
        }
        else if(index == 1)
        {
             emit currentTabSignal("正在下载");
        }
        else 
        {
            emit currentTabSignal("传输记录");
            dispayDataRecord(); 
        }
    });
    
    ui->tabWidget->tabBar()->setStyleSheet(
       "QTabBar::tab{"
       "background-color: rgb(182, 202, 211);"
       "border-right: 1px solid gray;"
       "padding: 6px"
       "}"
       "QTabBar::tab:selected, QtabBar::tab:hover {"
       "background-color: rgb(20, 186, 248);"
       "}"
    );

    connect(ui->clearBtn, &QToolButton::clicked, [=]()
    {
        LoginInfoInstance *login = LoginInfoInstance::getInstance(); //获取单例

        QString fileName = RECORDDIR + login->getUser();

        if( QFile::exists(fileName) ) //如果文件存在
        {
             QFile::remove(fileName); //删除文件
             ui->record_msg->clear();
        }
    });
}

Transfer::~Transfer()
{
    delete ui;
}

void Transfer::dispayDataRecord(QString path)
{
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 


    QString fileName = path + login->getUser();
    QFile file(fileName);

    if( false == file.open(QIODevice::ReadOnly) ) 
    {
        cout << "file.open(QIODevice::ReadOnly) err";
        return;
    }

    QByteArray array = file.readAll();

    #ifdef _WIN32 
        ui->record_msg->setText( QString::fromLocal8Bit(array) );
    #else 
        ui->record_msg->setText( array );
    #endif

        file.close();
}

void Transfer::showUpload()
{
    ui->tabWidget->setCurrentWidget(ui->upload);
}

void Transfer::showDownload()
{
    ui->tabWidget->setCurrentWidget(ui->download);
}
