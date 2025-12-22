#pragma once
#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(int userId, const QString &role, QWidget *parent=nullptr);

private:
    int m_userId;
    QString m_role;
};
