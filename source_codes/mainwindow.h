#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QFileDialog>
#include <QDebug>
#include <QStandardItemModel>
#include <QSplineSeries>
#include <QLineSeries>
#include <algorithm>
#include <QtCharts/QtCharts>
#include <QTableView>
#include <queue>
#include <stack>
#include <iostream>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlRecord>
#include "workers.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void removeAllDock();
    void showDock(const QList<int> & index=QList<int>());
    void initialize();
    void createStatusBar();

private slots:
    void on_pushButton_clicked();

    void connect_database();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void display_chart(int,int *,int *);

    void enable_plot_button();

    void set_statusBar(QString,int t=3000);


    void on_pushButton_5_clicked();

    void set_in_spinBox_value(QString);

    void set_out_spinBox_value(QString);


private:
    Ui::MainWindow *ui;
    QList<QDockWidget*> m_docks;
    QSqlDatabase *m_db;
    QSqlTableModel * model;
    QSqlTableModel * plotModel;
    QTableView *tableview;
    int adjMat[83][83];

    static const int MAXLEN=int(1e4);
    static const int MINLEN=11;
    static const int stationNum=81;
    static const int FIELDS=7;
    const QString title="Final-Project-CS241";
    QString get_filter();
    void Interpolation(int &len,int *inFlow,int *outFlow,QDateTime *xTime);
    void load_data_set();
    bool check_connection();    

};
#endif // MAINWINDOW_H
