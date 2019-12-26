#ifndef WORKERS_H
#define WORKERS_H

#include <QObject>
#include <QStandardItemModel>
#include <QFile>
#include <QFileDialog>
#include <QtDebug>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlRecord>
#include <QDateTime>

class loadDataSetWorker : public QObject
{
    Q_OBJECT
private:
    QString dirName;
    QSqlDatabase *m_db;

public:
    explicit loadDataSetWorker(QString,QObject *parent = nullptr);

signals:
    void dataset_ready();
    void workDone();
    void message(QString);
public slots:
    void doWork();
};

class plotWorker:public QObject
{
    Q_OBJECT

private:
    int *inNum;
    int *outNum;
    int len;
    QString filter;
    QDateTime startDateTime;
    int timeStep;
    QSqlDatabase *m_db;
public:
    explicit plotWorker(int,QString,QDateTime,int,QObject * parent=nullptr);
signals:
    void workDone(int ,int *,int *);
    void message(QString,int);

public slots:
    void doWork();


};

#endif // WORKERS_H
