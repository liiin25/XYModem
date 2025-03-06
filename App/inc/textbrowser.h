#ifndef TEXTBROWSER_H
#define TEXTBROWSER_H
#include <QMenu>
#include <QTextBrowser>
#include <qaction.h>
#include <qevent.h>
#include <qicon.h>
#include <qtextedit.h>
#include <qtmetamacros.h>

class textBrowser : public QTextBrowser {

    Q_OBJECT

  public:
    textBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {

        m_contextMenu = new QMenu(this);
        m_copy = new QAction(m_contextMenu);
        m_selectAll = new QAction(m_contextMenu);
        m_clear = new QAction(m_contextMenu);
        m_timestamp = new QAction(m_contextMenu);
        m_echoSend = new QAction(m_contextMenu);
        m_pcktLen = new QAction(m_contextMenu);
        m_format = new QAction(m_contextMenu);
        m_prefix = new QAction(m_contextMenu);
        m_contextMenu->addActions({m_copy, m_selectAll, m_clear, m_timestamp, \
                                    m_pcktLen, m_echoSend, m_format, m_prefix});
        m_contextMenu->insertSeparator(m_clear);
        m_contextMenu->insertSeparator(m_timestamp);
        m_contextMenu->insertSeparator(m_format);
        m_contextMenu->insertSeparator(m_prefix);
        connect(m_copy, &QAction::triggered, this, &QTextBrowser::copy);
        connect(m_selectAll, &QAction::triggered, this, &QTextBrowser::selectAll);
        connect(m_clear, &QAction::triggered, this, [this]{
            QTextBrowser::clear();
            emit clearMessage();
        });
        connect(m_timestamp, &QAction::triggered, [this]() { m_isTimestamp = !m_isTimestamp; });
        connect(m_echoSend, &QAction::triggered, [this]() { m_isEcho = !m_isEcho; });
        connect(m_pcktLen, &QAction::triggered, [this]() { m_isPcktLen = !m_isPcktLen; });
        connect(m_format, &QAction::triggered, [this]() { m_isHex = !m_isHex; });
        connect(m_prefix, &QAction::triggered, [this]() { m_isPrefix = !m_isPrefix; });
    };

    bool m_isTimestamp = true;
    bool m_isEcho = false;
    bool m_isPcktLen = true;
    bool m_isHex = false;
    bool m_isPrefix = false;

 signals:
    void clearMessage();

  protected:
    // FIXME: 防止Browser在只读状态下中文输入法还能输入
    void inputMethodEvent(QInputMethodEvent *event) override {
        event->ignore();
    }
    void contextMenuEvent(QContextMenuEvent *event) override {

        m_copy->setText(tr("复制"));
        m_selectAll->setText(tr("全选"));
        m_clear->setText(tr("清除信息"));
        m_timestamp->setText(tr("时间戳"));
        m_pcktLen->setText(tr("包字节"));
        m_echoSend->setText(tr("回显发送"));
        m_format->setText(tr("Hex格式"));
        m_prefix->setText(tr("0x前缀"));

        m_prefix->setEnabled(m_isHex);
        m_copy->setEnabled(this->textCursor().hasSelection());
        m_selectAll->setEnabled(!this->toPlainText().isEmpty());

        QIcon unChecked("");
        QIcon checked(":/image/checked#000000.svg");
        m_timestamp->setIcon((this->m_isTimestamp) ? checked : unChecked);
        m_pcktLen->setIcon((this->m_isPcktLen) ? checked : unChecked);
        m_echoSend->setIcon((this->m_isEcho) ? checked : unChecked);
        m_format->setIcon((this->m_isHex) ? checked : unChecked);
        m_prefix->setIcon((this->m_isPrefix) ? checked : unChecked);

        m_contextMenu->exec(QCursor::pos());

        event->accept();
    }

  private:
    QMenu *m_contextMenu;
    QAction *m_copy;
    QAction *m_selectAll;
    QAction *m_clear;
    QAction *m_timestamp;
    QAction *m_echoSend;
    QAction *m_pcktLen;
    QAction *m_format;
    QAction *m_prefix;
};

#endif