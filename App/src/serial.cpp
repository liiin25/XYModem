#include "serial.h"
#include <QSerialPortInfo>
#include <QTimerEvent>

Serial::Serial(QObject *parent) : QSerialPort(parent) {

    m_serialPort = new QSerialPort(this);
    // clang-format off
    connect(m_serialPort, &QSerialPort::readyRead, [this]() {
        if(m_serialPort->isOpen()) {
            emit push_rxData(m_serialPort->readAll());
        }
    });
    // clang-format on
    m_timeID = startTimer(200);
}

Serial::~Serial() {

    m_serialPort->close();
    killTimer(m_timeID);
}

void Serial::timerEvent(QTimerEvent *event) {

    if(event->timerId() == m_timeID) {

        QString newPort;
        QStringList port;
        QStringList description;

        foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            port << info.portName();
            description << info.description();
            if(!m_perPort.contains(port.last())) {
                newPort = port.last();
            }
        }
        if(m_perPort.size() != port.size()) {
            m_perPort = port;
            emit push_portList(newPort, port, description);
            if(m_serialPort->isOpen()) {
                if(!port.contains(m_portName)) {
                    m_serialPort->close();
                    emit push_portState(LOSS);
                }
            }
        }
    }

    event->accept();
}

void Serial::get_serialConnect(bool enable, const serialInfo_TypeDef &info) {

    if(enable) {
        m_serialPort->setPortName(info.portName);
        m_serialPort->setBaudRate(info.baudrate);
        m_serialPort->setDataBits(info.dataBit);
        m_serialPort->setParity(info.parity);
        m_serialPort->setStopBits(info.stopBit);
        m_serialPort->setFlowControl(info.flowCtrl);

        if(m_serialPort->open(QIODevice::ReadWrite)) {
            m_serialPort->clear();
            emit push_portState(OPEN);
        } else {
            m_serialPort->close();
            emit push_portState(ERROR);
        }
    } else {
        m_serialPort->close();
        emit push_portState(CLOSE);
    }
}

void Serial::get_txData(const QByteArray &data) {
    if(m_serialPort->isOpen()) {
        m_serialPort->write(data);
    }
}
