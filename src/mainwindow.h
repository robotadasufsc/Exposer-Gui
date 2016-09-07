#pragma once

#include <QMainWindow>
#include <QChartView>
#include <QWidget>
#include <QtCharts>

namespace Ui
{
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
    union convStruct
    {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        int8_t int8;
        int16_t int16;
        int32_t int32;
        float float32;
        char c[0];
    };

    enum {UINT8, UINT16, UINT32, INT8, INT16, INT32, FLOAT};
    uint8_t m_sizes[7] = {1, 2, 4, 1, 2, 4, 4};

    enum {REQUEST_ALL = 33, WRITE, READ};
    const QStringList typeNames = (QStringList() << "uint8" << "uint16" << "uint32" << "int8" << "int16" << "int32" << "float");

    void update();
    void updateData();
    void updateTree();
    void checkTree(QTreeWidgetItem *item, int column);
    void checkStartButton();
    QString getTime();
    void  addLog(QByteArray msg);
    void cellChanged(int row, int col);
    void checkReceivedCommand();
    void checkPushedCommands(QByteArray bmsg);
    QByteArray createCommand(char op, char target, QByteArray data);
    void getComm();
    void askForData();
    int numberOfLists;
    uint baudrate;
    bool running;

    QList<QList<QPointF>> dataList;
    QMap<uint,QString> dataInfo;
};
