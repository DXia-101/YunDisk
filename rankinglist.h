#ifndef RANKINGLIST_H
#define RANKINGLIST_H

#include <QWidget>
#include "common/common.h"

namespace Ui {
class RankingList;
}

struct RankingFileInfo
{
    QString filename;  
    int pv;            
};

class RankingList : public QWidget
{
    Q_OBJECT

public:
    explicit RankingList(QWidget *parent = 0);
    ~RankingList();
    void initTableWidget();
    void clearshareFileList();

    void refreshFiles();                                
    QByteArray setFilesListJson(int start, int count);  
    void getUserFilesList();                            
    void getFileJsonInfo(QByteArray data);              
    void refreshList();
private:
    Ui::RankingList *ui;

    Common m_cm;
    QNetworkAccessManager *m_manager;

    int  m_start;                     
    int  m_count;                     
    long m_userFilesCount;            
    QList<RankingFileInfo *> m_list;  
};

#endif //  RANKINGLIST_H
