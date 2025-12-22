#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccess(int userId, const QString &role);

private slots:
    void onLogin();
    void onRegister();

private:
    QLineEdit *editLogin;
    QLineEdit *editPassword;
    QPushButton *btnLogin;
    QPushButton *btnRegister;
    QLabel *lblStatus;
};
