#include "rankinglist.h"
#include "ui_rankinglist.h"
#include "common/logininfoinstance.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

RankingList::RankingList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RankingList)
{
    ui->setupUi(this);
    initTableWidget();

    m_manager = Common::getNetManager();
}

RankingList::~RankingList()
{
    delete ui;
}

void RankingList::initTableWidget()
{
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(300); 

    ui->tableWidget->horizontalHeader()-> setSectionsClickable(false);
    QStringList header;
    header.append("排名");
    header.append("文件名");
    header.append("下载量");
    ui->tableWidget->setHorizontalHeaderLabels(header);
    QFont font = ui->tableWidget->horizontalHeader()->font();
    font.setBold(true);
    ui->tableWidget->horizontalHeader()->setFont(font);

    ui->tableWidget->verticalHeader()->setDefaultSectionSize(40);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);  
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); 

    ui->tableWidget->horizontalHeader()->setStyleSheet(
                "QHeaderView::section{"
                "background: skyblue;"
                "font: 16pt \"新宋体\";"
                "height: 35px;"
                "border:1px solid #c7f0ea;"
                "}");
    ui->tableWidget->setColumnWidth(0,100);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}

void RankingList::clearshareFileList()
{
    int n = m_list.size();
    for(int i = 0; i < n; ++i)
    {
        RankingFileInfo *tmp = m_list.takeFirst();
        delete tmp;
    }
}
void RankingList::refreshFiles()
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
        m_userFilesCount = array.toLong();
        cout << "userFilesCount = " << m_userFilesCount;
        clearshareFileList();

        if(m_userFilesCount > 0)
        {
            m_start = 0;  
            m_count = 10; 
            getUserFilesList();
        }
        else 
        {
            refreshList();
        }
    });
}

QByteArray RankingList::setFilesListJson(int start, int count)
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

void RankingList::getUserFilesList()
{
    if(m_userFilesCount <= 0)
    {
        cout << "获取共享文件列表条件结束";
        refreshList();

        return;
    }
    else if(m_count > m_userFilesCount)
    {
        m_count = m_userFilesCount;
    }


    QNetworkRequest request; 
    LoginInfoInstance *login = LoginInfoInstance::getInstance();
    QString url;
    url = QString("http:// %1:%2/sharefiles?cmd=pvdesc").arg(login->getIp()).arg(login->getPort());
    request.setUrl(QUrl( url ));
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");
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
            cout << array.data();
            getFileJsonInfo(array);
            getUserFilesList();

        }

    });
}

void RankingList::getFileJsonInfo(QByteArray data)
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
                RankingFileInfo *info = new RankingFileInfo;
                info->filename = tmp.value("filename").toString();
                info->pv = tmp.value("pv").toInt();
                m_list.append(info);
            }
        }
    }
    else
    {
        cout << "err = " << error.errorString();
    }
}

void RankingList::refreshList()
{
    int rowCount = ui->tableWidget->rowCount();
    for(int i = 0; i < rowCount; ++i)
    {
        ui->tableWidget->removeRow(0);
    }

    int n = m_list.size();
    rowCount = 0;
    for(int i = 0; i < n; ++i)
    {
        RankingFileInfo *tmp = m_list.at(i);
        ui->tableWidget->insertRow(rowCount); 
        QTableWidgetItem *item1 = new QTableWidgetItem;
        QTableWidgetItem *item2 = new QTableWidgetItem;
        QTableWidgetItem *item3 = new QTableWidgetItem;
        item1->setTextAlignment(Qt::AlignHCenter |  Qt::AlignVCenter);
        item2->setTextAlignment(Qt::AlignLeft |  Qt::AlignVCenter);
        item3->setTextAlignment(Qt::AlignHCenter |  Qt::AlignVCenter);

        QFont font;
        font.setPointSize(15); 
        item1->setFont( font );
        item1->setText(QString::number(i+1));
        item2->setText(tmp->filename);
        item3->setText(QString::number(tmp->pv));
        ui->tableWidget->setItem(rowCount, 0, item1);
        ui->tableWidget->setItem(rowCount, 1, item2);
        ui->tableWidget->setItem(rowCount, 2, item3);

        rowCount++;
    }
}
