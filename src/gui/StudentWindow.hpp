#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;

class StudentWindow : public QWidget {
    Q_OBJECT
public:
    explicit StudentWindow(int studentId, QWidget *parent=nullptr);

private slots:
    void loadAssignments();
    void onUpload();
    void onAssignmentDoubleClicked(int row, int column);

private:
    int m_studentId;
    QTableWidget *tblAssignments;
    QPushButton *btnUpload;
};
