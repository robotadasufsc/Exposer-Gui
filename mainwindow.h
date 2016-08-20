#pragma once

#include <QMainWindow>
#include <QChartView>
#include <QWidget>
#include <QtCharts>

namespace Ui {
class MainWindow;
}

class SerialLayer;

class mMainWindow : public QMainWindow
{

public:
    explicit mMainWindow(QWidget *parent = NULL);
    ~mMainWindow();

private:
    Ui::MainWindow *ui;
    QTimer *updateTimer;
    QTimer *dataTimer;
    SerialLayer *ser;

    void update();
    void updateData();
    void updateTree();
    void checkTree(QTreeWidgetItem *item, int column);
    void checkStartButton();
    QString getTime();
    void  addLog(QByteArray msg);
    void checkReceivedCommand();
    void checkPushedCommands(QByteArray bmsg);
    uint numberofLists;
    uint baudrate;
    bool running;

    QList<QList<QPointF>> dataList;
    QMap<uint,QString> dataInfo;
};
