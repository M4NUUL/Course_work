#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

class LoginWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

private:
    void buildUi();

    QLabel* m_titleLabel = nullptr;
    QLabel* m_loginLabel = nullptr;
    QLabel* m_passwordLabel = nullptr;

    QLineEdit* m_loginEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;

    QPushButton* m_authButton = nullptr;
    QLabel* m_forgotPasswordLabel = nullptr;

    QToolButton* m_menuButton = nullptr; // “три палочки” (пока без поведения)
};
