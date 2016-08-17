#pragma once

#include <QMainWindow>
#include <QChartView>
#include <QWidget>
#include <QtCharts>

namespace Ui {
class MainWindow;
}

class mMainWindow : public QMainWindow
{

public:
    explicit mMainWindow(QWidget *parent = NULL);
    ~mMainWindow();

private:
    Ui::MainWindow *ui;
    QTimer *updateTimer;
    QTimer *dataTimer;
    void update();
    void updateData();
    void updateTree();
    void checkTree(QTreeWidgetItem *item, int column);
    void checkStartButton();
    uint numberofLists;
    bool running;

    QList<QList<QPointF>> dataList;
    QMap<uint,QString> dataInfo;
};
