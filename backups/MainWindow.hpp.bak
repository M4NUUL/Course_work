#pragma once

#include <QMainWindow>

class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(int userId, const QString &role, QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onLogout();

private:
    QPushButton *btnLogout = nullptr;
    int m_userId;
    QString m_role;
    void setupUi();
};
