#include "LoginWindow.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    setWindowTitle("Авторизация");
    setMinimumSize(420, 520);
}

void LoginWindow::buildUi()
{
    // Верхняя панель: по центру "авторизация", справа сверху кнопка-меню
    m_titleLabel = new QLabel(QStringLiteral("авторизация"), this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    m_menuButton = new QToolButton(this);
    // Пока просто символ "≡" как три палочки
    m_menuButton->setText(QStringLiteral("≡"));
    QFont menuFont = m_menuButton->font();
    menuFont.setPointSize(menuFont.pointSize() + 6);
    m_menuButton->setFont(menuFont);
    m_menuButton->setAutoRaise(true);
    m_menuButton->setToolTip("Меню");

    auto* topRow = new QHBoxLayout();
    topRow->addStretch(1);
    topRow->addWidget(m_titleLabel, /*stretch*/0, Qt::AlignCenter);
    topRow->addStretch(1);
    topRow->addWidget(m_menuButton, 0, Qt::AlignRight | Qt::AlignTop);

    // Центр: логин/пароль
    m_loginLabel = new QLabel(QStringLiteral("логин"), this);
    QFont labelFont = m_loginLabel->font();
    labelFont.setPointSize(labelFont.pointSize() + 1);
    m_loginLabel->setFont(labelFont);

    m_loginEdit = new QLineEdit(this);
    m_loginEdit->setPlaceholderText("Введите логин");

    m_passwordLabel = new QLabel(QStringLiteral("пароль"), this);
    m_passwordLabel->setFont(labelFont);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Введите пароль");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    // Кнопка авторизации
    m_authButton = new QPushButton(QStringLiteral("авторизация"), this);

    // "забыл пароль" — серым, меньше
    m_forgotPasswordLabel = new QLabel(QStringLiteral("забыл пароль"), this);
    QFont smallFont = m_forgotPasswordLabel->font();
    smallFont.setPointSize(std::max(8, smallFont.pointSize() - 1));
    m_forgotPasswordLabel->setFont(smallFont);
    m_forgotPasswordLabel->setStyleSheet("color: gray;"); // пока минимально
    m_forgotPasswordLabel->setAlignment(Qt::AlignCenter);

    auto* formLayout = new QVBoxLayout();
    formLayout->setSpacing(10);

    formLayout->addWidget(m_loginLabel);
    formLayout->addWidget(m_loginEdit);

    formLayout->addSpacing(6);

    formLayout->addWidget(m_passwordLabel);
    formLayout->addWidget(m_passwordEdit);

    formLayout->addSpacing(14);
    formLayout->addWidget(m_authButton);
    formLayout->addWidget(m_forgotPasswordLabel);

    // Общая компоновка
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 18, 24, 24);
    root->addLayout(topRow);
    root->addSpacing(30);

    // Центрирование формы по вертикали
    root->addStretch(1);
    root->addLayout(formLayout);
    root->addStretch(2);

    setLayout(root);

    // Логику кликов пока НЕ добавляем (ты просил без “физики”).
}
