#ifndef SHARELIST_H
#define SHARELIST_H

#include <QWidget>
#include <QTimer>
#include "common/common.h"
#include "selfwidget/mymenu.h"

namespace Ui {
class ShareList;
}

class ShareList : public QWidget
{
    Q_OBJECT

public:
    explicit ShareList(QWidget *parent = 0);
    ~ShareList();

    void initListWidget();

    void addActionMenu();

    void clearshareFileList();

    void clearItems();

    void refreshFileItems();

    QByteArray setFilesListJson(int start, int count); 
    void getUserFilesList();                           
    void getFileJsonInfo(QByteArray data);             

    void addDownloadFiles();
    void downloadFilesAction();

    QByteArray setShareFileJson(QString user, QString md5, QString filename);

    void dealFilePv(QString md5, QString filename); 

    enum CMD{Property, Cancel, Save};
    void dealSelectdFile(CMD cmd=Property); 

    void getFileProperty(FileInfo *info); 

    void cancelShareFile(FileInfo *info);

    void saveFileToMyList(FileInfo *info);

signals:
    void gotoTransfer(TransferStatus status);

private:
    Ui::ShareList *ui;

    Common  m_cm;
    MyMenu   *m_menuItem;       
    QAction *m_downloadAction;  
    QAction *m_propertyAction;  
    QAction *m_cancelAction;    
    QAction *m_saveAction;      

    MyMenu *m_menuEmpty;        
    QAction *m_refreshAction;   

    QNetworkAccessManager *m_manager;


    int m_start;           
    int m_count;           
    long m_userFilesCount; 

    QList<FileInfo *> m_shareFileList; 

    QTimer m_downloadTimer;   
};

#endif // SHARELIST_H
