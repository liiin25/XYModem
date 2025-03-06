#include "widget.h"
#include "./ui_widget.h"
#include "encryptdialog.h"
#include "plaintextedit.h"
#include "textbrowser.h"
#include "xymodem.h"
#include <QScrollBar>
#include <QPropertyAnimation>
#include <qobject.h>

Widget::Widget(QWidget *parent) : QWidget(parent), m_ui(new Ui::Widget) {

    appInit();
}

Widget::~Widget() {

    emit push_exitCMD();
    m_xythread->wait();

    m_xymodem->deleteLater();
    m_xythread->deleteLater();

    delete m_ui;
}

void Widget::on_connectButton_clicked() {

    Serial::serialInfo_TypeDef info;
    if(m_ui->connectButton->text() == tr("连接")) {
        info.portName = m_ui->portCombo->currentText();
        info.baudrate = m_ui->baudrateCombo->currentText().toUInt();
        QVariant data = m_ui->dataBitCombo->currentData();
        info.dataBit = data.value<QSerialPort::DataBits>();
        data = m_ui->parityCombo->currentData();
        info.parity = data.value<QSerialPort::Parity>();
        data = m_ui->stopBitCombo->currentData();
        info.stopBit = data.value<QSerialPort::StopBits>();
        data = m_ui->flowCtrlCombo->currentData();
        info.flowCtrl = data.value<QSerialPort::FlowControl>();
        emit push_serialConnect(true, info);
    } else {
        emit push_serialConnect(false, info);
    }
}

void Widget::on_updateButton_clicked() {

    if(m_ui->updateButton->text() == tr("更新固件")) {

        m_isIap = true;
        m_spendTimer.start();
        controlEnable(0x0020);

        const QString fileName(m_fileInfo.fileName());
        m_ui->updateButton->setText(tr("更新中止"));
        emit push_updateCMD(m_ui->XModemButton->checkState(), fileName, m_fileData);
    } else {
        m_isIap = false;
        emit push_stopCMD(0);
        m_ui->updateButton->setText(tr("更新固件"));
    }
}

void Widget::on_uploadButton_clicked() {

    m_isIap = true;
    m_spendTimer.start();
    emit push_uploadCMD();
}

void Widget::on_eraseButton_clicked() {

    m_isIap = true;
    m_spendTimer.start();
    controlEnable(0x0000);
    m_ui->eraseButton->setText(tr("正在擦除..."));
    emit push_eraseCMD();
}

void Widget::on_jump2BootButton_clicked() {
    emit push_txData("BOOTLOADER\n");
}

void Widget::on_jump2AppButton_clicked() {
    emit push_txData("JUMP2APP\n");
}

void Widget::on_sendButton_clicked() {

    if(!m_ui->inputEdit->toPlainText().isEmpty()) {
        QByteArray txData;
        if(m_ui->inputEdit->m_isHex) {
            QString hex(m_ui->inputEdit->toPlainText());
            if(hex.length() % 2) hex.removeLast();
            QRegularExpression regex("[^0-9A-Fa-f]");
            hex.replace(regex, "0");
            txData = QByteArray::fromHex(hex.toLatin1());
        } else {
            txData = m_ui->inputEdit->toPlainText().toUtf8();
            switch(m_ui->inputEdit->m_feedMode) {
                case 1: txData.append("\r"); break;
                case 2: txData.append("\n"); break;
                case 3: txData.append("\r\n"); break;
                case 4: txData.append("\n\r"); break;
                default: break;
            }
        }
        if(txData.size() > 1024 * 256)
            txData.resize(1024 * 256);

        if(m_ui->messageBrowser->m_isEcho) {
            insertMessage("Tx", txData);
        }
        m_txCount += txData.size();
        m_ui->txLabel->setText(QString::number(m_txCount) + " Bytes");
        emit push_txData(txData);
    }
}

void Widget::on_pathButton_clicked() {

    // clang-format off
    QString path(QFileDialog::getOpenFileName(this, tr("选择固件文件"), \
                                                        m_recordPath,  \
                                                        "*.bin;;*.hex;;*.aes",
                                                        &m_recordSuffix));
    // clang-format on
    if(!path.isEmpty())
        file2Data(path);
}

void Widget::on_pathEdit_editingFinished() {

    QString path(m_ui->pathEdit->text());
    if(m_recordPath != path)
        file2Data(path);
}

void Widget::on_encryptButton_clicked() {

    m_encryptDialog->fileInfo = m_fileInfo;
    m_encryptDialog->plainText = m_fileData;

    m_encryptDialog->show();
    m_encryptDialog->exec();
}

void Widget::on_baudrateCombo_currentIndexChanged(int index) {

    if(m_ui->baudrateCombo->currentIndex() == 7) {
        m_ui->baudrateCombo->setEditable(true);
        m_ui->baudrateCombo->clearEditText();
        m_ui->baudrateCombo->setValidator(m_baudrateValidator);
        m_ui->baudrateCombo->lineEdit()->setPlaceholderText(tr("自定义"));
    } else {
        m_ui->baudrateCombo->setEditable(false);
    }
}

void Widget::on_XModemButton_stateChanged(int arg) {
    if(arg == Qt::Checked)
        printLog(Message, tr("使用XModem传输。"));
    else
        printLog(Message, tr("使用YModem传输。"));
}

void Widget::get_portList(const QString &newPort, const QStringList &port, const QStringList &description) {

    m_ui->portCombo->clear();
    m_ui->portCombo->addItems(port);
    m_ui->portCombo->hidePopup();
    m_ui->portCombo->setCurrentText(newPort);
    m_portDescription = description;

    if(!m_isStart) {
        QString str(m_setting->value("XYModem/Port", "").toString());
        if(port.contains(str))
            m_ui->portCombo->setCurrentText(str);
        m_isStart = true;
    }
}

void Widget::get_portState(Serial::portState state) {

    m_isConnect = false;
    if(state == Serial::OPEN) { // 串口连接成功
        m_isConnect = true;
        m_ui->connectButton->setText(tr("断开"));
        m_ui->connectButton->setStyleSheet("QToolButton { color: #d81e06; }");
        controlEnable(0x01ff);
        // clang-format off
        QString serial(QStringLiteral("%1(%2)-%3-%4-%5-%6-%7")
                                        .arg(m_ui->portCombo->currentText())
                                        .arg(m_portDescription[m_ui->portCombo->currentIndex()])
                                        .arg(m_ui->baudrateCombo-> currentText())
                                        .arg(m_ui->dataBitCombo->currentText())
                                        .arg(m_ui->stopBitCombo->currentText())
                                        .arg(m_ui->parityCombo->currentText())
                                        .arg(m_ui->flowCtrlCombo->currentText()));
        printLog(Connect, serial);
        m_ui->tabWidget->setCurrentIndex(1);
    } else {
        m_ui->connectButton->setText(tr("连接"));
        m_ui->connectButton->setStyleSheet("QToolButton { color: #000000; }");
        controlEnable(0x7fc0);
    }

    if(state == Serial::ERROR) {
        printLog(Error, tr("串口连接失败，请检查设备与参数。"));
    } else if(state == Serial::LOSS) {
        emit push_stopCMD(1);
        printLog(Error, tr("串口连接中断，请检查设备。"));
    }

    // m_ui->updateButton->setEnabled(isConnect && isDataDone);
}

void Widget::get_xymodemState(quint8 state, quint16 progressValue, quint16 error) {

    if(state > XYmodem::progress) {
        m_isIap = false;
        m_ui->updateButton->setText(tr("更新固件"));
        m_ui->uploadButton->setText(tr("读取固件"));
        m_ui->eraseButton->setText(tr("擦除固件"));
        controlEnable(0x01ff);
    }

    switch(state) {
        case XYmodem::updateOK: {
            // clang-format off
            printLog(Message, tr("更新成功:") \
                + QString::number(m_spendTimer.elapsed()) + "ms");
            // clang-format on
            shakeWindow();
            break;
        }
        case XYmodem::uploadOK: {
            // clang-format off
            printLog(Message, tr("读取成功:") \
                + QString::number(m_spendTimer.elapsed()) + "ms");
            // clang-format on
            break;
        }
        case XYmodem::eraseOK: {
            // clang-format off
            printLog(Message, tr("擦除成功:") \
                + QString::number(m_spendTimer.elapsed()) + "ms");
            // clang-format on
            shakeWindow();
            break;
        }
        case XYmodem::updateFail: {
            QString str;
            // clang-format off
            if(error == 4) str = tr("数据重传超出次数！");
            else if(error == 5) str = tr("固件超出大小！");
            else if(error == 6) str = tr("设备刷写失败！");
            else str = tr("设备无应答！");
            // clang-format on
            printLog(Error, str);
            break;
        }
        case XYmodem::uploadFail: printLog(Error, tr("读取失败！")); break;
        case XYmodem::eraseFail: printLog(Error, tr("擦除失败！")); break;

        case XYmodem::updateCancel: printLog(Warn, tr("更新中止！")); break;
        case XYmodem::uploadCancel: printLog(Warn, tr("读取中止！")); break;

        case XYmodem::disconnect: controlEnable(0x7fc0); break;
    }

    m_ui->progressBar->setValue(progressValue);
}

void Widget::get_rxData(const QByteArray &data) {

    if(!m_isIap) {
        insertMessage("Rx", data);
        m_rxCount += data.size();
        m_ui->rxLabel->setText(QString::number(m_rxCount) + " Bytes");
    }
}

void Widget::creatContextMenu() {

    m_contextMenu = new QMenu(this);
    m_language = new QAction("English", m_contextMenu);
    m_pinTop = new QAction(m_contextMenu);
    m_darkMode = new QAction(m_contextMenu);
    m_contextMenu->addActions({m_pinTop, m_language, m_darkMode});
    m_contextMenu->insertSeparator(m_language);
    connect(m_language, &QAction::triggered, [this]() {
        m_isEnglish = !m_isEnglish;
        printLog(Restart, tr("重启应用以更改语言。"));
    });
    connect(m_pinTop, &QAction::triggered, [this]() {
        if((this->windowFlags() & Qt::WindowStaysOnTopHint) == 0) {
            this->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
            m_pinTop->setIcon(QIcon(":/image/pinned#000000.svg"));
        } else {
            this->setWindowFlags(windowFlags() & (~Qt::WindowStaysOnTopHint));
            m_pinTop->setIcon(QIcon(""));
        }
        this->show();
    });
    // connect(m_darkMode, &QAction::triggered, [this]() { isDark = !isDark; });
}
void Widget::appInit() {

    m_ui->setupUi(this);
    creatContextMenu();

    m_encryptDialog = new EncryptDialog(this);
    connect(this, &Widget::push_changeLanguage, m_encryptDialog, &EncryptDialog::get_changeLanguage);

    m_serial = new Serial(this);
    connect(m_serial, &Serial::push_portList, this, &Widget::get_portList);
    connect(m_serial, &Serial::push_portState, this, &Widget::get_portState);
    connect(m_serial, &Serial::push_rxData, this, &Widget::get_rxData);
    connect(this, &Widget::push_serialConnect, m_serial, &Serial::get_serialConnect);
    connect(this, &Widget::push_txData, m_serial, &Serial::get_txData);

    m_xymodem = new XYmodem();
    m_xythread = new QThread();
    m_xymodem->moveToThread(m_xythread);
    connect(m_serial, &Serial::push_rxData, m_xymodem, &XYmodem::get_rxData);
    connect(m_xymodem, &XYmodem::push_txData, m_serial, &Serial::get_txData);
    connect(m_xymodem, &XYmodem::push_xymodemState, this, &Widget::get_xymodemState);
    connect(this, &Widget::push_updateCMD, m_xymodem, &XYmodem::get_updateCMD);
    connect(this, &Widget::push_uploadCMD, m_xymodem, &XYmodem::get_uploadCMD);
    connect(this, &Widget::push_eraseCMD, m_xymodem, &XYmodem::get_eraseCMD);
    connect(this, &Widget::push_stopCMD, m_xymodem, &XYmodem::get_stopCMD);
    connect(this, &Widget::push_exitCMD, m_xymodem, &XYmodem::get_exitCMD);
    m_xythread->start();

    // clang-format off
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, [this]() {
        file2Data(m_recordPath);
    });
    // clang-format on

    /* 加载配置文件 */
    const QString iniPath("./xymodem.ini");
    m_setting = new QSettings(iniPath, QSettings::IniFormat, this);
    QString filePath;
    if(!QFile::exists(iniPath)) {
        m_ui->splitter->setSizes({309, 65});
        /* ini文件没有找到则在软件目录下新建 */
        printLog(Message, "xymodem.ini文件不存在，将在软件目录下创建。");
        printLog(Message, "The xymodem.ini file does not exist."
                          "It will be created in the software directory.");
    } else {
        m_setting->beginGroup("XYModem");
        filePath = m_setting->value("FilePath", "").toString();
        m_ui->dataBitCombo->setCurrentText(m_setting->value("DataBit", "8").toString());
        m_ui->parityCombo->setCurrentText(m_setting->value("Parity", "none").toString());
        m_ui->stopBitCombo->setCurrentText(m_setting->value("StopBit", "1").toString());
        m_ui->flowCtrlCombo->setCurrentText(m_setting->value("FlowCtrl", "none").toString());
        m_ui->messageBrowser->m_isTimestamp = m_setting->value("Timestamp", false).toBool();
        m_ui->messageBrowser->m_isPcktLen = m_setting->value("PacketLength", false).toBool();
        m_ui->messageBrowser->m_isEcho = m_setting->value("EchoSend", false).toBool();
        m_ui->messageBrowser->m_isHex = m_setting->value("MessageHex", false).toBool();
        m_ui->messageBrowser->m_isPrefix = m_setting->value("Prefix", false).toBool();
        m_ui->inputEdit->m_isHex = m_setting->value("InputHex", false).toBool();
        m_ui->inputEdit->m_feedMode = m_setting->value("FeedMode", 0).toInt();
        m_ui->splitter->restoreState(m_setting->value("SplitterState").toByteArray());
        m_isEnglish = m_setting->value("English", false).toBool();
        m_isDark = m_setting->value("DarkMode", false).toBool();
        this->resize(m_setting->value("Width", 1000).toInt(), m_setting->value("height", 500).toInt());
        m_fontSize = m_setting->value("FontSize", 12).toInt();
        m_rxColor.setNamedColor(m_setting->value("RxColor").toString());
        m_txColor.setNamedColor(m_setting->value("TxColor").toString());
        m_textColor.setNamedColor(m_setting->value("TextColor").toString());
        // m_ui->tabWidget->setCurrentIndex(m_setting->value("Tab", 0).toInt());
        /* 设置波特率 */
        QString baudrate(m_setting->value("Baudrate", "115200").toString());
        m_baudrateValidator = new QIntValidator(1, 99999999, this);
        if(m_ui->baudrateCombo->findText(baudrate) == -1) {
            m_ui->baudrateCombo->setCurrentIndex(7);
            m_ui->baudrateCombo->setCurrentText(baudrate);
        } else {
            m_ui->baudrateCombo->setCurrentText(baudrate);
        }
        /* 设置XModem选框 */
        if(m_setting->value("XModem", false).toBool()) {
            m_ui->XModemButton->setCheckState(Qt::Checked);
        } else {
            m_ui->XModemButton->setCheckState(Qt::Unchecked);
        }
        m_setting->endGroup();
    }

    /* 检查颜色是否合法 */
    if(!m_rxColor.isValid())
        m_rxColor.setNamedColor("#007bff");
    if(!m_txColor.isValid())
        m_txColor.setNamedColor("#ff8400");
    if(!m_textColor.isValid())
        m_textColor.setNamedColor("#000000");

    /* 设置中英文 */
    m_translator = new QTranslator(this);
    if(m_isEnglish && m_translator->load(":/translations/en_US.qm")) {
        qApp->installTranslator(m_translator);
        m_ui->retranslateUi(this);
        emit push_changeLanguage(m_translator);
    }

    /* 信息窗口和输入框字体 */
    int fontId = QFontDatabase::addApplicationFont(":/ttf/JetBrainsMonoNL-SemiBold.ttf");
    if(fontId != -1) {
        QStringList fontFamilies(QFontDatabase::applicationFontFamilies(fontId));
        QFont font(fontFamilies[0], m_fontSize);
        m_ui->inputEdit->setFont(font);
        m_ui->messageBrowser->setFont(font);
        m_ui->logBrowser->setFont(font);
        m_ui->startBrowser->setFont(font);
    }

    /* 设置换行模式 */
    m_ui->messageBrowser->setWordWrapMode(QTextOption::WrapAnywhere);
    m_ui->logBrowser->setWordWrapMode(QTextOption::WrapAnywhere);

    /* 获取光标和默认格式 */
    m_messageCursor = m_ui->messageBrowser->textCursor();
    m_messageFormat = m_ui->messageBrowser->textCursor().charFormat();

    /* 加载启动页 */
    QFile file(":/start/startlogo.txt");
    if(file.open(QFile::ReadOnly)) {
        const QString str(file.readAll());
        file.close();
        m_ui->startBrowser->setFontPointSize(8);
        m_ui->startBrowser->insertPlainText(str);
    }
    file.setFileName(QStringLiteral(":/start/start_%1.html").arg(m_isEnglish ? "EN" : "ZH"));
    if(file.open(QFile::ReadOnly)) {
        const QString str(file.readAll());
        file.close();
        m_ui->startBrowser->insertHtml(str);
    }

    /* 加载文件 */
    file2Data(filePath);

    /* 设置串口数据 */
    m_ui->dataBitCombo->setItemData(0, QSerialPort::Data5);
    m_ui->dataBitCombo->setItemData(1, QSerialPort::Data6);
    m_ui->dataBitCombo->setItemData(2, QSerialPort::Data7);
    m_ui->dataBitCombo->setItemData(3, QSerialPort::Data8);
    m_ui->parityCombo->setItemData(0, QSerialPort::NoParity);
    m_ui->parityCombo->setItemData(1, QSerialPort::EvenParity);
    m_ui->parityCombo->setItemData(2, QSerialPort::OddParity);
    m_ui->parityCombo->setItemData(3, QSerialPort::SpaceParity);
    m_ui->parityCombo->setItemData(4, QSerialPort::MarkParity);
    m_ui->stopBitCombo->setItemData(0, QSerialPort::OneStop);
    m_ui->stopBitCombo->setItemData(1, QSerialPort::OneAndHalfStop);
    m_ui->stopBitCombo->setItemData(2, QSerialPort::TwoStop);
    m_ui->flowCtrlCombo->setItemData(0, QSerialPort::NoFlowControl);
    m_ui->flowCtrlCombo->setItemData(1, QSerialPort::HardwareControl);
    m_ui->flowCtrlCombo->setItemData(2, QSerialPort::SoftwareControl);

    connect(m_ui->messageBrowser, &textBrowser::clearMessage, [this]() {
        m_txCount = 0;
        m_rxCount = 0;
        m_ui->rxLabel->setText("0 Bytes");
        m_ui->txLabel->setText("0 Bytes");
    });
}

void Widget::closeEvent(QCloseEvent *event) {

    m_setting->beginGroup("XYModem");
    m_setting->setValue("FilePath", m_recordPath);
    m_setting->setValue("Port", m_ui->portCombo->currentText());
    m_setting->setValue("DataBit", m_ui->dataBitCombo->currentText());
    m_setting->setValue("Parity", m_ui->parityCombo->currentText());
    m_setting->setValue("StopBit", m_ui->stopBitCombo->currentText());
    m_setting->setValue("FlowCtrl", m_ui->flowCtrlCombo->currentText());
    m_setting->setValue("XModem", m_ui->XModemButton->isChecked());
    m_setting->setValue("Timestamp", m_ui->messageBrowser->m_isTimestamp);
    m_setting->setValue("PacketLength", m_ui->messageBrowser->m_isPcktLen);
    m_setting->setValue("EchoSend", m_ui->messageBrowser->m_isEcho);
    m_setting->setValue("MessageHex", m_ui->messageBrowser->m_isHex);
    m_setting->setValue("Prefix", m_ui->messageBrowser->m_isPrefix);
    m_setting->setValue("InputHex", m_ui->inputEdit->m_isHex);
    m_setting->setValue("FeedMode", m_ui->inputEdit->m_feedMode);
    m_setting->setValue("English", m_isEnglish);
    m_setting->setValue("DarkMode", m_isDark);
    m_setting->setValue("Width", width());
    m_setting->setValue("height", height());
    m_setting->setValue("FontSize", m_fontSize);
    m_setting->setValue("RxColor", m_rxColor.name());
    m_setting->setValue("TxColor", m_txColor.name());
    m_setting->setValue("TextColor", m_textColor.name());
    m_setting->setValue("SplitterState", m_ui->splitter->saveState());
    // m_setting->setValue("Tab", m_ui->tabWidget->currentIndex());
    if(!m_ui->baudrateCombo->currentText().isEmpty())
        m_setting->setValue("Baudrate", m_ui->baudrateCombo->currentText());
    m_setting->endGroup();
    m_setting->sync();
    event->accept();
}

void Widget::insertMessage(const QString symbol, const QByteArray &data) {

    int max = m_ui->messageBrowser->verticalScrollBar()->maximum();
    int current = m_ui->messageBrowser->verticalScrollBar()->value();

    QString packetHead;
    if(m_ui->messageBrowser->m_isTimestamp) {
        packetHead = QStringLiteral("[%1|%2]:")
            .arg(symbol).arg(QTime::currentTime().toString("hh:mm:ss.zzz"));
    } else {
        packetHead = QStringLiteral("[%1]:").arg(symbol);
    }

    QString rxData;
    if(m_ui->messageBrowser->m_isHex) {
        QString prefix;
        quint32 size = data.size();
        if(m_ui->messageBrowser->m_isPrefix) {
            prefix = "0x";
            rxData.reserve(size * 5);
        } else {
            prefix = "";
            rxData.reserve(size * 3);
        }

        for (int i = 0; i < size; ++i) {
            rxData += prefix % QString("%1 ")
                .arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        }
        rxData.chop(1);
        m_ui->messageBrowser->setWordWrapMode(QTextOption::WordWrap);
    } else {
        rxData = data;
        rxData.replace('\0', ' ');
        m_ui->messageBrowser->setWordWrapMode(QTextOption::WrapAnywhere);
    }

    QString packetTail;
    if(m_ui->messageBrowser->m_isPcktLen) {
            packetTail = QStringLiteral("[%1]").arg(data.size());
    }

    quint32 browserSize(m_ui->messageBrowser->toPlainText().size());
    if(browserSize > 1024 * 512) {
        m_messageCursor.setPosition(0);
        m_messageCursor.setPosition(1024 * 256, QTextCursor::KeepAnchor);
        m_messageCursor.removeSelectedText();
        m_messageCursor.movePosition(QTextCursor::End);
    }

    QString color((symbol == "Tx") ? m_txColor.name() : m_rxColor.name());
    m_ui->messageBrowser->append(QStringLiteral("<span style = 'color:%1;'>%2</span>")
                                                .arg(color).arg(packetHead));
    m_messageFormat.setForeground(m_textColor);
    m_messageCursor.insertText(rxData, m_messageFormat);
    m_messageCursor.insertHtml(QStringLiteral("<span style = 'color:%1;'>%2</span>")
                                                .arg(color).arg(packetTail));

    if(current == max) {
        max = m_ui->messageBrowser->verticalScrollBar()->maximum();
        m_ui->messageBrowser->verticalScrollBar()->setValue(max);
    }
}

void Widget::file2Data(const QString &path) {

    m_isDataDone = false;
    m_fileData.clear();
    m_fileInfo = QFileInfo(path);

    if(path.isEmpty()) {
        printLog(Message, tr("请载入固件文件！"));
    } else if(!m_fileInfo.isFile()) {
        m_recordPath.clear();
        printLog(Error, tr("文件或路径不存在！"));
    } else {
        QFile file(path);
        if(!file.open(QFile::ReadOnly)) {
            m_fileWatcher->removePath(m_recordPath);
            printLog(Error, tr("文件打开失败！"));
        } else {
            QString suffix(m_fileInfo.suffix());
            if(suffix == "hex") {
                if(Hex2Bin(file))
                    m_isDataDone = true;
            } else if((suffix == "bin") || (suffix == "aes")) {
                m_fileData = file.readAll();
                if((m_fileData.size()) && (file.error() == QFile::NoError))
                    m_isDataDone = true;
            }
            file.close();

            logLevel level(m_isDataDone ? Message : Error);

            m_fileWatcher->removePath(m_recordPath);
            m_fileWatcher->addPath(path);
            if(m_recordPath != path) {
                m_recordPath = path;
                showFileInfo(level);
            } else {
                printLog(level, tr("重新载入文件。"));
            }

            if(!m_isDataDone) {
                if(suffix == "hex") {
                    printLog(level, tr("文件转换失败！"));
                } else {
                    printLog(level, tr("文件读取失败！"));
                }
            } else {
                m_ui->progressBar->setValue(0);
            }
        }
    }

    m_ui->pathEdit->setText(path);
    m_ui->encryptButton->setEnabled(m_isDataDone);
    m_ui->updateButton->setEnabled(m_isConnect && m_isDataDone);
}

bool Widget::Hex2Bin(QFile &file) {

    auto hex2Sum = [](const QByteArray &checkData) -> char {
        char sum = 0;
        int size = checkData.size();
        for(int i = 0; i < size; i++) {
            sum += checkData[i];
        }
        return static_cast<char>(~sum + 1);
    };

    quint32 pos = 0;
    quint32 size = file.size();
    QByteArray data(file.readAll());
    if(data.size() && (file.error() == QFile::NoError)) {
        while(pos < size) {
            if(data[pos] != ':') {
                ++pos;
                continue;
            }

            quint16 byteCount = data.mid(pos + 1, 2).toInt(nullptr, 16);
            quint16 recordType = data.mid(pos + 7, 2).toInt(nullptr, 16);

            if(recordType == 0) {
                QByteArray checkData(QByteArray::fromHex(data.mid(pos + 1, byteCount * 2 + 8)));
                char checkSum = static_cast<char>(data.mid(pos + byteCount * 2 + 9, 2).toInt(nullptr, 16));

                if(hex2Sum(checkData) != checkSum) {
                    return false;
                }
                m_fileData.append(checkData.mid(4, byteCount));
            } else if(recordType == 1) {
                return m_fileData.isEmpty() ? false : true;
            }

            pos += byteCount * 2 + 11;
        }
    }

    return false;
}

void Widget::showFileInfo(logLevel level) {
    QString size, unit;
    int fileSize = m_fileInfo.size();
    if(fileSize >= 1024) {
        size = QString::number(static_cast<float>(fileSize) / 1024, 'f', 2);
        unit = " KB";
    } else {
        size = QString::number(fileSize);
        unit = " Bytes";
    }
    printLog(level, tr("文件名：") + m_fileInfo.fileName());
    printLog(level, tr("大小：") + size + unit);
    printLog(level, tr("路径：") + m_fileInfo.absolutePath());
}

void Widget::printLog(logLevel level, const QString &text) {

    QString symbol;
    QString color;

    switch(level) {
        case Widget::Connect: {
            symbol = tr(" 连接 ");
            color = "#007bff";
            break;
        }
        case Widget::Message: {
            symbol = tr(" 信息 ");
            color = "#69b779";
            break;
        }
        case Widget::Restart: {
            symbol = tr(" 重启 ");
            color = "#eebb44";
            break;
        }
        case Widget::Debug: {
            symbol = tr(" 调试 ");
            color = "#56b5c1";
            break;
        }
        case Widget::Warn: {
            symbol = tr(" 警告 ");
            color = "#fea443";
            break;
        }
        case Widget::Error: {
            symbol = tr(" 错误 ");
            color = "#ff0000";
            break;
        }
        default: {
            symbol = tr("严重错误");
            color = "#e74141";
            break;
        }
    }
    // clang-format off
    /* 注意：文本格式为保留空格 */
    QString log(QStringLiteral("<span style = 'color:%1; white-space:pre;'>[%2|%3] </span>"\
                                "<span>%4</span>")
                                .arg(color)
                                .arg(symbol)
                                .arg(QTime::currentTime().toString())
                                .arg(text));
    m_ui->logBrowser->append(log);
    // clang-format on

    /* 重启/调试/警告/错误/严重错误时跳转到log页面 */
    if(level > Widget::Message)
        m_ui->tabWidget->setCurrentIndex(2);

    QScrollBar *scrollBar = m_ui->logBrowser->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void Widget::controlEnable(quint16 bit) {

    m_ui->portCombo->setEnabled(bit & 0x4000);
    m_ui->baudrateCombo->setEnabled(bit & 0x2000);
    m_ui->dataBitCombo->setEnabled(bit & 0x1000);
    m_ui->parityCombo->setEnabled(bit & 0x800);
    m_ui->stopBitCombo->setEnabled(bit & 0x400);
    m_ui->flowCtrlCombo->setEnabled(bit & 0x200);
    m_ui->connectButton->setEnabled(bit & 0x100);

    m_ui->pathEdit->setEnabled(bit & 0x80);
    m_ui->pathButton->setEnabled(bit & 0x40);

    m_ui->updateButton->setEnabled((bit & 0x20) && m_isConnect && m_isDataDone);
    // m_ui->uploadButton->setEnabled(bit & 0x10);
    m_ui->eraseButton->setEnabled(bit & 0x08);
    m_ui->jump2BootButton->setEnabled(bit & 0x04);
    m_ui->jump2AppButton->setEnabled(bit & 0x02);
    m_ui->sendButton->setEnabled(bit & 0x01);
}


void Widget::shakeWindow() {

    QPropertyAnimation *m_animation = new QPropertyAnimation(this, "pos");

    m_animation->setDuration(500);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
    QPoint originalPos = this->pos();
    m_animation->setKeyValueAt(0.1, originalPos + QPoint(10, 0));
    m_animation->setKeyValueAt(0.2, originalPos + QPoint(-10, 0));
    m_animation->setKeyValueAt(0.3, originalPos + QPoint(10, 0));
    m_animation->setKeyValueAt(0.4, originalPos + QPoint(-10, 0));
    m_animation->setKeyValueAt(0.5, originalPos + QPoint(10, 0));
    m_animation->setKeyValueAt(0.6, originalPos + QPoint(-10, 0));
    m_animation->setKeyValueAt(0.7, originalPos + QPoint(10, 0));
    m_animation->setKeyValueAt(0.8, originalPos + QPoint(-10, 0));
    m_animation->setKeyValueAt(0.9, originalPos + QPoint(10, 0));
    m_animation->setKeyValueAt(1, originalPos);

    m_animation->start(QAbstractAnimation::DeleteWhenStopped);
}


