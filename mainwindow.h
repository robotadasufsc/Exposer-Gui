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
    QTimer *askForDataTimer;
    SerialLayer *ser;

    enum {UINT8, UINT16, UINT32, INT8, INT16, INT32, FLOAT};
    const QStringList typeNames = (QStringList() << "uint8" << "uint16" << "uint32" << "int8" << "int16" << "int32" << "float");

    void update();
    void updateData();
    void updateTree();
    void checkTree(QTreeWidgetItem *item, int column);
    void checkStartButton();
    QString getTime();
    void  addLog(QByteArray msg);
    QVariant convert(QByteArray data, uint type);
    void cellChanged(int row, int col);
    void checkReceivedCommand();
    void checkPushedCommands(QByteArray bmsg);
    QByteArray createCommand(char op, char target, QByteArray data);
    void getComm();
    void askForData();
    uint numberofLists;
    uint baudrate;
    bool running;

    QList<QList<QPointF>> dataList;
    QMap<uint,QString> dataInfo;
};
