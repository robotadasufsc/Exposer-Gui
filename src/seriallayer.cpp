#include <QDebug>
#include <QSerialPortInfo>

#include "seriallayer.h"

QByteArray SerialLayer::_head = QByteArray("<");

SerialLayer::SerialLayer(QWidget *parent):
    serial(new QSerialPort()),
    _serialOpened(false)
{
    connect(serial, &QSerialPort::readyRead, this, &SerialLayer::readData);
};

bool SerialLayer::open(QString port, uint baud)
{
    if (_serialOpened)
        serial->close();

    serial->setPortName(port);
    serial->setBaudRate(baud);
    _serialOpened = serial->open(QIODevice::ReadWrite);
    serial->clear();
    return _serialOpened;
}

QStringList SerialLayer::serialList()
{
    QStringList ports;
    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    if (!serialPortInfoList.isEmpty())
    {
        for (const auto &serialPortInfo: serialPortInfoList)
        {
            ports.append("/dev/" + serialPortInfo.portName());
        }
    }
    else
    {
        return QStringList();
    }

    return ports;
}

bool SerialLayer::opened()
{
    return _serialOpened;
}

void SerialLayer::readData()
{
    _rawData.append(serial->readAll());
    QList<QByteArray> tempList;

    // Ugly way to do cool things
    while (_rawData.size() > 5)
    {
        if (_rawData.at(0) != _head.at(0))
        {
            _rawData.remove(0, 1);
            continue;
        }

        char crcf = 0;
        // header operation
        char crc = _head.at(0) ^ _rawData.at(1);
        // target
        crc = crc  ^ _rawData.at(2);
        // payload
        crc = crc  ^ _rawData.at(3);

        // get rest
        if (_rawData.at(3) < _rawData.size() - 4)
        {
            int i = 3 + 1;
            uint size = i;
            for (; i < _rawData.at(3) + 4; i++)
            {
                crc = crc ^ _rawData.at(i);
                size = i + 1;
            }
            crcf = _rawData.at(size);

            //qDebug() << "OK ?" << (crc == crcf) << _rawData << _rawData.left(size) << size;

            if (crc == crcf)
            {
                _rByteCommands.append(_rawData.left(size));
                emit(receivedCommand(_rawData.left(size)));
                _rawData = _rawData.mid(size + 1);
            }
        }
        else
        {
            if (serial->bytesAvailable())
            {
                // add new data
                _rawData.append(serial->readAll());
            }
            else
            {
                //qDebug()  << "NO ENOUGH DATA " << _rawData;
                break;
            }
        }
    }
}

void SerialLayer::pushCommand(QByteArray comm)
{
    serial->write(comm);
    emit(pushedCommand(comm));
}

void SerialLayer::addCommand(QByteArray comm)
{
    _sByteCommands.append(comm);
}

void SerialLayer::push()
{
    foreach (const auto& comm, _sByteCommands)
    {
        serial->write(comm);
        emit(pushedCommand(comm));
    }
    _sByteCommands.clear();
}

QByteArray SerialLayer::popCommand()
{
    if (!_rByteCommands.isEmpty())
        return _rByteCommands.takeFirst();
    return QByteArray();
}

bool SerialLayer::commandAvailable()
{
    return !_rByteCommands.isEmpty();
}

SerialLayer::~SerialLayer()
{
    delete serial;
}

void SerialLayer::closeConnection()
{
    if (_serialOpened)
    {
        serial->close();
    }
}
