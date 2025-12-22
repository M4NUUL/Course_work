#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class AdminWindow : public QWidget {
    Q_OBJECT
public:
    explicit AdminWindow(int adminId, QWidget *parent = nullptr);

private slots:
    void loadUsers();
    void onCreateUser();
    void onToggleActive();
    void onEditUser();
    void onCreateDiscipline();
    void onCreateAssignment();
    void onRefresh();

private:
    int m_adminId;
    QTableWidget *tblUsers;
    QPushButton *btnCreateUser;
    QPushButton *btnEditUser;
    QPushButton *btnToggleActive;
    QPushButton *btnCreateDiscipline;
    QPushButton *btnCreateAssignment;
    QPushButton *btnRefresh;

    void showError(const QString &text);
};
