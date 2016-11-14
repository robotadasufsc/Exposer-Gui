#include <QTime>
#include <QElapsedTimer>
#include "mainwindow.h"
#include "seriallayer.h"
#include "exposervariables.h"
#include "ui_mainwindow.h"

mMainWindow::mMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    updateTimer(new QTimer(this)),
    dataTimer(new QTimer(this)),
    askForDataTimer(new QTimer(this)),
    elapsedTimer(new QElapsedTimer()),
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

    elapsedTimer->start();

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
    Q_UNUSED(status);
    // Note that if a file with the name newName already exists, copy() returns false (i.e. QFile will not overwrite it).
    QString saveFileName = QFileDialog::getSaveFileName(this, tr("Save Log to file (abbreviaton)"), "log");

    for (const auto list: evars->indiceToVarInfo)
    {
        if (list.done != true)
            continue;

        QFile file(QString("%0-%1.csv").arg(saveFileName, list.name));
        QTextStream out(&file);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::information(this, tr("Error"), tr("File problem."));
            return;
        }

        out << "index" << ',' << list.name << '\n';

        for (auto &value: *list.QVariantList)
        {
            // To no deal with the QVariant's type, we save the string
            out << value.time << ',' << value.var.toString() << '\n';
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

    // This function is make only for the user input in the col 3
    if (!evars->isValid(row) || col != 3)
        return;

    if (evars->getType(row) != ExposerVariables::STRING)
    {
        value = (ui->table->item(row, col)->text()).toFloat();
    }

    //http://stackoverflow.com/questions/2773977/convert-from-float-to-qbytearray
    switch (evars->getType(row))
    {
        case ExposerVariables::UINT8:
        {
            uint8_t convValue = (uint8_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::UINT16:
        {
            uint16_t convValue = (uint16_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::UINT32:
        {
            uint32_t convValue = (uint32_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::INT8:
        {
            int8_t convValue = (int8_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::INT16:
        {
            int16_t convValue = (int16_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::INT32:
        {
            int32_t convValue = (int32_t) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::FLOAT:
        {
            float convValue = (float) value;
            QByteArray tempArray(reinterpret_cast<const char*>(&convValue), sizeof(convValue));
            array = tempArray;
        }
        break;

        case ExposerVariables::STRING:
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

    uint msgType = msg.at(1);
    uint msgIndice = msg.at(2);

    //request all
    if (msgType == REQUEST_ALL)
    {
        //[id].name = name
        QString name = msg.mid(4, msg.at(3) - 1);
        uint type = msg.at(4 + msg.at(3) - 1);

        if (name == QString())
            return;

        evars->setIndiceName(name, msgIndice);
        evars->setType(msgIndice, type);
    }

    //value
    if (msgType == READ)
    {
        QVariant value;
        convStruct conv;
        uint8_t type = evars->getType(msgIndice);
        uint8_t size = msg.at(3);

        QByteArray data = msg.mid(4, size).data();

        for (int i = 0; i < size; i++)
        {
            conv.c[i] = data[i];
        }

        switch (type)
        {
            case ExposerVariables::UINT8:
                value = conv.uint8;
                break;

            case ExposerVariables::UINT16:
                value = conv.uint16;
                break;

            case ExposerVariables::UINT32:
                value = conv.uint32;
                break;

            case ExposerVariables::INT8:
                value = conv.int8;
                break;

            case ExposerVariables::INT16:
                value = conv.int16;
                break;

            case ExposerVariables::INT32:
                value = conv.int32;
                break;

            case ExposerVariables::FLOAT:
                value = conv.float32;
                break;

            case ExposerVariables::STRING:
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

        evars->append(msgIndice, value, elapsedTimer->nsecsElapsed());
    }

    if (ui->table->rowCount() < evars->size())
    {
        ui->table->setRowCount(evars->size());
    }

    updateTable();
}

void mMainWindow::updateTable()
{
    for (int i = 0; i < evars->size(); i++)
    {
        QTableWidgetItem* item = ui->table->item(i, 0);
        if (item != nullptr)
        {
            if (item->text() != evars->getName(i))
            {
                item->setText(evars->getName(i));
            }
        }
        else
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(evars->getName(i));
            newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ui->table->setItem(i, 0, newItem);
        }

        QTableWidgetItem* itemType = ui->table->item(i, 1);
        if (itemType != nullptr)
        {
            if (itemType->text() != typeNames[evars->getType(i)])
            {
                itemType->setText(typeNames[evars->getType(i)]);
            }
        }
        else
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(evars->getLast(i).toString());
            newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ui->table->setItem(i, 1, newItem);
        }


        QTableWidgetItem* itemValue = ui->table->item(i, 2);
        if (itemValue != nullptr)
        {
            if (itemValue->text() != evars->getLast(i).toString())
            {
                itemValue->setText(evars->getLast(i).toString());
            }
        }
        else
        {
            QTableWidgetItem *newItem = new QTableWidgetItem(evars->getLast(i).toString());
            newItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            ui->table->setItem(i, 2, newItem);
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
    for (int i = 0; i < evars->size(); i++)
    {
        if (!evars->isValid(i))
            continue;
        if (evars->getType(i) != ExposerVariables::STRING)
            numberOfLists++;
    }

    //update with fake data
    if (dataList.count() < numberOfLists)
    {
        for (int i = dataList.count(); i < numberOfLists; i++)
        {
            if (!evars->isValid(i))
                continue;
            if (evars->getType(i) != ExposerVariables::STRING)
            {
                dataInfo[i] = evars->getName(i);
                QList<QPointF> point;
                point.append(QPointF(0, 0));
                dataList.append(point);
            }
        }
        updateTree();
    }

    for (int i = 0 ; i < numberOfLists; i++)
    {
        if (!evars->isValid(i))
            continue;
        if (evars->getType(i) != ExposerVariables::STRING)
        {
            dataList[i].append(QPointF(dataList[i].last().rx() + 1, evars->getLast(i).toFloat()));
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
        if (evars->getType(i) != ExposerVariables::STRING)
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

    if (evars->size() > 0)
    {
        for (int i = 0; i < evars->size(); i++)
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
    delete elapsedTimer;
    delete ui;
}
