#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H
#include <QMenu>
#include <QPlainTextEdit>
#include <qglobal.h>
#include <qobject.h>

class plainTextEdit : public QPlainTextEdit {

    Q_OBJECT

  public:
    plainTextEdit(QWidget *parent = nullptr) : QPlainTextEdit(parent) {

        m_contextMenu = new QMenu(this);
        m_clear = new QAction(m_contextMenu);
        m_format = new QAction(m_contextMenu);
        m_contextMenu->addAction(m_clear);
        m_contextMenu->addSeparator();
        m_contextMenu->addAction(m_format);
        connect(m_format, &QAction::triggered, [this]() { m_isHex = !m_isHex; });
        connect(m_clear, &QAction::triggered, this, &QPlainTextEdit::clear);

        m_subContextMenu = new QMenu(m_contextMenu);
        m_NONE = new QAction("None", m_contextMenu);
        m_CR = new QAction("CR", m_contextMenu);
        m_LF = new QAction("LF", m_contextMenu);
        m_CRLF = new QAction("CRLF", m_contextMenu);
        m_LFCR = new QAction("LFCR", m_contextMenu);
        m_subContextMenu->addActions({m_NONE, m_CR, m_LF, m_CRLF, m_LFCR});
        m_subContextMenu->insertSeparator(m_CR);
        m_contextMenu->addMenu(m_subContextMenu);
        connect(m_NONE, &QAction::triggered, [this]() { m_feedMode = 0; });
        connect(m_CR, &QAction::triggered, [this]() { m_feedMode = 1; });
        connect(m_LF, &QAction::triggered, [this]() { m_feedMode = 2; });
        connect(m_CRLF, &QAction::triggered, [this]() { m_feedMode = 3; });
        connect(m_LFCR, &QAction::triggered, [this]() { m_feedMode = 4; });
    };

    bool m_isHex = 0;
    int m_feedMode = 0;

  protected:
    void contextMenuEvent(QContextMenuEvent *event) override {

        QIcon icon(":/image/checked#000000.svg");
        QAction *actions[] = {m_NONE, m_CR, m_LF, m_CRLF, m_LFCR};
        QStringList EOL = {"NONE", "CR", "LF", "CRLF", "LFCR"};

        m_clear->setText(tr("清除输入"));
        m_format->setText(tr("Hex格式"));

        m_subContextMenu->setDisabled(m_isHex);
        m_format->setIcon((this->m_isHex) ? icon : QIcon(""));

        for(auto action : actions) {
            action->setIcon(QIcon(""));
        }
        actions[m_feedMode]->setIcon(icon);
        m_subContextMenu->setTitle(QString(tr("行尾序列") + ": %1").arg(EOL[m_feedMode]));

        m_contextMenu->exec(QCursor::pos());

        event->accept();
    }

  private:
    QMenu *m_contextMenu;
    QMenu *m_subContextMenu;
    QAction *m_clear;
    QAction *m_format;
    QAction *m_NONE;
    QAction *m_CR;
    QAction *m_LF;
    QAction *m_CRLF;
    QAction *m_LFCR;
};

#endif