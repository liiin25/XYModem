#include "encryptdialog.h"
#include "./ui_encryptdialog.h"
#include <QPushButton>
#include <qevent.h>
#include <openssl/err.h>

EncryptDialog::EncryptDialog(QWidget *parent) : QDialog(parent),
                                                m_ui(new Ui::EncryptDialog),
                                                m_key(16, 0),
                                                m_iv(16, 0) {

    m_ui->setupUi(this);

    this->setModal(true);
    connect(m_ui->generateButton, &QPushButton::clicked, [this]() {
        // clang-format off
        QByteArray CipherText = aesCbcEncrypt(plainText, m_key, m_iv);
        QString path(QStringLiteral("%1/%2%3")
                                    .arg(fileInfo.path())
                                    .arg(fileInfo.baseName())
                                    .arg(".aes"));
        // clang-format on
        QFile file(path);
        if(file.open(QFile::WriteOnly)) {
            if(file.write(CipherText) != -1) {
                m_ui->messageLabel->setText(tr("加密成功"));
            } else {
                m_ui->messageLabel->setText(tr("加密失败"));
            }
            file.close();
        }
    });

    OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS, nullptr);
}

EncryptDialog::~EncryptDialog() {

    OPENSSL_cleanup();
    delete m_ui;
}

void EncryptDialog::closeEvent(QCloseEvent *event) {

    if(m_key.size() == 16 && m_iv.size() == 16)
        m_ui->messageLabel->setText("");

    event->accept();
}

void EncryptDialog::get_changeLanguage(QTranslator * translator) {
    qApp->installTranslator(translator);
    m_ui->retranslateUi(this);
}

void EncryptDialog::on_keyEdit_textChanged(const QString &arg1) {

    m_key = arg1.toUtf8();
    if(m_key.size() == 16) {
        if(m_iv.size() == 16)
            m_ui->generateButton->setEnabled(true);
        m_ui->keyLabel->setPixmap(QPixmap(":/image/correct#69b779.svg"));
    } else {
        m_ui->generateButton->setEnabled(false);
        m_ui->keyLabel->setPixmap(QPixmap(":/image/incorrect#d0021b.svg"));
    }
}

void EncryptDialog::on_ivEdit_textChanged(const QString &arg1) {

    m_iv = arg1.toUtf8();
    if(m_iv.size() == 16) {
        if(m_key.size() == 16)
            m_ui->generateButton->setEnabled(true);
        m_ui->ivLabel->setPixmap(QPixmap(":/image/correct#69b779.svg"));
    } else {
        m_ui->generateButton->setEnabled(false);
        m_ui->ivLabel->setPixmap(QPixmap(":/image/incorrect#d0021b.svg"));
    }
}

QByteArray EncryptDialog::aesCbcEncrypt(const QByteArray &plainText, const QByteArray &key, const QByteArray &iv) {

    struct CtxDeleter {
        void operator()(EVP_CIPHER_CTX* ctx) { EVP_CIPHER_CTX_free(ctx); }
    };
    std::unique_ptr<EVP_CIPHER_CTX, CtxDeleter> ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return QByteArray();
    }

    struct CipherDeleter {
        void operator()(EVP_CIPHER* cipher) { EVP_CIPHER_free(cipher); }
    };
    std::unique_ptr<EVP_CIPHER, CipherDeleter> cipher(EVP_CIPHER_fetch(nullptr, "AES-128-CBC", nullptr));
    if (!cipher) {
        return QByteArray();
    }

    if (EVP_EncryptInit_ex2(ctx.get(), cipher.get(),
                            (const unsigned char*)(key.constData()),
                            (const unsigned char*)(iv.constData()),
                            nullptr) != 1) {
        return QByteArray();
    }

    QByteArray ciphertext(plainText.size() + EVP_CIPHER_get_block_size(cipher.get()), 0);

    int outl = 0, finalLen = 0;
    if (EVP_EncryptUpdate(ctx.get(), (unsigned char*)(ciphertext.data()),&outl,
                            (const unsigned char*)(plainText.constData()), plainText.size()) != 1) {
        return QByteArray();
    }

    if (EVP_EncryptFinal_ex(ctx.get(), (unsigned char*)(ciphertext.data()) + outl, &finalLen) != 1) {
        return QByteArray();
    }

    ciphertext.resize(outl + finalLen);
    return ciphertext;
}

