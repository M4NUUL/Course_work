#include "CreateUserDialog.hpp"
#include "../auth/PasswordUtils.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

CreateUserDialog::CreateUserDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Создать пользователя");
    auto v = new QVBoxLayout(this);

    auto form = new QFormLayout();
    editLogin = new QLineEdit();
    editPassword = new QLineEdit();
    editPassword->setEchoMode(QLineEdit::Password);
    editPasswordRepeat = new QLineEdit();
    editPasswordRepeat->setEchoMode(QLineEdit::Password);
    editFullName = new QLineEdit();
    editEmail = new QLineEdit();
    comboRole = new QComboBox();
    comboRole->addItems({"student","teacher","admin"});

    form->addRow("Логин:", editLogin);
    form->addRow("Пароль:", editPassword);
    form->addRow("Повтор пароля:", editPasswordRepeat);
    form->addRow("Полное имя:", editFullName);
    form->addRow("Email:", editEmail);
    form->addRow("Роль:", comboRole);

    v->addLayout(form);

    lblError = new QLabel();
    lblError->setStyleSheet("color:red");
    v->addWidget(lblError);

    auto h = new QHBoxLayout();
    auto btnOk = new QPushButton("OK");
    auto btnCancel = new QPushButton("Отмена");
    h->addStretch();
    h->addWidget(btnOk);
    h->addWidget(btnCancel);
    v->addLayout(h);

    connect(btnOk, &QPushButton::clicked, this, &CreateUserDialog::onOk);
    connect(btnCancel, &QPushButton::clicked, this, &CreateUserDialog::reject);
}

QString CreateUserDialog::login() const { return editLogin->text().trimmed(); }
QString CreateUserDialog::password() const { return editPassword->text(); }
QString CreateUserDialog::role() const { return comboRole->currentText(); }
QString CreateUserDialog::fullName() const { return editFullName->text().trimmed(); }
QString CreateUserDialog::email() const { return editEmail->text().trimmed(); }

void CreateUserDialog::onOk() {
    QString l = login();
    QString p1 = password();
    QString p2 = editPasswordRepeat->text();
    if (l.isEmpty() || p1.isEmpty()) {
        lblError->setText("Логин и пароль обязательны");
        return;
    }
    if (p1 != p2) {
        lblError->setText("Пароли не совпадают");
        return;
    }
    std::string err;
    if (!auth::validatePasswordRules(p1.toStdString(), err)) {
        lblError->setText(QString::fromStdString(err));
        return;
    }
    accept();
}
