#include "myfilewg.h"
#include "ui_myfilewg.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QListWidgetItem>
#include "common/logininfoinstance.h"
#include "selfwidget/filepropertyinfo.h"
#include "common/downloadtask.h"
#include <QHttpMultiPart>
#include <QHttpPart>

MyFileWg::MyFileWg(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MyFileWg)
{
    ui->setupUi(this);
    initListWidget();
    addActionMenu();
    m_manager = Common::getNetManager();
    checkTaskList();
}

MyFileWg::~MyFileWg()
{
    delete ui;
}

void MyFileWg::initListWidget()
{
    ui->listWidget->setViewMode(QListView::IconMode);  
    ui->listWidget->setIconSize(QSize(80, 80));        
    ui->listWidget->setGridSize(QSize(100, 120));      
    ui->listWidget->setResizeMode(QListView::Adjust);  
    ui->listWidget->setMovement(QListView::Static);
    ui->listWidget->setSpacing(30);
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, this, &MyFileWg::rightMenu);
    connect(ui->listWidget, &QListWidget::itemPressed, this, [=](QListWidgetItem* item)
    {
        QString str = item->text();
        if(str == "上传文件")
        {
            addUploadFiles();
        }
    });
}

void MyFileWg::addActionMenu()
{
    m_menu = new MyMenu( this );
    m_downloadAction = new QAction("下载", this);
    m_shareAction = new QAction("分享", this);
    m_delAction = new QAction("删除", this);
    m_propertyAction = new QAction("属性", this);
    m_menu->addAction(m_downloadAction);
    m_menu->addAction(m_shareAction);
    m_menu->addAction(m_delAction);
    m_menu->addAction(m_propertyAction);
    m_menuEmpty = new MyMenu( this );
    m_pvAscendingAction = new QAction("按下载量升序", this);
    m_pvDescendingAction = new QAction("按下载量降序", this);
    m_refreshAction = new QAction("刷新", this);
    m_uploadAction = new QAction("上传", this);
    m_menuEmpty->addAction(m_pvAscendingAction);
    m_menuEmpty->addAction(m_pvDescendingAction);
    m_menuEmpty->addAction(m_refreshAction);
    m_menuEmpty->addAction(m_uploadAction);
    connect(m_downloadAction, &QAction::triggered, [=]()
    {
        cout << "下载动作";
        addDownloadFiles();
    });
    connect(m_shareAction, &QAction::triggered, [=]()
    {
        cout << "分享动作";
        dealSelectdFile("分享");
    });
    connect(m_delAction, &QAction::triggered, [=]()
    {
        cout << "删除动作";
        dealSelectdFile("删除");
    });
    connect(m_propertyAction, &QAction::triggered, [=]()
    {
        cout << "属性动作";
        dealSelectdFile("属性");
    connect(m_pvAscendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量升序";
        refreshFiles(PvAsc);
    });


    connect(m_pvDescendingAction, &QAction::triggered, [=]()
    {
        cout << "按下载量降序";
        refreshFiles(PvDesc);
    });
    connect(m_refreshAction, &QAction::triggered, [=]()
    {
        cout << "刷新动作";
        refreshFiles();
    });
    connect(m_uploadAction, &QAction::triggered, [=]()
    {
        cout << "上传动作";
        addUploadFiles();
    });
}
void MyFileWg::addUploadFiles()
{
    emit gotoTransfer(TransferStatus::Uplaod);
    UploadTask *uploadList = UploadTask::getInstance();
    if(uploadList == NULL)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }

    QStringList list = QFileDialog::getOpenFileNames();
    for(int i = 0; i < list.size(); ++i)
    {
        int res = uploadList->appendUploadList(list.at(i));
        if(res == -1)
        {
            QMessageBox::warning(this, "文件太大", "文件大小不能超过30M！！！");
        }
        else if(res == -2)
        {
            QMessageBox::warning(this, "添加失败", "上传的文件是否已经在上传队列中！！！");
        }
        else if(res == -3)
        {
            cout << "打开文件失败";
        }
        else if(res == -4)
        {
            cout << "获取布局失败";
        }
    }
}
QByteArray MyFileWg::setMd5Json(QString user, QString token, QString md5, QString fileName)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);
    tmp.insert("md5", md5);
    tmp.insert("fileName", fileName);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    return jsonDocument.toJson();
}

void MyFileWg::uploadFilesAction()
{
    UploadTask *uploadList = UploadTask::getInstance();
    if(uploadList == NULL)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }
    if( uploadList->isEmpty() )
    {
        return;
    }
    if( uploadList->isUpload() )
    {
        return;
    }
    LoginInfoInstance *login = LoginInfoInstance::getInstance();
    QNetworkRequest request;
    QString url = QString("http://%1:%2/md5").arg( login->getIp() ).arg( login->getPort() );
    request.setUrl( QUrl( url )); 
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    UploadFileInfo *info = uploadList->takeTask();
    QByteArray array = setMd5Json(login->getUser(), login->getToken(), info->md5, info->fileName);

    QNetworkReply *reply = m_manager->post(request, array);
    if(reply == NULL)
    {
        cout << "reply is NULL";
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
        if("006" == m_cm.getCode(array) )
        {
            m_cm.writeRecord(login->getUser(), info->fileName, "006");
            uploadList->dealUploadTask();

        }
        else if("007" == m_cm.getCode(array) )
        {
            uploadFile(info);
        }
        else if("005" == m_cm.getCode(array) )
        {
            m_cm.writeRecord(login->getUser(), info->fileName, "005");

            uploadList->dealUploadTask();
        }
        else if("111" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); 
            return;
        }
    });
}
void MyFileWg::uploadFile(UploadFileInfo *info)
{
    QFile *file = info->file;           
    QString fileName = info->fileName;  
    QString md5 = info->md5;            
    qint64 size = info->size;           
    DataProgress *dp = info->dp;        
    QString boundary = m_cm.getBoundary();  
    LoginInfoInstance *login = LoginInfoInstance::getInstance();
#if 0
    QByteArray data;
    data.append(boundary);
    data.append("\r\n");
    data.append("Content-Disposition: form-data; ");
    data.append( QString("user=\"%1\" ").arg( login->getUser() ) );
    data.append( QString("filename=\"%1\" ").arg(fileName) );
    data.append( QString("md5=\"%1\" ").arg(md5) ); 
    data.append( QString("size=%1").arg(size)  );  
    data.append("\r\n");

    data.append("Content-Type: application/octet-stream");
    data.append("\r\n");
    data.append("\r\n");
    data.append( file->readAll() );
    data.append("\r\n");
    data.append(boundary);
#endif
    QHttpPart part;
    QString disp = QString("form-data; user=\"%1\"; filename=\"%2\"; md5=\"%3\"; size=%4")
            .arg(login->getUser()).arg(info->fileName).arg(info->md5).arg(info->size);
    part.setHeader(QNetworkRequest::ContentDispositionHeader, disp);
    part.setHeader(QNetworkRequest::ContentTypeHeader, "image/png"); 
    part.setBodyDevice(info->file);
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
    multiPart->append(part);


    QNetworkRequest request; 
    QString url = QString("http://%1:%2/upload").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data");
    QNetworkReply * reply = m_manager->post( request, multiPart );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    connect(reply, &QNetworkReply::uploadProgress, [=](qint64 bytesRead, qint64 totalBytes)
    {
        if(totalBytes != 0) 
        {
            dp->setProgress(bytesRead/1024, totalBytes/1024);
        }
    });
    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) 
        {
            cout << reply->errorString();
            reply->deleteLater(); 
            m_cm.writeRecord(login->getUser(), info->fileName, "009");
            UploadTask *uploadList = UploadTask::getInstance();
            uploadList->dealUploadTask(); 
            return;
        }

        QByteArray array = reply->readAll();

        reply->deleteLater();
        // 析构对象
        multiPart->deleteLater();
        if("008" == m_cm.getCode(array) )
        {
            m_cm.writeRecord(login->getUser(), info->fileName, "008");
        }
        else if("009" == m_cm.getCode(array) )
        {
            m_cm.writeRecord(login->getUser(), info->fileName, "009");
        }
        UploadTask *uploadList = UploadTask::getInstance();
        if(uploadList == NULL)
        {
            cout << "UploadTask::getInstance() == NULL";
            return;
        }

        uploadList->dealUploadTask();
    }
    );
}
void MyFileWg::clearFileList()
{
    int n = m_fileList.size();
    for(int i = 0; i < n; ++i)
    {
        FileInfo *tmp = m_fileList.takeFirst();
        delete tmp;
    }
}
void MyFileWg::clearItems()
{
    int n = ui->listWidget->count();
    for(int i = 0; i < n; ++i)
    {
        QListWidgetItem *item = ui->listWidget->takeItem(0); //这里是0，不是i
        delete item;
    }
}

void MyFileWg::addUploadItem(QString iconPath, QString name)
{
    ui->listWidget->addItem(new QListWidgetItem(QIcon(iconPath), name));
}

void MyFileWg::refreshFileItems()
{
    clearItems();

    if(m_fileList.isEmpty() == false)
    {
        int n = m_fileList.size(); 
        for(int i = 0; i < n; ++i)
        {
            FileInfo *tmp = m_fileList.at(i);
            QListWidgetItem *item = tmp->item;
            //list widget add item
            ui->listWidget->addItem(item);
        }
    }

    //添加上传文件item
    this->addUploadItem();
}
QStringList MyFileWg::getCountStatus(QByteArray json)
{
    QJsonParseError error;
    QStringList list;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (error.error == QJsonParseError::NoError) //没有出错
    {
        if (doc.isNull() || doc.isEmpty())
        {
            cout << "doc.isNull() || doc.isEmpty()";
            return list;
        }

        if( doc.isObject() )
        {
            QJsonObject obj = doc.object();
            list.append( obj.value( "token" ).toString() ); 
            list.append( obj.value( "num" ).toString() );
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }

    return list;
}
void MyFileWg::refreshFiles(MyFileWg::Display cmd)
{
    m_userFilesCount = 0;

    QNetworkRequest request; 
    LoginInfoInstance *login = LoginInfoInstance::getInstance();
    QString url = QString("http://%1:%2/myfiles?cmd=count").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray data = setGetCountJson( login->getUser(), login->getToken());
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
        cout << "server return file ...: " << array;

        reply->deleteLater();
        QStringList list = getCountStatus(array);
        if( list.at(0) == "111" )
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");

            emit loginAgainSignal(); 

            return; 
        }
        m_userFilesCount = list.at(1).toLong();
        cout << "userFilesCount = " << m_userFilesCount;
        clearFileList();

        if(m_userFilesCount > 0)
        {
            m_start = 0; 
            m_count = 10;
            getUserFilesList(cmd);
        }
        else //没有文件
        {
            refreshFileItems(); //更新item
        }
    });
}
QByteArray MyFileWg::setGetCountJson(QString user, QString token)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(tmp);
    if ( jsonDocument.isNull() )
    {
        cout << " jsonDocument.isNull() ";
        return "";
    }
    return jsonDocument.toJson();
}

QByteArray MyFileWg::setFilesListJson(QString user, QString token, int start, int count)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);
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
void MyFileWg::getUserFilesList(MyFileWg::Display cmd)
{
    if(m_userFilesCount <= 0) 
    {
        cout << "获取用户文件列表条件结束";
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

    QString tmp;
    if(cmd == Normal)
    {
        tmp = "normal";
    }
    else if(cmd == PvAsc)
    {
        tmp = "pvasc";
    }
    else if(cmd == PvDesc)
    {
        tmp = "pvdesc";
    }

    url = QString("http://%1:%2/myfiles?cmd=%3").arg(login->getIp()).arg(login->getPort()).arg(tmp);
    request.setUrl(QUrl( url )); //设置url
    cout << "myfiles url: " << url;
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray data = setFilesListJson( login->getUser(), login->getToken(), m_start, m_count);

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
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }
        QByteArray array = reply->readAll();
        cout << "file info:" << array;

        reply->deleteLater();

        if("111" == m_cm.getCode(array) ) //common.h
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); //发送重新登陆信号

            return; //中断
        }
        if("015" != m_cm.getCode(array) ) //common.h
        {
            getFileJsonInfo(array);

            getUserFilesList(cmd);
        }
    });
}

void MyFileWg::getFileJsonInfo(QByteArray data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error == QJsonParseError::NoError) //没有出错
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

            int size = array.size();   //数组个数
            cout << "size = " << size;

            for(int i = 0; i < size; ++i)
            {
                QJsonObject tmp = array[i].toObject(); //取第i个对象
                
                FileInfo *info = new FileInfo;
                info->user = tmp.value("user").toString(); //用户
                info->md5 = tmp.value("md5").toString(); //文件md5
                info->time = tmp.value("time").toString(); //上传时间
                info->filename = tmp.value("filename").toString(); //文件名字
                info->shareStatus = tmp.value("share_status").toInt(); //共享状态
                info->pv = tmp.value("pv").toInt(); //下载量
                info->url = tmp.value("url").toString(); //url
                info->size = tmp.value("size").toInt(); //文件大小，以字节为单位
                info->type = tmp.value("type").toString();//文件后缀
                QString type = info->type + ".png";
                info->item = new QListWidgetItem(QIcon( m_cm.getFileType(type) ), info->filename);

                //list添加节点
                m_fileList.append(info);
            }
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }
}
void MyFileWg::dealSelectdFile(QString cmd)
{
    //获取当前选中的item
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == NULL)
    {
        return;
    }

    //查找文件列表匹配的元素
    for(int i = 0; i < m_fileList.size(); ++i)
    {
        if(m_fileList.at(i)->item == item)
        {
            if(cmd == "分享")
            {
                shareFile( m_fileList.at(i) ); //分享某个文件
            }
            else if(cmd == "删除")
            {
                delFile( m_fileList.at(i) ); //删除某个文件
            }
            else if(cmd == "属性")
            {
                getFileProperty( m_fileList.at(i) ); //获取属性信息
            }
            break; //跳出循环
        }
    }
}

QByteArray MyFileWg::setDealFileJson(QString user, QString token, QString md5, QString filename)
{
    QMap<QString, QVariant> tmp;
    tmp.insert("user", user);
    tmp.insert("token", token);
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

// 分享文件
void MyFileWg::shareFile(FileInfo *info)
{
    if(info->shareStatus == 1) //已经分享，不能再分享
    {
        QMessageBox::warning(this, "此文件已经分享", "此文件已经分享!!!");
        return;
    }

    QNetworkRequest request; 
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 
    QString url = QString("http://%1:%2/dealfile?cmd=share").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); 
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    QByteArray data = setDealFileJson( login->getUser(),  login->getToken(), info->md5, info->filename);

    //发送post请求
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
        if("010" == m_cm.getCode(array) ) //common.h
        {
            //修改文件列表的属性信息
            info->shareStatus = 1; //设置此文件为已分享
            QMessageBox::information(this, "分享成功", QString("[%1] 分享成功!!!").arg(info->filename));
        }
        else if("011" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "分享失败", QString("[%1] 分享失败!!!").arg(info->filename));
        }
        else if("012" == m_cm.getCode(array))
        {
            QMessageBox::warning(this, "分享失败", QString("[%1] 别人已分享此文件!!!").arg(info->filename));
        }
        else if("111" == m_cm.getCode(array)) //token验证失败
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); //发送重新登陆信号

            return;
        }
    });
}

void MyFileWg::delFile(FileInfo *info)
{
    QNetworkRequest request; 
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 
    QString url = QString("http://%1:%2/dealfile?cmd=del").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray data = setDealFileJson( login->getUser(),  login->getToken(), info->md5, info->filename);

    //发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); 
            return;
        }
        QByteArray array = reply->readAll();

        reply->deleteLater();
        if("013" == m_cm.getCode(array) ) 
        {
            QMessageBox::information(this, "文件删除成功", QString("[%1] 删除成功!!!").arg(info->filename));

            for(int i = 0; i < m_fileList.size(); ++i)
            {
                if( m_fileList.at(i) == info)
                {
                    QListWidgetItem *item = info->item;
                    //从列表视图移除此item
                    ui->listWidget->removeItemWidget(item);
                    delete item;

                    m_fileList.removeAt(i);
                    delete info;
                    break; //很重要的中断条件
                }
            }
        }
        else if("014" == m_cm.getCode(array))
        {
            QMessageBox::information(this, "文件删除失败", QString("[%1] 删除失败!!!").arg(info->filename));
        }
        else if("111" == m_cm.getCode(array)) //token验证失败
        {
            QMessageBox::warning(this, "账户异常", "请重新登陆！！！");
            emit loginAgainSignal(); //发送重新登陆信号

            return; //中断
        }
    });
}

void MyFileWg::getFileProperty(FileInfo *info)
{
    FilePropertyInfo dlg; //创建对话框
    dlg.setInfo(info);

    dlg.exec(); 
}
void MyFileWg::addDownloadFiles()
{
    emit gotoTransfer(TransferStatus::Download);
    QListWidgetItem *item = ui->listWidget->currentItem();
    if(item == NULL)
    {
        cout << "item == NULL";
        return;
    }

    //获取下载列表实例
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    for(int i = 0; i < m_fileList.size(); ++i)
    {
        if(m_fileList.at(i)->item == item)
        {

            QString filePathName = QFileDialog::getSaveFileName(this, "选择保存文件路径", m_fileList.at(i)->filename );
            if(filePathName.isEmpty())
            {
                cout << "filePathName.isEmpty()";
                return;
            }
            int res = p->appendDownloadList(m_fileList.at(i), filePathName); //追加到下载列表
            if(res == -1)
            {
                QMessageBox::warning(this, "任务已存在", "任务已经在下载队列中！！！");
            }
            else if(res == -2) //打开文件失败
            {
                m_cm.writeRecord(m_fileList.at(i)->user, m_fileList.at(i)->filename, "010"); //下载文件失败，记录
            }

            break;
        }
    }
}

void MyFileWg::downloadFilesAction()
{
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    if( p->isEmpty() ) //如果队列为空，说明没有任务
    {
        return;
    }

    if( p->isDownload() ) 
    {
        return;
    }

    if(p->isShareTask() == true)
    {
        return;
    }

    DownloadInfo *tmp = p->takeTask(); //取下载任务


    QUrl url = tmp->url;
    QFile *file = tmp->file;
    QString md5 = tmp->md5;
    QString user = tmp->user;
    QString filename = tmp->filename;
    DataProgress *dp = tmp->dp;

    //发送get请求
    QNetworkReply * reply = m_manager->get( QNetworkRequest(url) );
    if(reply == NULL)
    {
        p->dealDownloadTask(); //删除任务
        cout << "get err";
        return;
    }

    connect(reply, &QNetworkReply::finished, [=]()
    {
        cout << "下载完成";
        reply->deleteLater();

        p->dealDownloadTask();//删除下载任务

        m_cm.writeRecord(user, filename, "010"); //下载文件成功，记录

        dealFilePv(md5, filename); 
    });

    connect(reply, &QNetworkReply::readyRead, [=]()
    {
        //如果文件存在，则写入文件
        if (file != NULL)
        {
            file->write(reply->readAll());
        }
    });

    //有可用数据更新时
    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesRead, qint64 totalBytes)
    {
        dp->setProgress(bytesRead, totalBytes);//设置进度
    }
    );
}

void MyFileWg::dealFilePv(QString md5, QString filename)
{
    QNetworkRequest request;
    LoginInfoInstance *login = LoginInfoInstance::getInstance(); 
    QString url = QString("http://%1:%2/dealfile?cmd=pv").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url )); 
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    QByteArray data = setDealFileJson( login->getUser(), login->getToken(), md5, filename); //设置json包

    //发送post请求
    QNetworkReply * reply = m_manager->post( request, data );
    if(reply == NULL)
    {
        cout << "reply == NULL";
        return;
    }

    connect(reply, &QNetworkReply::finished, [=]()
    {
        if (reply->error() != QNetworkReply::NoError) //有错误
        {
            cout << reply->errorString();
            reply->deleteLater(); //释放资源
            return;
        }

        //服务器返回用户的数据
        QByteArray array = reply->readAll();

        reply->deleteLater();
        if("016" == m_cm.getCode(array) )
        {
            //该文件pv字段+1
            for(int i = 0; i < m_fileList.size(); ++i)
            {
                FileInfo *info = m_fileList.at(i);
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

void MyFileWg::clearAllTask()
{
    //获取上传列表实例
    UploadTask *uploadList = UploadTask::getInstance();
    if(uploadList == NULL)
    {
        cout << "UploadTask::getInstance() == NULL";
        return;
    }

    uploadList->clearList();

    //获取下载列表实例
    DownloadTask *p = DownloadTask::getInstance();
    if(p == NULL)
    {
        cout << "DownloadTask::getInstance() == NULL";
        return;
    }

    p->clearList();
}

// 定时检查处理任务队列中的任务
void MyFileWg::checkTaskList()
{
    //定时检查上传队列是否有任务需要上传
    connect(&m_uploadFileTimer, &QTimer::timeout, [=]()
    {
		uploadFilesAction();
    });
    m_uploadFileTimer.start(500);

    connect(&m_downloadTimer, &QTimer::timeout, [=]()
    {
        downloadFilesAction();
    });
    m_downloadTimer.start(500);
}


// 显示右键菜单
void MyFileWg::rightMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->listWidget->itemAt( pos );

    if( item == NULL ) 
    {
        m_menuEmpty->exec( QCursor::pos() ); 
    }
    else //点图标
    {
        ui->listWidget->setCurrentItem(item);

        if(item->text() == "上传文件")
        {
            return;
        }

        m_menu->exec( QCursor::pos() );
    }
}

void MyFileWg::contextMenuEvent(QContextMenuEvent *event)
{
}
