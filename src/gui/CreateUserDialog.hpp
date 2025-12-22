#pragma once
#include <QDialog>

class QLineEdit;
class QComboBox;
class QLabel;

class CreateUserDialog : public QDialog {
    Q_OBJECT
public:
    explicit CreateUserDialog(QWidget *parent = nullptr);

    QString login() const;
    QString password() const;
    QString role() const;
    QString fullName() const;
    QString email() const;

private slots:
    void onOk();

private:
    QLineEdit *editLogin;
    QLineEdit *editPassword;
    QLineEdit *editPasswordRepeat;
    QLineEdit *editFullName;
    QLineEdit *editEmail;
    QComboBox *comboRole;
    QLabel *lblError;
};
