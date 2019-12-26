#include "workers.h"

/////////////////////////////////////////////
/// \brief loadDataSetWorker::loadDataSetWorker
/// \param dir_name
/// \param parent
///
loadDataSetWorker::loadDataSetWorker(QString dir_name,QObject *parent)
    :dirName(dir_name)
{
}

void loadDataSetWorker::doWork()
{
    {
        m_db= new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE","load_connection"));

        m_db->setDatabaseName("metro.db");
        if (!m_db->open())
        {
            qDebug() << "loadDataSetWorker::Error: connection with database fail";
        }
        else
        {
            qDebug() << "loadDataSetWorker::Database: connection ok";
        }
        QSqlQuery query(*m_db);
        query.prepare("CREATE TABLE metro(id INTEGER PRIMARY KEY,time TEXT,lineID TEXT,"
                      "stationID TEXT,deviceID TEXT,"
                      "status TEXT, userID TEXT, payType TEXT);");

        if (!query.exec())
        {
            qDebug() << "loadDataSetWorker::Couldn't create the table 'metro': one might already exist.";
        }

        else
        {
            QDir *dir = new QDir(dirName);
            QStringList filter;
            filter<<"*.csv";
            QList<QFileInfo> * fileInfo=new QList<QFileInfo>(dir->entryInfoList(filter));

            m_db->transaction();
            for(int i=0;i<fileInfo->length();i++)
            {
                QFile file(fileInfo->at(i).filePath());
                if(!file.open(QIODevice::ReadOnly))
                {
                    qDebug()<<"Fail to open file.";
                    return;
                }

                QString line = file.readLine();
                while(!file.atEnd())
                {
                    QString line = file.readLine();
                    QStringList lineToken = line.split(",", QString::SkipEmptyParts);
                    query.prepare("INSERT INTO metro values(?,?,?,?,?,?,?,?)");
                    query.bindValue(0,QVariant(QVariant::Int));
                    query.bindValue(1,lineToken[0]);
                    query.bindValue(2,lineToken[1]);
                    query.bindValue(3,lineToken[2]);
                    query.bindValue(4,lineToken[3]);
                    query.bindValue(5,lineToken[4]);
                    query.bindValue(6,lineToken[5]);
                    query.bindValue(7,lineToken[6].simplified());
                    query.exec();
                }
                qDebug()<<fileInfo->at(i).fileName();
                QString msg=tr("Scanning file: %1...").arg(fileInfo->at(i).fileName());
                emit message(msg);
                file.close();
            }
            m_db->commit();

            qDebug()<<"loadDataSetWorker::load files successfully.";
        }
        if(m_db)
        {
            m_db->close();
            delete m_db;
            QSqlDatabase::removeDatabase("load_connection");
        }

    }

    emit message("Scanning files over.");
    emit workDone();
}



/////////////////////////////////////////////////////////////
/// \brief plotWorker::plotWorker
/// \param length
/// \param fil
/// \param startDT
/// \param timestep
plotWorker::plotWorker(int length,QString fil,QDateTime startDT,int timestep,QObject*)
    :filter(fil),startDateTime(startDT),timeStep(timestep),len(0),m_db(nullptr)
{
    inNum = new int[length];
    outNum = new int[length];
    for(int i=0;i<length;i++)
        inNum[i]=outNum[i]=0;
}

void plotWorker::doWork()
{
    m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE","plot_connection"));
    m_db->setDatabaseName("metro.db");
    if (!m_db->open())
    {
        qDebug() << "plotWorker::Error: connection with database fail";
    }
    else
    {
        qDebug() << "plotWorker::Database: connection ok.";
    }

    qDebug()<<"plotWorker::scanning entries to plot";
    emit message("Scanning entries to plot...",-1);
    QString filter_head="SELECT * FROM metro WHERE ";
    filter = filter_head + filter;
    {
        QSqlQuery query(filter,*m_db);
        QSqlRecord rec = query.record();
        int timeIdx = rec.indexOf("time");
        int statusIdx = rec.indexOf("status");
        while(query.next())
        {
            QDateTime curDateTime=QDateTime::fromString(query.value(timeIdx).toString(),"yyyy-MM-dd hh:mm:ss");
            uint deltaTime=curDateTime.toTime_t()-startDateTime.toTime_t();
            int curIndex=int(deltaTime/60./timeStep);
            len = std::max(len,curIndex+1);
            bool status = query.value(statusIdx).toBool();
            inNum[curIndex]+=status;
            outNum[curIndex]+=!status;
         }
      }
    if(m_db)
    {
        m_db->close();
        delete m_db;
        QSqlDatabase::removeDatabase("plot_connection");
    }


    emit workDone(len,inNum,outNum);


}

