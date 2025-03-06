#ifndef WIDGET_H
#define WIDGET_H
#include "serial.h"
#include "xymodem.h"
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTextCursor>
#include <QTranslator>
#include <qevent.h>
#include "encryptdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget {

    Q_OBJECT

  public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    typedef enum {
        Connect = 0,
        Message,
        Restart,
        Debug,
        Warn,
        Error,
        Fatal,
    } logLevel;

  signals:
    void push_stopCMD(bool stopMode);
    void push_exitCMD();

    void push_txData(const QByteArray &data);
    void push_serialConnect(bool enable, const Serial::serialInfo_TypeDef &info);

    void push_updateCMD(bool isXModem, const QString &fileName, const QByteArray &data);
    void push_uploadCMD();
    void push_eraseCMD();

    void push_changeLanguage(QTranslator * translator);

  protected:
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override {

        m_pinTop->setText(tr("窗口置顶"));
        m_darkMode->setEnabled(false);
        m_darkMode->setText(tr("深色模式(未完成)"));

        QIcon unchecked("");
        QIcon checked(":/image/checked#000000.svg");
        m_darkMode->setIcon((this->m_isDark) ? checked : unchecked);
        m_language->setIcon((this->m_isEnglish) ? checked : unchecked);

        m_contextMenu->exec(QCursor::pos());

        event->accept();
    }

  private slots:
    void on_connectButton_clicked();

    void on_updateButton_clicked();
    void on_uploadButton_clicked();
    void on_eraseButton_clicked();
    void on_jump2BootButton_clicked();
    void on_jump2AppButton_clicked();

    void on_sendButton_clicked();

    void on_pathButton_clicked();
    void on_pathEdit_editingFinished();
    void on_encryptButton_clicked();

    void on_baudrateCombo_currentIndexChanged(int index);
    void on_XModemButton_stateChanged(int arg);

    void get_portState(Serial::portState state);
    void get_rxData(const QByteArray &data);
    void get_portList(const QString &newPort, const QStringList &port, const QStringList &description);
    void get_xymodemState(quint8 state, quint16 progressValue, quint16 error);

  private:
    Ui::Widget *m_ui;
    Serial *m_serial;
    XYmodem *m_xymodem;
    QThread *m_xythread;
    EncryptDialog *m_encryptDialog;
    QSettings *m_setting;
    QTranslator *m_translator;
    QFileSystemWatcher *m_fileWatcher;
    QIntValidator *m_baudrateValidator;

    QMenu *m_contextMenu;
    QAction *m_pinTop;
    QAction *m_language;
    QAction *m_darkMode;

    QByteArray m_fileData;
    QFileInfo m_fileInfo;
    QStringList m_portDescription;

    bool m_isStart = false;
    bool m_isConnect = false;
    bool m_isDataDone = false;
    bool m_isIap = false;
    bool m_isDark = false;
    bool m_isEnglish = false;

    int m_fontSize = 12;
    quint32 m_rxCount = 0;
    quint32 m_txCount = 0;

    QColor m_rxColor;
    QColor m_txColor;
    QColor m_textColor;
    QString m_recordPath;
    QString m_recordSuffix;

    QTextCursor m_messageCursor;
    QTextCharFormat m_messageFormat;

    QElapsedTimer m_spendTimer;

    void appInit();
    void creatContextMenu();
    void controlEnable(quint16 bit);
    void file2Data(const QString &path);
    bool Hex2Bin(QFile &file);
    void insertMessage(const QString symbol, const QByteArray &data);
    void showFileInfo(logLevel level);
    void printLog(logLevel level, const QString &text);
    void shakeWindow();
};

#endif // WIDGET_H
