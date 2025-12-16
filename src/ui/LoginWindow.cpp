#include "LoginWindow.h"
#include "ToggleSwitch.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QFrame>
#include <QClipboard>
#include <QToolTip>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QApplication>
#include <QResizeEvent>
#include <QSpacerItem>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    createSideMenu();
    setWindowTitle("Авторизация");
    setMinimumSize(420, 520);

    // начальная тема — светлая
    applyTheme(m_darkTheme);
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
    m_menuButton->setText(QStringLiteral("≡"));
    QFont menuFont = m_menuButton->font();
    menuFont.setPointSize(menuFont.pointSize() + 6);
    m_menuButton->setFont(menuFont);
    m_menuButton->setAutoRaise(true);
    m_menuButton->setToolTip("Меню");
    connect(m_menuButton, &QToolButton::clicked, this, &LoginWindow::onMenuButtonClicked);

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
    m_forgotPasswordLabel->setStyleSheet("color: gray;");
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

    // Логику кликов для прочих элементов (например, аутентификация) можно добавить позже.
}

void LoginWindow::createSideMenu()
{
    // Side menu как панель справа
    m_sideMenu = new QFrame(this);
    m_sideMenu->setObjectName("sideMenu"); // для QSS
    m_sideMenu->setFrameShape(QFrame::StyledPanel);
    m_sideMenu->setFixedWidth(260);
    m_sideMenu->setVisible(false);

    QVBoxLayout* menuLayout = new QVBoxLayout(m_sideMenu);
    menuLayout->setContentsMargins(12, 12, 12, 12);
    menuLayout->setSpacing(12);

    QLabel* menuTitle = new QLabel(QStringLiteral("Меню"), m_sideMenu);
    QFont mt = menuTitle->font();
    mt.setPointSize(mt.pointSize() + 2);
    mt.setBold(true);
    menuTitle->setFont(mt);
    menuLayout->addWidget(menuTitle);

    // Пункт 1: переключатель темы в виде тумблера
    QLabel* themeLabel = new QLabel(QStringLiteral("Тёмная тема"), m_sideMenu);
    QFont labelFont = themeLabel->font();
    labelFont.setPointSize(labelFont.pointSize() + 0);
    themeLabel->setFont(labelFont);

    // Ряд с меткой и тумблером
    QHBoxLayout* themeRow = new QHBoxLayout();
    themeRow->addWidget(themeLabel);
    themeRow->addStretch();

    m_themeToggle = new ToggleSwitch(m_sideMenu);
    m_themeToggle->setChecked(m_darkTheme);
    connect(m_themeToggle, &ToggleSwitch::toggled, this, &LoginWindow::onThemeToggled);
    themeRow->addWidget(m_themeToggle);
    menuLayout->addLayout(themeRow);

    // Небольшая разделительная линия (стретчер)
    menuLayout->addSpacing(6);

    // Пункт 2: поддержка
    m_supportButton = new QPushButton(QStringLiteral("Поддержка"), m_sideMenu);
    connect(m_supportButton, &QPushButton::clicked, this, &LoginWindow::onSupportClicked);
    menuLayout->addWidget(m_supportButton);

    // Поле, куда будет отображаться email (скрыто по умолчанию)
    m_supportEmailLabel = new QLabel(QString(), m_sideMenu);
    QFont small = m_supportEmailLabel->font();
    small.setPointSize(std::max(8, small.pointSize() - 1));
    m_supportEmailLabel->setFont(small);
    m_supportEmailLabel->setStyleSheet("color: gray;");
    m_supportEmailLabel->setVisible(false);
    menuLayout->addWidget(m_supportEmailLabel);

    // Заполнитель внизу
    menuLayout->addStretch(1);

    // Позиционирование панели — в resizeEvent
}

void LoginWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (m_sideMenu) {
        const int w = m_sideMenu->width();
        const int h = height();
        const int x = width() - w - 12; // небольшой отступ справа
        m_sideMenu->setGeometry(x, 0, w, h);
    }
}

void LoginWindow::onMenuButtonClicked()
{
    if (!m_sideMenu)
        return;
    m_sideMenu->setVisible(!m_sideMenu->isVisible());
}

void LoginWindow::onThemeToggled(bool checked)
{
    m_darkTheme = checked;
    applyTheme(m_darkTheme);
}

void LoginWindow::onSupportClicked()
{
    const QString email = QStringLiteral("example.email.com");

    // Отобразить email в панели
    m_supportEmailLabel->setText(email);
    m_supportEmailLabel->setVisible(true);

    // Скопировать в буфер обмена
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(email);

    // Показать тултип рядом с кнопкой
    QPoint globalPos = m_supportButton->mapToGlobal(QPoint(m_supportButton->width()/2, m_supportButton->height()));
    QToolTip::showText(globalPos, QStringLiteral("Email скопирован в буфер обмена"), m_supportButton);
}

void LoginWindow::applyTheme(bool dark)
{
    if (dark) {
        const QString darkQss = QStringLiteral(R"(
            QWidget { background-color: #2b2b2b; color: #e6e6e6; }
            QLineEdit { background-color: #3c3f41; color: #e6e6e6; border: 1px solid #5a5a5a; padding: 4px; }
            QPushButton { background-color: #4a4a4a; color: #e6e6e6; border: 1px solid #666666; padding: 6px; border-radius: 3px; }
            QLabel { color: #e6e6e6; }
            QToolButton { color: #e6e6e6; background: transparent; }
            QFrame#sideMenu { background-color: #353535; border-left: 1px solid #444444; }
        )");
        qApp->setStyleSheet(darkQss);
    } else {
        const QString lightQss = QStringLiteral(R"(
            QWidget { background-color: #ffffff; color: #222222; }
            QLineEdit { background-color: #ffffff; color: #222222; border: 1px solid #cfcfcf; padding: 4px; }
            QPushButton { background-color: #f5f5f5; color: #222222; border: 1px solid #cfcfcf; padding: 6px; border-radius: 3px; }
            QLabel { color: #222222; }
            QToolButton { color: #222222; background: transparent; }
            QFrame#sideMenu { background-color: #f7f7f7; border-left: 1px solid #e0e0e0; }
        )");
        qApp->setStyleSheet(lightQss);
    }

    // Если нужно — можем обновлять кастомные виджеты здесь (например, цвета ToggleSwitch),
    // но ToggleSwitch использует палитру/свои цвета, поэтому обычно достаточно setStyleSheet.
}
