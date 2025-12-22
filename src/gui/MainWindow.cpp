#include "MainWindow.hpp"
#include "TeacherWindow.hpp"
#include "AdminWindow.hpp"
#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(int userId, const QString &role, QWidget *parent)
    : QMainWindow(parent), m_userId(userId), m_role(role) {
    setWindowTitle("Jumandgi — Главная");
    resize(900,600);
    QWidget *central = new QWidget();
    auto v = new QVBoxLayout(central);
    auto label = new QLabel(QString("Добро пожаловать, пользователь %1 (роль: %2)").arg(userId).arg(role));
    v->addWidget(label);
    setCentralWidget(central);

    if (m_role == "teacher") {
        TeacherWindow *tw = new TeacherWindow(m_userId);
        tw->setAttribute(Qt::WA_DeleteOnClose);
        tw->show();
    } else if (m_role == "admin") {
        AdminWindow *aw = new AdminWindow(m_userId);
        aw->setAttribute(Qt::WA_DeleteOnClose);
        aw->show();
    } else {
        // студентский интерфейс по желанию
    }

    // admin and student windows can be implemented similarly
}
