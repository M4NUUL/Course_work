#include "MainWindow.hpp"

#include "LoginWindow.hpp"
#include "TeacherWindow.hpp"
#include "StudentWindow.hpp"
#include "AdminWindow.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QLabel>

MainWindow::MainWindow(int userId, const QString &role, QWidget *parent)
    : QMainWindow(parent), m_userId(userId), m_role(role)
{
    setupUi();
}

void MainWindow::setupUi() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    auto v = new QVBoxLayout(central);

    // Верхняя панель с кнопкой выхода
    auto topBar = new QHBoxLayout();
    topBar->addStretch();
    btnLogout = new QPushButton("Выйти", this);
    topBar->addWidget(btnLogout);
    v->addLayout(topBar);

    // Основная область содержимого
    if (m_role == "teacher") {
        TeacherWindow *tw = new TeacherWindow(m_userId, this);
        v->addWidget(tw, 1);
    } else if (m_role == "student") {
        StudentWindow *sw = new StudentWindow(m_userId, this);
        v->addWidget(sw, 1);
    } else if (m_role == "admin") {
        AdminWindow *aw = new AdminWindow(m_userId, this);
        v->addWidget(aw, 1);
    } else {
        QLabel *lbl = new QLabel(QString("Неизвестная роль: %1").arg(m_role), this);
        v->addWidget(lbl);
    }

    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogout);
}

void MainWindow::onLogout() {
    // Создание окна входа и отображение его пользователю
    LoginWindow *login = new LoginWindow();
    login->setAttribute(Qt::WA_DeleteOnClose);
    login->show();

    // При успешной авторизации открывается новое главное окно
    connect(login, &LoginWindow::loginSuccess, [login](int userId, const QString &role) {
        MainWindow *mw = new MainWindow(userId, role);
        mw->show();
        // Закрытие и удаление окна входа
        login->close();
    });

    // Закрытие текущего главного окна
    this->close();
}
