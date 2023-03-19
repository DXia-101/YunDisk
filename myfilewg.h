#ifndef MYFILEWG_H
#define MYFILEWG_H

#include <QWidget>
#include <QTimer>
#include "common/common.h"
#include "common/uploadtask.h"
#include "selfwidget/mymenu.h"

namespace Ui {
class MyFileWg;
}

class MyFileWg : public QWidget
{
    Q_OBJECT

public:
    explicit MyFileWg(QWidget *parent = 0);
    ~MyFileWg();

    void initListWidget();

    void addActionMenu();
    void addUploadFiles();
    QByteArray setMd5Json(QString user, QString token, QString md5, QString fileName);
    void uploadFilesAction();
    void uploadFile(UploadFileInfo *info);
    void clearFileList();
    void clearItems();
    void addUploadItem(QString iconPath=":/images/upload.png", QString name="上传文件");
    void refreshFileItems();
    enum Display{Normal, PvAsc, PvDesc};
    QStringList getCountStatus(QByteArray json);
    void refreshFiles(Display cmd=Normal);
    QByteArray setGetCountJson(QString user, QString token);
    QByteArray setFilesListJson(QString user, QString token, int start, int count);
    void getUserFilesList(Display cmd=Normal);
    void getFileJsonInfo(QByteArray data);
    void dealSelectdFile(QString cmd="分享");
    QByteArray setDealFileJson(QString user, QString token, QString md5, QString filename);//设置json包
    void shareFile(FileInfo *info); 
    void delFile(FileInfo *info);
    void getFileProperty(FileInfo *info);
    void addDownloadFiles();
    void downloadFilesAction();
    void dealFilePv(QString md5, QString filename);
    void clearAllTask();
    void checkTaskList();


signals:
    void loginAgainSignal();
    void gotoTransfer(TransferStatus status);

private:
    void rightMenu(const QPoint &pos);

protected:
    void contextMenuEvent(QContextMenuEvent *event);


private:
    Ui::MyFileWg *ui;

    Common m_cm;
    QNetworkAccessManager* m_manager;
    MyMenu   *m_menu;         
    QAction *m_downloadAction;
    QAction *m_shareAction;   
    QAction *m_delAction;     
    QAction *m_propertyAction;

    MyMenu   *m_menuEmpty;         
    QAction *m_pvAscendingAction;  
    QAction *m_pvDescendingAction; 
    QAction *m_refreshAction;      
    QAction *m_uploadAction;       


    long m_userFilesCount;       
    int m_start;                 
    int m_count;
    QTimer m_uploadFileTimer;    
    QTimer m_downloadTimer;      


    QList<FileInfo *> m_fileList;
};

#endif // MYFILEWG_H
