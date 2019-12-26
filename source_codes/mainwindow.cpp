#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_db(nullptr)
{
    ui->setupUi(this);
    adjMat[0][0]=-1;
    initialize();
}
MainWindow::~MainWindow()
{
    delete ui;
    if(m_db)
        delete m_db;
}
void MainWindow::initialize()
{
    QWidget *p = takeCentralWidget();
    if(p)
        delete p;
//    setDockNestingEnabled(true);
    m_docks.append(ui->dockWidget);
    m_docks.append(ui->dockWidget_2);
    m_docks.append(ui->dockWidget_3);
//    m_docks.append(ui->dockWidget_4);
    removeAllDock();
    addDockWidget(Qt::BottomDockWidgetArea,ui->dockWidget);
    splitDockWidget(ui->dockWidget,ui->dockWidget_2,Qt::Horizontal);
    splitDockWidget(ui->dockWidget_2,ui->dockWidget_3,Qt::Horizontal);
    tabifyDockWidget(ui->dockWidget,ui->dockWidget_2);
    tabifyDockWidget(ui->dockWidget_2,ui->dockWidget_3);
    showDock();

    createStatusBar();

    connect(ui->inStationID_edit,SIGNAL(textChanged(QString)),this,SLOT(set_in_spinBox_value(QString)));
    connect(ui->in_spinBox,SIGNAL(valueChanged(QString)),ui->inStationID_edit,SLOT(setText(QString)));
    connect(ui->outStationID_edit,SIGNAL(textChanged(QString)),this,SLOT(set_out_spinBox_value(QString)));
    connect(ui->out_spinBox,SIGNAL(valueChanged(QString)),ui->outStationID_edit,SLOT(setText(QString)));

    ui->tableView->setMinimumSize(780,380);
    ui->graphicsView->setMinimumSize(1000,330);
    ui->textBrowser->setMinimumSize(600,400);

}

void MainWindow::removeAllDock()
{
    for(int i=0;i<m_docks.length();i++)
        removeDockWidget(m_docks[i]);
}

void MainWindow::showDock(const QList<int> &index)
{
    if(index.isEmpty())
    {
        for(int i=0;i<m_docks.length();i++)
            m_docks[i]->show();
    }
    else
    {
        foreach (int i, index)
            m_docks[i]->show();
    }
}

void MainWindow::createStatusBar()
{
    ui->statusbar->showMessage("Please load the dataSet first.");
}


////////////////////////////////////////
/// slots part
////////////////////////////////////////
void MainWindow::on_pushButton_clicked()
{
    load_data_set();
}

void MainWindow::load_data_set()
{
    QFileInfo fi(QDir::currentPath()+"\\metro.db");
    if(fi.isFile())
    {
        connect_database();
        return;
    }

    QString dirname = QFileDialog::getExistingDirectory();
    auto thread = new QThread;
    auto worker = new loadDataSetWorker(dirname);

    connect(thread,SIGNAL(started()),worker,SLOT(doWork()));
    connect(worker,SIGNAL(workDone()),thread,SLOT(quit()));
    connect(worker,SIGNAL(workDone()),this,SLOT(connect_database()));
    connect(thread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    connect(worker,SIGNAL(message(QString)),this,SLOT(set_statusBar(QString)));
    worker->moveToThread(thread);
    thread->start();
}

void MainWindow::connect_database()
{
    if(m_db)
        return;
    m_db=new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE","main_connection"));
    m_db->setDatabaseName("metro.db");
    if (!m_db->open())
    {
        qDebug() << "MainWindow::Error: connection with database fail";
    }
    else
    {
        qDebug() << "MainWindow::Database: connection ok";
        set_statusBar("Database connected successfully.");

    }
    ui->pushButton->setEnabled(false);
}


QString MainWindow::get_filter()
{
    QString start = ui->startTime_edit1->dateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString end =  ui->endTime_edit1->dateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString lineID = ui->lineID_edit1->text();
    QString stationID = ui->stationID_edit1->text();
    QString deviceID = ui->deviceID_edit1->text();
    QString status = ui->status_edit1->text();
    QString userID = ui->userID_edit1->text();
    QString payType = ui->payType_edit1->text();

    QString filter;
    filter = QObject::tr("time >= '%1' AND time <= '%2'" ).arg(start).arg(end);
    if(lineID!="all")
        filter += QObject::tr(" AND lineID='%1'").arg(lineID);
    if(stationID!="all")
        filter += QObject::tr(" AND stationID='%1'").arg(stationID);
    if(deviceID!="all")
        filter += QObject::tr(" AND deviceID='%1'").arg(deviceID);
    if(status!="all")
        filter += QObject::tr(" AND status='%1'").arg(status);
    if(userID!="all")
        filter += QObject::tr(" AND userID='%1'").arg(userID);
    if(payType!="all")
        filter += QObject::tr(" AND payType='%1'").arg(payType);
    return filter;


}
void MainWindow::on_pushButton_2_clicked()
{
////////////////////
////import data
/// ///////////////

    if(!check_connection())
        return;

    // filter
    QString filter = get_filter();
    if(model)
    {
        delete model;
        model=nullptr;
    }

    model = new QSqlTableModel(this,*m_db);
    model->setTable("metro");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setFilter(filter);
    model->select();
    model->removeColumn(0);
    ui->tableView->setModel(model);
    ui->tableView->show();
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::Interpolation(int &len,int inFlow[],int outFlow[],QDateTime xTime[])
{
    double delta = (xTime[len-1].toTime_t()-xTime[0].toTime_t())/double(MINLEN);
    double lagTable[MINLEN];
    for(int i=0;i<MINLEN;i++)
    {
        double x=xTime[0].toTime_t()+delta*(i+1);
        for(int j=0;j<len;j++)
        {
            double tmp1=1.0;
            double tmp2=1.0;
            for(int k=0;k<len;k++)
            {
                if(k==j)
                    continue;
                tmp1*=(x-xTime[k].toTime_t());
                tmp2*=(double(xTime[j].toTime_t())-xTime[k].toTime_t());    // uint-uint overflow!!!
            }
            lagTable[j]=tmp1/tmp2;
        }
        if(i==len/2)
            for(int j=0;j<len;j++)
                qDebug()<<lagTable[j]<<" ";
        inFlow[i+len]=0;
        outFlow[i+len]=0;
        double tmp_in=0;
        double tmp_out=0;
        for(int j=0;j<len;j++)
        {
            tmp_in+=inFlow[j]*lagTable[j];
            tmp_out+=outFlow[j]*lagTable[j];
        }
        inFlow[i+len]=int(tmp_in);
        outFlow[i+len]=int(tmp_out);
        xTime[i+len] = QDateTime::fromTime_t(uint(x));
    }
    len += MINLEN;

    for(int i=0;i<len;i++)
        for(int j=i+1;j<len;j++)
        {
            if(xTime[i]>xTime[j])
            {
                std::swap(xTime[i],xTime[j]);
                std::swap(inFlow[i],inFlow[j]);
                std::swap(outFlow[i],outFlow[j]);
            }
        }
    for(int i=0;i<len;i++)
        qDebug()<<xTime[i].time()<<" "<<inFlow[i];

}

void MainWindow::on_pushButton_3_clicked()
{
    /////////////////////////
    /// plot in flow and outflow trend
    ////////////////////////

    if(!check_connection())
        return;
    QDateTime startDateTime,endDateTime;
    startDateTime.setDate(ui->startTime_edit2->date());
    startDateTime.setTime(ui->startTime_edit2->time());
    endDateTime.setDate(ui->endTime_edit2->date());
    endDateTime.setTime(ui->endTime_edit2->time());

    int timeStep = ui->timeStep_edit2->text().toInt();
    int stationID = ui->stationID_edit2->text().toInt();

    QString filter=get_filter();
    filter += QObject::tr(" AND time >= '%1' AND time <= '%2'")
            .arg(startDateTime.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(endDateTime.toString("yyyy-MM-dd hh:mm:ss"));
    filter += QObject::tr(" AND stationID = '%1'").arg(stationID);

    ui->pushButton_3->setEnabled(false);
    auto thread = new QThread;
    auto *worker = new plotWorker(MAXLEN,filter,startDateTime,timeStep);
    connect(thread,SIGNAL(started()),worker,SLOT(doWork()));
    connect(worker,SIGNAL(workDone(int,int*,int*)),thread,SLOT(quit()));
    connect(worker,SIGNAL(workDone(int,int*,int*)),this,SLOT(display_chart(int,int*,int*)));
    connect(thread,SIGNAL(finished()),worker,SLOT(deleteLater()));
    connect(thread,SIGNAL(finished()),this,SLOT(enable_plot_button()));
    connect(worker,SIGNAL(message(QString, int)),this,SLOT(set_statusBar(QString,int)));
    worker->moveToThread(thread);
    thread->start();
}

void MainWindow::enable_plot_button()
{
    ui->pushButton_3->setEnabled(true);
}
void MainWindow::display_chart(int len,int *inNum,int *outNum)
{
///////////////
/// build chart
/// ///////////
    if(len==0){
        QString text = "Please import data with another range again or change the stationID here.";
        QMessageBox::information(this,"No suitable data provided",text);
        return;
    }

    qDebug()<<"building chart...";
    set_statusBar("Building chart...",-1);
    QDateTime startDateTime,endDateTime;
    startDateTime.setDate(ui->startTime_edit2->date());
    startDateTime.setTime(ui->startTime_edit2->time());
    endDateTime.setDate(ui->endTime_edit2->date());
    endDateTime.setTime(ui->endTime_edit2->time());

    int timeStep = ui->timeStep_edit2->text().toInt();

    QDateTime axisxTime[MAXLEN];
    for(int i=0;i<len;i++)
        axisxTime[i]=QDateTime::fromTime_t(i*60*timeStep+startDateTime.toTime_t());
    if(len<MINLEN&&ui->interpolation_checkBox->isChecked())
        Interpolation(len,inNum,outNum,axisxTime);

    int maxFlow=0;   //the upper bound of axisY
    for(int i=0;i<len;i++)
        maxFlow=std::max(maxFlow,std::max(inNum[i],outNum[i]));

    QLineSeries *seriesIn = new QLineSeries();
    QLineSeries *seriesOut = new QLineSeries();
    seriesIn->setName("Inflow");
    seriesOut->setName("Outflow");

    for(int i=0;i<len;i++)
   {
        long long  curTime = axisxTime[i].toMSecsSinceEpoch();
        seriesIn->append(curTime,inNum[i]);
        seriesOut->append(curTime,outNum[i]);
    }
    QChart *chart = new QChart();
    chart->addSeries(seriesIn);
    chart->addSeries(seriesOut);
    chart->legend()->setAlignment(Qt::AlignBottom);
    seriesIn->setUseOpenGL(true);
    seriesOut->setUseOpenGL(true);
    chart->setTitle(tr("Inflow and Outflow of station %1")
                    .arg(ui->stationID_edit2->text().toInt()));

    QDateTimeAxis *axisX=new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("d,hh:mm");
    axisX->setRange(startDateTime,endDateTime);
    chart->addAxis(axisX,Qt::AlignBottom);
    seriesIn->attachAxis(axisX);
    seriesOut->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setRange(0,maxFlow+5);
    axisY->setLabelFormat("%i");
    axisY->setTitleText("traffic flow");
    chart->addAxis(axisY, Qt::AlignLeft);
    seriesIn->attachAxis(axisY);
    seriesOut->attachAxis(axisY);

    if(ui->show_in_new_window_checkBox->isChecked())
    {
        QChartView *view=new QChartView(chart);
        view->setWindowTitle(tr("the flow at station %1").arg(ui->stationID_edit2->text().toInt()));
        view->setRenderHint(QPainter::Antialiasing);
        view->resize(1000,300);
        view->show();
    }
    else
    {
        if(ui->graphicsView->chart())
            delete ui->graphicsView->chart();
        ui->graphicsView->setChart(chart);
        ui->graphicsView->setRenderHint(QPainter::Antialiasing);

        delete []inNum;
        delete []outNum;
    }
    ui->statusbar->clearMessage();

}
void MainWindow::on_pushButton_4_clicked()
{
////////////////////
///search route
///////////////////

    if(adjMat[0][0]==-1)
    {
        QString name=QDir::currentPath()+"\\Metro_roadMap.csv";
        QFileInfo fi(name);

        if(!fi.isFile())
        {
            QString title="Information";
            QString text="MetroRoadMap has not been loaded yet.\n"
                        "Do you want to load it now?";
            QMessageBox msgbox(QMessageBox::Information,title,text,QMessageBox::Yes|QMessageBox::No);
            int ret=msgbox.exec();
            if(ret==QMessageBox::Yes)
                on_pushButton_5_clicked();
            else return;

        }
        else
            on_pushButton_5_clicked();

    }


    int inID = ui->inStationID_edit->text().toInt();
    int outID = ui->outStationID_edit->text().toInt();

    bool visited[stationNum+3]={false};
    int previous[stationNum+3]={-1};
    std::queue<int> que;

    que.push(inID);
    visited[inID]=true;
    while(!que.empty())
    {
        int curID=que.front();
        que.pop();
        if(curID == outID)
            break;

        for(int i=0;i<stationNum;i++)
        {
            if(!visited[i]&&adjMat[curID][i])
            {
                que.push(i);
                visited[i]=true;
                previous[i]=curID;
            }
        }
    }

    qDebug()<<"route found";
    std::stack<int> st;
    for(int id = outID;id!=-1&&id!=inID;id = previous[id])
        st.push(id);

    ui->textBrowser->clear();
    ui->textBrowser->append(QString("Possible route:\n\n")+QString::number(inID));
    while(!st.empty())
    {
        ui->textBrowser->append("->");
        ui->textBrowser->append(QString::number(st.top()));
        st.pop();
    }


}

void MainWindow::on_pushButton_5_clicked()
{
    ///////////////////////////
    /////load_roadMap
    //////////////////////////
    ///
    QString name=QDir::currentPath()+"\\Metro_roadMap.csv";
    QFileInfo fi(name);

    if(!fi.isFile())
    {
        name = QFileDialog::getOpenFileName();
    }
    QFile file(name);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug()<< file.errorString();
        return;
    }

    //load adjencyMatrix
    int lineindex=0;
    file.readLine(); //skip line 0
    while(!file.atEnd())
    {
        lineindex++;
        QString line = file.readLine();
        QStringList lineToken =line.split(",", QString::SkipEmptyParts);
        for (int j = 1; j < lineToken.size(); j++) {
            int value = lineToken.at(j).toInt();
            adjMat[lineindex-1][j-1]=value;

        }
    }

    name=QDir::currentPath()+"\\Metro_roadMap.csv";
    QFileInfo fi2(name);
    if(!fi2.isFile())
        file.copy("Metro_roadMap.csv");
    file.close();

    ui->pushButton_5->setEnabled(false);

}

void MainWindow::set_statusBar(QString text,int t)
{
    ui->statusbar->clearMessage();
    if(t==-1)
    {
        ui->statusbar->showMessage(text);
    }

    else
    {
        ui->statusbar->showMessage(text,t);
    }
}


bool MainWindow::check_connection()
{
    if(m_db)
        return true;
    QFileInfo fi(QDir::currentPath()+"\\metro.db");
    QString title="Warning";
    QString text;
    if(fi.isFile())
       text="Database has not been connected yet.\n"
                "Connect now?";
    else
        text="Dataset has not been loaded yet.\n"
             "Load now?";
    QMessageBox msgbox(QMessageBox::Warning,title,text,QMessageBox::Yes|QMessageBox::No);
    int ret=msgbox.exec();
    if(ret==QMessageBox::Yes)
        load_data_set();
    return false;

}

void MainWindow::set_in_spinBox_value(QString str)
{
    int value=str.toInt();
    if(value==ui->in_spinBox->value())
        return;
    ui->in_spinBox->setValue(value);
}
void MainWindow::set_out_spinBox_value(QString str)
{
    int value=str.toInt();
    if(value==ui->out_spinBox->value())
        return;
    ui->out_spinBox->setValue(value);
}
