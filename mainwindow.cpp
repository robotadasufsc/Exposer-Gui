#include <QTime>
#include "mainwindow.h"
#include "seriallayer.h"
#include "ui_mainwindow.h"

struct Variable
{
    uint type;
    QString name;
    QVariant value;
};

QMap<int, Variable> variables;

mMainWindow::mMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    updateTimer(new QTimer(this)),
    dataTimer(new QTimer(this)),
    askForDataTimer(new QTimer(this)),
    ser(new SerialLayer(this)),
    numberofLists(4),
    running(false),
    baudrate(115200)
{
    ui->setupUi(this);

    ui->spinBox->setMaximum(10000);
    ui->spinBox->setMinimum(10);
    ui->spinBox->setValue(100);
    ui->comm->setPlaceholderText("Operation Target [Data]: 33 0; 35 0; 34 0 1");

    ui->table->setRowCount(4);
    ui->table->setColumnCount(3);
    QStringList tableHeader;
    tableHeader <<"Name" << "Type" << "Value";
    ui->table->setHorizontalHeaderLabels(tableHeader);
    ui->table->setStyleSheet("QTableView {selection-background-color: yellow;}");
    ui->table->horizontalHeader()->setStretchLastSection(true);

    connect(updateTimer, &QTimer::timeout, this, &mMainWindow::update);
    updateTimer->setInterval(100);

    connect(dataTimer, &QTimer::timeout, this, &mMainWindow::updateData);
    dataTimer->setInterval(100);

    connect(askForDataTimer, &QTimer::timeout, this, &mMainWindow::askForData);
    askForDataTimer->setInterval(100);

    ui->serialBox->addItems(ser->serialList());

    ui->treeWidget->setHeaderLabel("Plots");
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &mMainWindow::checkTree);
    connect(ui->pushButton, &QPushButton::clicked, this, &mMainWindow::checkStartButton);

    connect(ser, &SerialLayer::receivedCommand, this, &mMainWindow::checkReceivedCommand);
    connect(ser, &SerialLayer::pushedCommand, this, &mMainWindow::checkPushedCommands);
    connect(ui->comm, &QLineEdit::returnPressed, this, &mMainWindow::getComm);
}

QString mMainWindow::getTime()
{
    return QTime::currentTime().toString("hh:mm:ss:zzz");
}

void  mMainWindow::addLog(QByteArray msg)
{
    QString msgHex;
    for(const auto byte: msg)
    {
        if(byte > 32 && byte < 127)
            msgHex.append(byte);
        else
        {
            msgHex.append("\\x");
            if((uint)byte < 16)
                msgHex.append("0");
            msgHex.append(QString::number((byte & 0xFF), 16));
        }
    }

    const QString text = QString("[%1] ").arg(getTime()) + msgHex;
    qDebug() << text;
    ui->console->appendPlainText(text);
}

QVariant mMainWindow::convert(QByteArray msg, uint type)
{
    char *data = msg.data();

    switch(type)
    {
        case UINT8:
            return (0xFF & data[0]);
            break;

        case UINT16:
             qDebug() << (((0x00FF & data[1])<<8) & (0xFF & data[0]));
            return (uint16_t)((0x00FF & data[0]) & ((0x00FF & data[1])<<8));
            break;

        default:
            return 0;
            break;
    }
}

void mMainWindow::checkReceivedCommand()
{
    if(!ser->commandAvailable())
        return;
    QByteArray msg = ser->popCommand();
    addLog(msg);

    //request all
    if (msg.at(1) == 33)
    {
        //[id].name = name
        variables[msg.at(2)].name = msg.mid(4, msg.at(3) - 1);
        variables[msg.at(2)].type = msg.at(4 + msg.at(3) - 1);
    }

    //value
    if (msg.at(1) == 35)
    {
        QVariant value;
        QByteArray val;
        switch (variables[msg.at(2)].type)
        {
            //uint8_t
            case UINT8:
                value = convert(msg.mid(4, 1), UINT8);
                qDebug() << value << (uint) (msg.at(4) & 0xFF);
                break;

            case UINT16:
                value = convert(msg.mid(4, 2), UINT16);
                qDebug() << msg.mid(4, 2);
                qDebug() << value << convert(msg.mid(4, 2), UINT16);
                break;

            case UINT32:
                value = msg.mid(4, 4).toUInt();
                qDebug() << value;
                break;

            /*
            case 3:
                value = (uint8_t)msg.mid(4).at(0);
                qDebug() << value;
                break;
            */

            default:
                break;

        }
        variables[msg.at(2)].value = value;
    }

     for (const auto var: variables)
    {
        qDebug() << var.name << var.type << var.value;
    }


    int line = 0;

    if (ui->table->rowCount() < variables.size())
    {
        ui->table->setRowCount(variables.size());
    }

    // should be moved somewhere else! updateTable(), perhaps?

    QMapIterator<int,Variable> i(variables);
    while (i.hasNext()) {
        i.next();
        auto var = i.value();
        int line = i.key();

        QTableWidgetItem* item = ui->table->item(line, 0);
        if (item != nullptr)
        {
            if (item->text() != var.name)
            {
                item->setText(var.name);
            }
        }else
        {
             QTableWidgetItem *newItem = new QTableWidgetItem(QString(var.name));
            ui->table->setItem(line, 0, newItem);
        }

        QTableWidgetItem* itemType = ui->table->item(line, 1);
        if (itemType != nullptr)
        {
            if (itemType->text() != typeNames[var.type])
            {
                itemType->setText(typeNames[var.type]);
            }
        }else
        {
             QTableWidgetItem *newItem = new QTableWidgetItem(var.value.toString());
            ui->table->setItem(line, 1, newItem);
        }



        QTableWidgetItem* itemValue = ui->table->item(line, 2);
        if (itemValue != nullptr)
        {
            if (itemValue->text() != var.value.toString())
            {
                itemValue->setText(var.value.toString());
            }
        }else
        {
             QTableWidgetItem *newItem = new QTableWidgetItem(var.value.toString());
            ui->table->setItem(line, 2, newItem);
        }
    }

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
        ser->open(ui->serialBox->currentText(), baudrate);
        ui->pushButton->setText("Stop");
        updateTimer->start();
        dataTimer->start();
        askForDataTimer->start();
    }
    else
    {
        ui->pushButton->setText("Continue");
        updateTimer->stop();
        dataTimer->stop();
        askForDataTimer->stop();
    }
}

QByteArray mMainWindow::createCommand(char op, char target, QByteArray data)
{
    QByteArray msg;
    msg.append('<');
    msg.append(op);
    msg.append(target);
    msg.append(data.size());
    if(!data.isEmpty())
        msg.append(data);

    char crc = msg.at(0) ^ msg.at(0);
    for(const auto byte: msg)
        crc ^= byte;

    msg.append(crc);

    return msg;
}

void mMainWindow::askForData()
{

    if (variables.count() > 0)
    {
        for ( int i = 0; i < variables.count(); i++)
        {
            auto msg = createCommand(35, i, QByteArray());
            ser->pushCommand(msg);
        }
    }
    else
    {
        auto msg = createCommand(33, 0, QByteArray());
        ser->pushCommand(msg);
    }
/*  //write target 0
    QByteArray value;
    value.append((char)1);
    msg = createCommand(34, 0, value);
    ser->pushCommand(msg);
    */
}

void mMainWindow::getComm()
{
    QString command = ui->comm->text();
    QStringList list = command.split(' ');

    QByteArray msg;
    for (const auto item: list)
    {
        if (item.at(0).isDigit())
            msg.append((char)item.toInt());
        else
            msg.append(item);
    }
    auto com = createCommand(msg.at(0), msg.at(1), msg.mid(2));
    ser->pushCommand(com);
}

mMainWindow::~mMainWindow()
{
    delete ui;
}