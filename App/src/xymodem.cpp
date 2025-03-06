#include "xymodem.h"

XYmodem::XYmodem(QObject *parent) : QObject(parent) {

    m_timeoutTimer = new QTimer(this);
    m_responseEventLoop = new QEventLoop(this);
    connect(m_timeoutTimer, &QTimer::timeout, m_responseEventLoop, &QEventLoop::quit);
    connect(this, &XYmodem::response, m_responseEventLoop, &QEventLoop::quit);
}

XYmodem::~XYmodem() {

}

quint16 XYmodem::packet2CRC(const QByteArray &packet) {

    quint16 crc = 0x00;
    quint16 size = (packet.size() - 2);

    for(quint16 i = 3; i < size; i++) {
        crc ^= static_cast<quint16>(packet[i] << 8);
        for(quint16 j = 0; j < 8; j++) {
            if(crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}

quint16 XYmodem::packet2CheckSum(const QByteArray &packet) {

    quint16 checkSum = 0;

    for(quint8 i = 3; i < 131; i++) {
        checkSum += static_cast<quint8>(packet[i]);
    }

    return checkSum;
}

quint16 XYmodem::data2Packet(bool isXModem, QByteArray &packet, const QByteArray &data, quint32 &dataIndex, const quint32 &residualSize, const quint8 &number) {

    quint16 crc = 0;
    quint16 packetSize = 0;
    quint16 currentSize = 0;

    packetSize = (!isXModem && residualSize >= PACKET_1K_SIZE) ? PACKET_1K_SIZE : PACKET_SIZE;
    packet.resize(packetSize + PACKET_OVERHEAD);
    packet.fill(0x00);
    packet[0] = (packetSize >= PACKET_1K_SIZE) ? STX : SOH;
    packet[1] = number;
    packet[2] = ~number;
    currentSize = (residualSize < packetSize) ? residualSize : packetSize;
    memcpy((packet.data() + 3), (data.data() + dataIndex), currentSize);

    crc = isXModem ? packet2CheckSum(packet) : packet2CRC(packet);
    packet[packetSize + 3] = (crc >> 8);
    packet[packetSize + 4] = (crc & 0xFF);

    return packetSize;
}

void XYmodem::get_updateCMD(bool isXModem, const QString &fileName, const QByteArray &data) {

    quint8 errorCount = 0;
    quint8 progressValue = 0;
    quint8 error = 0;
    quint8 number = 1;
    quint16 packetSize = 0;
    quint32 dataIndex = 0;
    xymodemState state = progress;
    QByteArray packet;
    QByteArray deviceInfo;

    m_currentRun = 0;
    m_isSessionEnd = false;
    m_responsePacket.resize(2, 0);
    quint32 residualSize(data.size());

    while(!m_isSessionEnd) {

        if(m_currentRun == 0) {
            emit push_txData("UPDATE\n");
        } else {
            emit push_txData(packet);
        }

        m_responsePacket.fill(0);
        m_timeoutTimer->start(NAK_TIMEOUT);
        m_responseEventLoop->exec();

        switch(m_currentRun) {
            case 0: {
                if(m_responsePacket[0] == CRC16) {

                    packet.resize(3, 0);
                    packet[0] = SOH;
                    packet[1] = 0x00;
                    packet[2] = 0xFF;
                    packet.append(fileName.toUtf8());
                    packet.append(static_cast<char>(0));
                    packet.append(QByteArray::number(residualSize));
                    packet.append(static_cast<char>(0));
                    packet.resize(133, 0);
                    packet[130] = isXModem;
                    quint16 crc = packet2CRC(packet);
                    packet[131] = (crc >> 8);
                    packet[132] = (crc & 0xff);
                    errorCount = 0;
                    m_currentRun++;
                }
                break;
            }
            case 1: {
                if((m_responsePacket.size() == 2) && (m_responsePacket[0] == ACK) && (m_responsePacket[1] == CRC16)) {
                    errorCount = 0;
                    m_currentRun++;
                    state = progress;
                    packetSize = data2Packet(isXModem, packet, data, dataIndex, residualSize, number);
                }
                break;
            }
            case 2: {
                if(m_responsePacket[0] == ACK) {
                    errorCount = 0;
                    if(residualSize > packetSize) {
                        dataIndex += packetSize;
                        residualSize -= packetSize;
                        number = (number == 255) ? 0 : ++number;
                        packetSize = data2Packet(isXModem, packet, data, dataIndex, residualSize, number);
                    } else {
                        m_currentRun++;
                        dataIndex += residualSize;
                        packet.resize(1);
                        packet[0] = EOT;
                    }
                    progressValue = (dataIndex * 98) / data.size();
                }
                break;
            }
            case 3: {
                if(m_responsePacket[0] == ACK) {
                    errorCount = 0;
                    m_currentRun++;
                    progressValue++;
                    packet.resize(133, 0x00);
                    packet[0] = SOH;
                    packet[1] = 0x00;
                    packet[2] = 0xFF;
                }
                break;
            }
            case 4: {
                if(m_responsePacket[0] == ACK) {
                    errorCount = 0;
                    progressValue++;
                    state = updateOK;
                    m_isSessionEnd = 1;
                }
                break;
            }
            case 5: { // cancel
                errorCount = 0;
                state = updateCancel;
                m_isSessionEnd = 1;

                packet.resize(1);
                packet[0] = ABORT1;
                emit push_txData(packet);
                break;
            }
            default: { // loss connect
                errorCount = 0;
                state = disconnect;
                m_isSessionEnd = 1;
                break;
            }
        }

        if(m_responsePacket[0] == NAK) {
            errorCount = 0;
        }

        if((m_responsePacket.size() == 3)
            && (m_responsePacket[0] == CA)
            && (m_responsePacket[1] == CA)) {
            error = m_responsePacket[2];
            state = updateFail;
            m_isSessionEnd = 1;
        }

        errorCount++;
        if((errorCount > MAX_ERRORS)) {
            state = updateFail;
            m_isSessionEnd = 1;
        }

        emit push_xymodemState(state, progressValue, error);
    }
}

void XYmodem::get_uploadCMD() {
    emit push_xymodemState(uploadOK, 100, 0);
}

void XYmodem::get_eraseCMD() {

    quint8 errorCount = 0;
    quint8 eraseSize = 0;
    quint8 currentSize = 0;
    quint8 progressValue = 0;
    xymodemState state = progress;

    m_isSessionEnd = false;
    m_responsePacket.resize(1, 0);

    emit push_txData("ERASE\n");
    while(!m_isSessionEnd) {

        m_responsePacket[0] = 0;
        m_timeoutTimer->start(NAK_TIMEOUT);
        m_responseEventLoop->exec();

        if(!eraseSize) {
            eraseSize = static_cast<quint8>(m_responsePacket[0]);
        } else {
            if(m_responsePacket[0] == ACK) {
                errorCount = 0;
                currentSize++;
                state = progress;
                progressValue = (currentSize * 100) / eraseSize;
                if(currentSize >= eraseSize) {
                    errorCount = 0;
                    state = eraseOK;
                    m_isSessionEnd = 1;
                }
            }
        }

        errorCount++;
        if(errorCount > MAX_ERRORS) {
            state = eraseFail;
            m_isSessionEnd = 1;
        }

        emit push_xymodemState(state, progressValue, 0);
    }
}


void XYmodem::get_rxData(const QByteArray &data) {

    static QByteArray rxData;
    static quint8 isWait = 0;

    if(isWait == 0) {
        if(data[0] == CA) isWait = 1;
    }

    rxData.append(data);
    if((isWait == 1) && (rxData.size() < 3)) {
        return;
    }

    isWait = 0;
    m_responsePacket = rxData;
    rxData.clear();
    emit response();
}


