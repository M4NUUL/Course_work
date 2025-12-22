#include "MainWindow.hpp"

#include "LoginWindow.hpp"
#include "TeacherWindow.hpp"
#include "StudentWindow.hpp"
#include "AdminWindow.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>

MainWindow::MainWindow(int userId, const QString &role, QWidget *parent)
    : QMainWindow(parent), m_userId(userId), m_role(role)
{
    setupUi();
}

void MainWindow::setupUi() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    auto v = new QVBoxLayout(central);

    // Top bar
    auto topBar = new QHBoxLayout();
    topBar->addStretch();
    btnLogout = new QPushButton("Выйти", this);
    topBar->addWidget(btnLogout);
    v->addLayout(topBar);

    // Content area
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
        // role unknown - show message
        v->addWidget(new QLabel(QString("Unknown role: %1").arg(m_role), this));
    }

    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogout);
}

void MainWindow::onLogout() {
    // Create a fresh LoginWindow and close this MainWindow
    LoginWindow *login = new LoginWindow();
    login->setAttribute(Qt::WA_DeleteOnClose);
    login->show();

    this->close();
}
