#pragma once

#include <QDialog>

class QLabel;
class QTextEdit;
class QTableWidget;
class QPushButton;

class AssignmentDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit AssignmentDetailDialog(int assignmentId, QWidget *parent = nullptr);

private slots:
    void onFileDoubleClicked(int row, int column);

private:
    void loadDetails();
    void loadFiles();

    int m_assignmentId;
    QLabel *lblTitle;
    QTextEdit *teDescription;
    QTableWidget *tblFiles;
    QPushButton *btnClose;
};
