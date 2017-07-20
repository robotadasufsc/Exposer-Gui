#pragma once

#include <QMainWindow>
#include <QChartView>
#include <QWidget>
#include <QtCharts>

namespace Ui
{
class MainWindow;
}

class QElapsedTimer;

class SerialLayer;

class ExposerVariables;

class mMainWindow : public QMainWindow
{

public:
    mMainWindow(QWidget* parent = nullptr);
    ~mMainWindow();

private:
    Ui::MainWindow *ui;
    QTimer *updateTimer;
    QTimer *dataTimer;
    QTimer *askForDataTimer;
    QElapsedTimer *elapsedTimer;
    SerialLayer *ser;
    ExposerVariables *evars;
    union convStruct
    {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        int8_t int8;
        int16_t int16;
        int32_t int32;
        float float32;
        char c[128];
    };

    enum {UINT8, UINT16, UINT32, INT8, INT16, INT32, FLOAT, STRING};
    uint8_t m_sizes[8] = {1, 2, 4, 1, 2, 4, 4, 0};

    enum {REQUEST_ALL = 33, WRITE, READ};
    const QStringList typeNames = (QStringList() << "uint8" << "uint16" << "uint32" << "int8" << "int16" << "int32" << "float" << "String");

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
    void updateTable();
    QByteArray createCommand(char op, char target, QByteArray data);
    void getComm();
    void askForData();
    void save(bool status);
    int numberOfLists;
    uint baudrate;
    bool running;

    QList<QList<QPointF>> dataList;
    QMap<uint,QString> dataInfo;
};
