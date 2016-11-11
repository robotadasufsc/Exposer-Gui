#include <QTime>
#include "mainwindow.h"
#include "seriallayer.h"
#include "exposervariables.h"
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
    evars(new ExposerVariables(this)),
    numberOfLists(0),
    baudrate(115200),
    running(false)
{
    ui->setupUi(this);

    ui->spinBox->setMaximum(10000);
    ui->spinBox->setMinimum(10);
    ui->spinBox->setValue(100);
    ui->comm->setPlaceholderText("Operation Target [Data]: 33 0; 35 0; 34 0 1");

    ui->table->setRowCount(0);
    ui->table->setColumnCount(4);
    QStringList tableHeader;
    tableHeader <<"Name" << "Type" << "Value" << "Enter";
    ui->table->setHorizontalHeaderLabels(tableHeader);
    ui->table->setStyleSheet("QTableView {selection-background-color: gray;}");
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
    connect(ui->table, &QTableWidget::cellChanged, this, &mMainWindow::cellChanged);

    connect(ser, &SerialLayer::receivedCommand, this, &mMainWindow::checkReceivedCommand);
    connect(ser, &SerialLayer::pushedCommand, this, &mMainWindow::checkPushedCommands);
    connect(ui->comm, &QLineEdit::returnPressed, this, &mMainWindow::getComm);

    connect(ui->actionSave, &QAction::triggered, this, &mMainWindow::save);
}

void mMainWindow::save(bool status)
{
    // Note that if a file with the name newName already exists, copy() returns false (i.e. QFile will not overwrite it).
    QString saveFileName = QFileDialog::getSaveFileName(this, tr("Save Log to file (abbreviaton)"), "log");
    unsigned int i = 0;
    for (const auto list: dataList)
    {
        QFile file(QString("%0-%1.csv").arg(saveFileName, dataInfo[i]));
        QTextStream out(&file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::information(this, tr("Error"), tr("File problem."));
            return;
        }

        out << "index" << ',' << dataInfo[i++] << '\n';

        for (const auto value: list)
        {
            out << value.x() << ',' << value.y() << '\n';
        }

        file.close();
    }
}

QString mMainWindow::getTime()
{
    return QTime::currentTime().toString("hh:mm:ss:zzz");
}

void  mMainWindow::addLog(QByteArray msg)
{
    QString msgHex;
    for (const auto byte: msg)
    {
        if (byte > 32 && byte < 127)
            msgHex.append(byte);
        else
        {
            msgHex.append("\\x");
            if ((uint)byte < 16)
                msgHex.append("0");
            msgHex.append(QString::number((byte & 0xFF), 16));
        }
    }

    const QString text = QString("[%1] ").arg(getTime()) + msgHex;
    ui->console->appendPlainText(text);
}

void mMainWindow::cellChanged(int row, int col)
{
    float value;
    QByteArray array;

    if (variables[row].type != STRING)
    {
        value = (ui->table->item(row, col)->text()).toFloat();
    }

    //http://stackoverflow.com/questions/2773977/convert-from-float-to-qbytearray
    switch (variables[row].type)
    {
        case UINT8:
        {
            uint8_t convValue = (uint8_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case UINT16:
        {
            uint16_t convValue = (uint16_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case UINT32:
        {
            uint32_t convValue = (uint32_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case INT8:
        {
            int8_t convValue = (int8_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case INT16:
        {
            int16_t convValue = (int16_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case INT32:
        {
            int32_t convValue = (int32_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case FLOAT:
        {
            QByteArray tempArray(reinterpret_cast<const char*>(&value), sizeof(value));
            array = tempArray;
        }
        break;

        case STRING:
        {
            array = (ui->table->item(row, col)->text()).toLocal8Bit();
        }
        break;

        default:
            break;

    }

    //write target
    QByteArray msg;
    msg = createCommand(WRITE, row, array);
    ser->pushCommand(msg);
}

void mMainWindow::checkReceivedCommand()
{
    if (!ser->commandAvailable())
        return;
    QByteArray msg = ser->popCommand();
    addLog(msg);

    //request all
    if (msg.at(1) == REQUEST_ALL)
    {
        //[id].name = name
        variables[msg.at(2)].name = msg.mid(4, msg.at(3) - 1);
        variables[msg.at(2)].type = msg.at(4 + msg.at(3) - 1);
    }

    //value
    if (msg.at(1) == READ)
    {
        QVariant value;
        convStruct conv;
        uint8_t type;
        uint8_t size;

        type = variables[msg.at(2)].type;
        size = msg.at(3);
        QByteArray data = msg.mid(4, size).data();

        for (int i = 0; i < size; i++)
        {
            conv.c[i] = data[i];
        }

        switch (type)
        {
            case UINT8:
                value = conv.uint8;
                break;

            case UINT16:
                value = conv.uint16;
                break;

            case UINT32:
                value = conv.uint32;
                break;

            case INT8:
                value = conv.int8;
                break;

            case INT16:
                value = conv.int16;
                break;

            case INT32:
                value = conv.int32;
                break;

            case FLOAT:
                value = conv.float32;
                break;

            case STRING:
            {
                QString text;
                for (uint i = 0; i < size; i++)
                {
                    text.append(conv.c[i]);
                }
                value = text;
                break;
            }

            default:
                value = 0.0;
                break;

        }
        variables[msg.at(2)].value = value;
    }

    if (ui->table->rowCount() < variables.size())
    {
        ui->table->setRowCount(variables.size());
    }

    // should be moved somewhere else! updateTable(), perhaps?

    QMapIterator<int, Variable> i(variables);
    while (i.hasNext())
    {
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
        }
        else
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(QString(var.name));
            newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ui->table->setItem(line, 0, newItem);
        }

        QTableWidgetItem* itemType = ui->table->item(line, 1);
        if (itemType != nullptr)
        {
            if (itemType->text() != typeNames[var.type])
            {
                itemType->setText(typeNames[var.type]);
            }
        }
        else
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(var.value.toString());
            newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ui->table->setItem(line, 1, newItem);
        }


        QTableWidgetItem* itemValue = ui->table->item(line, 2);
        if (itemValue != nullptr)
        {
            if (itemValue->text() != var.value.toString())
            {
                itemValue->setText(var.value.toString());
            }
        }
        else
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(var.value.toString());
            newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ui->table->setItem(line, 2, newItem);
        }
    }

}

void mMainWindow::checkPushedCommands(QByteArray bmsg)
{
    //qDebug() << bmsg;
}

void mMainWindow::updateData()
{
    numberOfLists = 0;
    for (const auto var: variables)
    {
        if (var.type != STRING)
            numberOfLists++;
    }

    //update with fake data
    if (dataList.count() < numberOfLists)
    {
        for (int i = dataList.count(); i < numberOfLists; i++)
        {
            if (variables[i].type != STRING)
            {
                dataInfo[i] = variables[i].name;
                QList<QPointF> point;
                point.append(QPointF(0, 0));
                dataList.append(point);
            }
        }
        updateTree();
    }

    for (int i = 0 ; i < numberOfLists; i++)
    {
        if (variables[i].type != STRING)
        {
            dataList[i].append(QPointF(dataList[i].last().rx() + 1, variables[i].value.toFloat()));
        }
    }
}

void mMainWindow::update()
{
    QChart* c = new QChart();
    c->setTitle("Graph");

    int lineNuber = 0;
    auto seriesList = ui->widget->chart()->series();
    for (const auto list: dataList)
    {
        QXYSeries* line1 = new QLineSeries();

        line1->setName(dataInfo[lineNuber]);

        if (list.count() < ui->spinBox->value())
            line1->append(list);
        else
            line1->append(list.mid(list.count()-ui->spinBox->value()));
        if (seriesList.size() > lineNuber)
        {
            if (!seriesList[lineNuber]->isVisible())
                line1->hide();
        }

        lineNuber++;
        c->addSeries(line1);
    }
    c->setTheme(QChart::ChartThemeDark);
    c->createDefaultAxes();
    //To avoid memory leaks need to make sure the previous chart is deleted.
    QChart* willBeDeleted = ui->widget->chart();;
    ui->widget->setChart(c);
    delete willBeDeleted;
    ui->widget->setRenderHint(QPainter::Antialiasing);
}

void mMainWindow::updateTree()
{
    for (int i = ui->treeWidget->topLevelItemCount(); i < dataInfo.count(); i++)
    {
        if (variables[i].type != STRING)
        {
            QTreeWidgetItem * item = new QTreeWidgetItem();
            item->setText(0, dataInfo[i]);
            item->setCheckState(0, Qt::Checked);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            ui->treeWidget->addTopLevelItem(item);
        }
    }
}

void mMainWindow::checkTree(QTreeWidgetItem *item, int column)
{
    if (running)
        updateTimer->stop();
    const uint id = ui->treeWidget->indexOfTopLevelItem(item);
    dataInfo[id] = item->text(0);
    if (item->checkState(0))
        ui->widget->chart()->series()[id]->show();
    else
        ui->widget->chart()->series()[id]->hide();
    if (running)
        updateTimer->start();
}

void mMainWindow::checkStartButton()
{
    running = !running;

    if (running)
    {
        ser->open(ui->serialBox->currentText(), baudrate);
        ui->pushButton->setText("Stop");
        updateTimer->start();
        dataTimer->start();
        askForDataTimer->start();
    }
    else
    {
        ser->closeConnection();
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
    if (!data.isEmpty())
        msg.append(data);

    char crc = msg.at(0) ^ msg.at(0);
    for (const auto byte: msg)
        crc ^= byte;

    msg.append(crc);

    return msg;
}

void mMainWindow::askForData()
{

    if (variables.count() > 0)
    {
        for (int i = 0; i < variables.count(); i++)
        {
            auto msg = createCommand(READ, i, QByteArray());
            ser->pushCommand(msg);
        }
    }
    else
    {
        auto msg = createCommand(REQUEST_ALL, 0, QByteArray());
        ser->pushCommand(msg);
    }
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
