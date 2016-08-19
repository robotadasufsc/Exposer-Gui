#include <QTime>
#include "mainwindow.h"
#include "seriallayer.h"
#include "ui_mainwindow.h"

mMainWindow::mMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    updateTimer(new QTimer(this)),
    dataTimer(new QTimer(this)),
    ser(new SerialLayer("/dev/ttyUSB0", 115200)),
    numberofLists(4),
    running(true)
{
    ui->setupUi(this);

    ui->spinBox->setMaximum(10000);
    ui->spinBox->setMinimum(10);
    ui->spinBox->setValue(100);

    connect(updateTimer, &QTimer::timeout, this, &mMainWindow::update);
    updateTimer->start(100);

    connect(dataTimer, &QTimer::timeout, this, &mMainWindow::updateData);
    dataTimer->start(75);

    ui->treeWidget->setHeaderLabel("Plots");
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &mMainWindow::checkTree);
    connect(ui->pushButton, &QPushButton::clicked, this, &mMainWindow::checkStartButton);

    connect(ser, &SerialLayer::receivedCommand, this, &mMainWindow::checkReceivedCommand);
    connect(ser, &SerialLayer::pushedCommand, this, &mMainWindow::checkPushedCommands);
}

QString mMainWindow::getTime()
{
    return QTime::currentTime().toString("hh:mm:ss:zzz");
}

void  mMainWindow::addLog(QByteArray msg)
{
    QString msgHex;
    msgHex.append("[");
    for(const auto byte: msg)
    {
        msgHex.append(QString::number(byte, 16));
        msgHex.append(" ");
    }
    msgHex.append("]");
    const QString text = QString("[%1] ").arg(getTime()) + msgHex;
    qDebug() << text;
    ui->console->appendPlainText(text);
}

void mMainWindow::checkReceivedCommand()
{
    if(ser->commandAvailable())
        addLog(ser->popCommand());
}

void mMainWindow::checkPushedCommands(QByteArray bmsg)
{
    qDebug() << bmsg;
}

void mMainWindow::updateData()
{
    //update with fake data
    if(dataList.count() < numberofLists)
    {
        for(uint i = dataList.count(); i < numberofLists; i++)
        {
            dataInfo[i] = QString("line " + QString::number(i));
            QList<QPointF> point;
            point.append(QPointF(0, 0));
            dataList.append(point);
        }
        updateTree();
    }

    for(uint i = 0 ; i < numberofLists; i++)
    {
        dataList[i].append(QPointF(dataList[i].last().rx()+1, rand()%255));
    }
}

void mMainWindow::update()
{
    QChart* c = new QChart();
    c->setTitle("Graph");

    uint lineNuber = 0;
    for(const auto list: dataList)
    {
        QLineSeries* line1 = new QLineSeries();
        line1->setName(dataInfo[lineNuber]);
        if(ui->widget->chart()->series().size() > lineNuber)
        {
            if(list.count() < ui->spinBox->value())
                line1->append(list);
            else
                line1->append(list.mid(list.count()-ui->spinBox->value()));

            if(!ui->widget->chart()->series()[lineNuber]->isVisible())
                line1->hide();

            c->addSeries(line1);
        }
        else
        {
            if(list.count() < ui->spinBox->value())
                line1->append(list);
            else
                line1->append(list.mid(list.count()-ui->spinBox->value()));
            c->addSeries(line1);
        }
        lineNuber++;
    }
    c->createDefaultAxes();
    ui->widget->setChart(c);
    ui->widget->chart()->setTheme(QChart::ChartThemeDark);
    ui->widget->setRenderHint(QPainter::Antialiasing);
}

void mMainWindow::updateTree()
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable;
    for(uint i = ui->treeWidget->columnCount()-1; i < dataInfo.count(); i++)
    {
        QTreeWidgetItem * item = new QTreeWidgetItem();
        item->setFlags(flags);
        item->setText(0, dataInfo[i]);
        item->setCheckState(0, Qt::Checked);
        ui->treeWidget->addTopLevelItem(item);
    }
}

void mMainWindow::checkTree(QTreeWidgetItem *item, int column)
{
    if(running)
        updateTimer->stop();
    const uint id = ui->treeWidget->indexOfTopLevelItem(item);
    dataInfo[id] = item->text(0);
    if(item->checkState(0))
        ui->widget->chart()->series()[id]->show();
    else
        ui->widget->chart()->series()[id]->hide();
    if(running)
        updateTimer->start();
}

void mMainWindow::checkStartButton()
{
    running = !running;

    if(running)
    {
        ui->pushButton->setText("Stop");
        updateTimer->start();
        dataTimer->start();
    }
    else
    {
        ui->pushButton->setText("Continue");
        updateTimer->stop();
        dataTimer->stop();
    }
}

mMainWindow::~mMainWindow()
{
    delete ui;
}