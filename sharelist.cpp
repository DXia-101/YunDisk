#include "sharelist.h"
#include "ui_sharelist.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFileDialog>
#include "common/logininfoinstance.h"
#include "common/downloadtask.h"
#include "selfwidget/filepropertyinfo.h"

ShareList::ShareList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ShareList)
{
    ui->setupUi(this);
    initListWidget();
    addActionMenu();
    m_manager = Common::getNetManager();

    connect(&m_downloadTimer, &QTimer::timeout, [=]()
    {
        downloadFilesAction();
    });

    m_downloadTimer.start(500);
}

ShareList::~ShareList()
{
    clearshareFileList();
    clearItems();

    delete ui;
}

void ShareList::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);  
    ui->listWidget->setIconSize(QSize(80, 80));        
    ui->listWidget->setGridSize(QSize(100, 120));      

    ui->listWidget->setResizeMode(QListView::Adjust);

    ui->listWidget->setMovement(QListView::Static);

    ui->listWidget->setSpacing(30);
    ui->listWidget->setContextMenuPolicy( Qt::CustomContextMenu );
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, [=](const QPoint& pos)
    {
        QListWidgetItem *item = ui->listWidget->itemAt( pos );

        if( item == NULL ) 
        {
            m_menuEmpty->exec( QCursor::pos() ); 
        }
        else 
        {
            ui->listWidget->setCurrentItem(item);
            m_menuItem->exec( QCursor::pos() );
        }
    });
}

void ShareList::addActionMenu()
{
    m_menuItem = new MyMenu( this );

    m_downloadAction = new QAction("下载",this);
    m_propertyAction = new QAction("属性",this);
    m_cancelAction = new QAction("取消分享",this);
    m_saveAction = new QAction("转存文件",this);

    m_menuItem->addAction(m_downloadAction);
    m_menuItem->addAction(m_propertyAction);
    m_menuItem->addAction(m_cancelAction);
    m_menuItem->addAction(m_saveAction);

    m_menuEmpty = new MyMenu( this );
    m_refreshAction = new QAction("刷新", this);
    m_menuEmpty->addAction(m_refreshAction);

    connect(m_downloadAction, &QAction::triggered, [=]() 
    {
        cout << "下载动作";
        addDownloadFiles();
    });

    connect(m_propertyAction, &QAction::triggered, [=]()   
    {
        cout << "属性动作";
        dealSelectdFile(Property);
    });

    connect(m_cancelAction, &QAction::triggered, [=]()
    {
        cout << "取消分享";
        dealSelectdFile(Cancel);
    });

    connect(m_saveAction, &QAction::triggered, [=]() 
    {
        cout << "转存文件";
        dealSelectdFile(Save);
    });

    connect(m_refreshAction, &QAction::triggered, [=]() 
    {
        cout << "刷新动作";
        refreshFiles();
    });
}

void ShareList::clearshareFileList()
{
    int n = m_shareFileList.size();
    for(int i = 0; i < n; ++i)
    {
        FileInfo *tmp = m_shareFileList.takeFirst();
        delete tmp;
    }
}

void ShareList::clearItems()
{
    int n = ui->listWidget->count();
    for(int i = 0; i < n; ++i)
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0); 
        delete item;
    }
}

void ShareList::refreshFileItems()
{
    clearItems();

    int n = m_shareFileList.size();
    for(int i = 0; i < n; ++i)
    {
        FileInfo *tmp = m_shareFileList.at(i);
        QListWidgetItem *item = tmp->item;
        ui->listWidget->addItem(item);
    }
}

void ShareList::refreshFiles()
{
    m_userFilesCount = 0;
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 
    QString url = QString("http:// %1:%2/sharefiles?cmd=count").arg(login->getIp()).arg(login->getPort());

    QNetworkReply * reply = m_manager->get( QNetworkRequest( QUrl(url)) );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) 
        {
            cout << reply->errorString();
            reply->deleteLater(); 
            return;
        }
        QByteArray array = reply->readAll();

        reply->deleteLater();
        clearshareFileList();
        m_userFilesCount = array.toLong();
        cout << "userFilesCount = " << m_userFilesCount;
        if(m_userFilesCount > 0)
        {
            m_start = 0;  
            m_count = 10; 
            getUserFilesList();
        }
        else
        {
            refreshFileItems(); 
        }
    });
}

QByteArray ShareList::setFilesListJson(int start, int count)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("start", start);
    tmp.insert("count", count);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }

    return jsonDocument.toJson();
}

void ShareList::getUserFilesList()
{
    if(m_userFilesCount <= 0)
    {
        cout << "获取共享文件列表条件结束";
        refreshFileItems();
        return;
    }
    else if(m_count > m_userFilesCount)
    {
        m_count = m_userFilesCount;
    }

    QNetworkRequest request;
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 
    QString url;
    url = QString("http:// %1:%2/sharefiles?cmd=normal").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); 
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray data = setFilesListJson(m_start, m_count);

    m_start += m_count;
    m_userFilesCount -= m_count;
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) 
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }
        QByteArray array = reply->readAll();

        reply->deleteLater();
        if("015" != m_cm.getCode(array) ) 
        {
            getFileJsonInfo(array);
            getUserFilesList();
        }
    });
}

void ShareList::getFileJsonInfo(QByteArray data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error == QJsonParseError::NoError)
    {
        if (doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return;
        }

        if( doc.isObject())
        {
            QJsonObject obj = doc.object();
            QJsonArray array = obj.value("files").toArray();

            int size = array.size();  
            cout << "size = " << size;

            for(int i = 0; i < size; ++i)
            {
                QJsonObject tmp = array[i].toObject(); 
                FileInfo *info = new FileInfo;
                info->user = tmp.value("user").toString(); 
                info->md5 = tmp.value("md5").toString();
                info->time = tmp.value("time").toString(); 
                info->filename = tmp.value("filename").toString(); 
                info->shareStatus = tmp.value("share_status").toInt(); 
                info->pv = tmp.value("pv").toInt(); 
                info->url = tmp.value("url").toString(); 
                info->size = tmp.value("size").toInt(); 
                info->type = tmp.value("type").toString();
                QString type = info->type + ".png";
                info->item = new QListWidgetItem(QIcon( m_cm.getFileType(type) ), info->filename);
               
                m_shareFileList.append(info);
            }
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }
}

void ShareList::addDownloadFiles()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == NULL)
    {
        cout << "item == NULL";
        return;
    }
    emit gotoTransfer(TransferStatus::Download);
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    for(int i = 0; i < m_shareFileList.size(); ++i)
    {
        if(m_shareFileList.at(i)->item == item)
        {

            QString filePathName = QFileDialog::getSaveFileName(this, "选择保存文件路径", m_shareFileList.at(i)->filename );
            if(filePathName.isEmpty())
            {
                cout << "filePathName.isEmpty()";
                return;
            }
            int res = p->appendDownloadList(m_shareFileList.at(i), filePathName, true);
            if(res == -1)
            {
                QMessageBox::warning(this, "任务已存在", "任务已经在下载队列中！！！");
            }
            else if(res == -2) 
            {
                LoginInfoInstance *login = LoginInfoInstance::getInstance();
                m_cm.writeRecord(login->getUser(), m_shareFileList.at(i)->filename, "010");
            }
            break;
        }
    }
}


void ShareList::downloadFilesAction()
{
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    if( p->isEmpty() )
    {
        return;
    }

    if( p->isDownload() )
    {
        return;
    }
    if(p->isShareTask() == false)
    {
        return;
    }

    DownloadInfo *tmp = p->takeTask();
    QUrl url = tmp->url;
    QFile *file = tmp->file;
    QString md5 = tmp->md5;
    QString filename = tmp->filename;
    DataProgress *dp = tmp->dp;

    QNetworkReply * reply = m_manager->get( QNetworkRequest(url) );
    if(reply == NULL)
    {
        p->dealDownloadTask();
        cout << "get err";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        cout << "下载完成";
        reply->deleteLater();
        p->dealDownloadTask();
        LoginInfoInstance *login = LoginInfoInstance::getInstance();
        m_cm.writeRecord(login->getUser(), filename, "010");

        dealFilePv(md5, filename);
    });
    connect(reply, &QNetworkReply::readyRead, [=]()
    {
        if (file != NULL)
        {
            file->write(reply->readAll());
        }
    });
    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesRead, qint64 totalBytes)
    {
        dp->setProgress(bytesRead, totalBytes);
    });
}

QByteArray ShareList::setShareFileJson(QString user, QString md5, QString filename)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("md5", md5);
    tmp.insert("filename", filename);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }

    return jsonDocument.toJson();
}

void ShareList::dealFilePv(QString md5, QString filename)
{
    QNetworkRequest request;
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 
    QString url = QString("http:// %1:%2/dealsharefile?cmd=pv").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); 
    cout << "url = " << url;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray data = setShareFileJson( login->getUser(),  md5, filename); 
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) 
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }
        QByteArray array = reply->readAll();

        reply->deleteLater();
        if("016" == m_cm.getCode(array) )
        {
            for(int i = 0; i < m_shareFileList.size(); ++i)
            {
                FileInfo *info = m_shareFileList.at(i);
                if( info->md5 == md5 && info->filename == filename)
                {
                    int pv = info->pv;
                    info->pv = pv+1;
                    break; 
                }
            }
        }
        else
        {
            cout << "下载文件pv字段处理失败";
        }
    });
}
void ShareList::dealSelectdFile(ShareList::CMD cmd)
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == NULL)
    {
        return;
    }
    for(int i = 0; i < m_shareFileList.size(); ++i)
    {
        if(m_shareFileList.at(i)->item == item)
        {
            if(cmd == Property)
            {
                getFileProperty( m_shareFileList.at(i) ); 
            }
            else if(cmd == Cancel)
            {
                cancelShareFile( m_shareFileList.at(i) );
            }
            else if(cmd == Save)
            {
                saveFileToMyList( m_shareFileList.at(i) );
            }

            break; 
        }
    }
}

void ShareList::getFileProperty(FileInfo *info)
{
    FilePropertyInfo dlg; 
    dlg.setInfo(info);
    dlg.exec();         
}
void ShareList::cancelShareFile(FileInfo *info)
{
    LoginInfoInstance *login = LoginInfoInstance::getInstance();
    if(login->getUser() != info->user)
    {
        QMessageBox::warning(this, "操作失败", "此文件不是当前登陆用户分享，无法取消分享！！！");
        return;
    }
    QNetworkRequest request;
    QString url = QString("http:// %1:%2/dealsharefile?cmd=cancel").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    cout << "url = " << url;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray data = setShareFileJson( login->getUser(),  info->md5, info->filename); // 设置json包

    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) 
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();

        reply->deleteLater();
        if("018" == m_cm.getCode(array) )
        {
            for(int i = 0; i < m_shareFileList.size(); ++i)
            {
                if( m_shareFileList.at(i) == info)
                {
                    QListWidgetItem *item = info->item;
                    ui->listWidget->removeItemWidget(item);
                    delete item;

                    m_shareFileList.removeAt(i);
                    delete info;
                    break;     
                }
            }

            QMessageBox::information(this, "操作成功", "此文件已取消分享！！！");
        }
        else
        {
            QMessageBox::warning(this, "操作失败", "取消分享文件操作失败！！！");
        }
    });
}

void ShareList::saveFileToMyList(FileInfo *info)
{
    LoginInfoInstance *login = LoginInfoInstance::getInstance();
    if(login->getUser() == info->user)
    {
        QMessageBox::warning(this, "操作失败", "此文件本就属于该用户，无需转存！！！");
        return;
    }

    QNetworkRequest request; 
    QString url = QString("http:// %1:%2/dealsharefile?cmd=save").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); 
    cout << "url = " << url;

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    QByteArray data = setShareFileJson( login->getUser(),  info->md5, info->filename); // 设置json包

    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) 
        {
            cout << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();

        reply->deleteLater();
        if("020" == m_cm.getCode(array) ) {
            QMessageBox::information(this, "操作成功", "该文件已保存到该用户列表中！！！");
        }
        else if("021" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "操作失败", "此文件已存在，无需转存！！！");
        }
        else if("022" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "操作失败", "文件转存失败！！！");
        }
    });
}
