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
    void onDownloadMaterial();

private:
    int m_assignmentId;
    QLabel *lblTitle;
    QLabel *lblDue;
    QTextEdit *txtDescription;
    QTableWidget *tblMaterials;
    QPushButton *btnDownload;
    void loadAssignment();
    void loadMaterials();
};
