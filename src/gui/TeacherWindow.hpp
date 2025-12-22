#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>

class TeacherWindow : public QWidget {
    Q_OBJECT

public:
    explicit TeacherWindow(int teacherId, QWidget *parent = nullptr);

private slots:
    void loadAssignments();
    void onAssignmentSelected(int row, int col);
    void loadSubmissions(int assignmentId);
    void onDownloadSubmission();
    void onGradeSubmission();
    void onCreateAssignment();
    void onDeleteAssignment();

private:
    int m_teacherId;
    QTableWidget *tblAssignments;
    QTableWidget *tblSubmissions;
    QPushButton *btnRefresh;
    QPushButton *btnDownload;
    QPushButton *btnGrade;
    QPushButton *btnCreateAssignment;
    QPushButton *btnDeleteAssignment;
    int currentAssignmentId = -1;

    void clearSubmissions();
};
