#ifndef SERIAL_H
#define SERIAL_H
#include <QSerialPort>

class Serial : public QSerialPort {

    Q_OBJECT

  public:
    Serial(QObject *parent = nullptr);
    ~Serial();

    typedef enum {
        OPEN = 0,
        CLOSE,
        ERROR,
        LOSS,
    } portState;

    typedef struct {
        QString portName;
        quint32 baudrate;
        QSerialPort::DataBits dataBit;
        QSerialPort::Parity parity;
        QSerialPort::StopBits stopBit;
        QSerialPort::FlowControl flowCtrl;
    } serialInfo_TypeDef;

  signals:
    void push_portList(const QString &newPort, const QStringList &port, const QStringList &description);
    void push_portState(portState state);
    void push_rxData(const QByteArray &data);

  public slots:
    void get_serialConnect(bool enable, const serialInfo_TypeDef &info);
    void get_txData(const QByteArray &data);

  protected:
    void timerEvent(QTimerEvent *event) override;

  private:
    QSerialPort *m_serialPort;

    int m_timeID = 0;

    QString m_portName;
    QStringList m_perPort;
};

#endif