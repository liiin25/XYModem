#ifndef ENCRYPTDIALOG_H
#define ENCRYPTDIALOG_H
#include <QDialog>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <QTranslator>
#include <QFileInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
    class EncryptDialog;
}
QT_END_NAMESPACE

class EncryptDialog : public QDialog {
    Q_OBJECT

  public:
    EncryptDialog(QWidget *parent = nullptr);
    ~EncryptDialog();

    QByteArray plainText;
    QFileInfo fileInfo;

  public slots:
    void get_changeLanguage(QTranslator * translator);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    Ui::EncryptDialog *m_ui;

    QByteArray m_key;
    QByteArray m_iv;

  private slots:
    void on_keyEdit_textChanged(const QString &arg1);
    void on_ivEdit_textChanged(const QString &arg1);

    QByteArray aesCbcEncrypt(const QByteArray &plainText, const QByteArray &key, const QByteArray &iv);
};
#endif // ENCRYPTDIALOG_H
