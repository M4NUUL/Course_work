#include "LoginWindow.hpp"
#include "../auth/AuthManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("EduDesk — Вход");
    auto v = new QVBoxLayout(this);
    editLogin = new QLineEdit();
    editLogin->setPlaceholderText("Логин");
    editPassword = new QLineEdit();
    editPassword->setPlaceholderText("Пароль");
    editPassword->setEchoMode(QLineEdit::Password);
    btnLogin = new QPushButton("Войти");
    btnRegister = new QPushButton("Зарегистрироваться");
    lblStatus = new QLabel("");

    v->addWidget(editLogin);
    v->addWidget(editPassword);
    auto h = new QHBoxLayout();
    h->addWidget(btnLogin);
    h->addWidget(btnRegister);
    v->addLayout(h);
    v->addWidget(lblStatus);

    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(btnRegister, &QPushButton::clicked, this, &LoginWindow::onRegister);
}

void LoginWindow::onLogin() {
    QString login = editLogin->text().trimmed();
    QString pwd = editPassword->text();
    if (login.isEmpty() || pwd.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите логин и пароль");
        return;
    }
    int userId = -1;
    std::string role;
    if (AuthManager::instance().authenticate(login.toStdString(), pwd.toStdString(), userId, role)) {
        emit loginSuccess(userId, QString::fromStdString(role));
    } else {
        QMessageBox::critical(this, "Ошибка", "Неверный логин или пароль");
    }
}

void LoginWindow::onRegister() {
    QMessageBox::information(this, "Регистрация", "Регистрация доступна через интерфейс администратора.");
}
