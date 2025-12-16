#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;
class QFrame;
class QResizeEvent;
class ToggleSwitch; // forward

class LoginWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onMenuButtonClicked();
    void onThemeToggled(bool checked);
    void onSupportClicked();

private:
    void buildUi();
    void createSideMenu();
    void applyTheme(bool dark);

    QLabel* m_titleLabel = nullptr;
    QLabel* m_loginLabel = nullptr;
    QLabel* m_passwordLabel = nullptr;

    QLineEdit* m_loginEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;

    QPushButton* m_authButton = nullptr;
    QLabel* m_forgotPasswordLabel = nullptr;

    QToolButton* m_menuButton = nullptr; // “три палочки”

    // Side menu
    QFrame* m_sideMenu = nullptr;
    ToggleSwitch* m_themeToggle = nullptr;
    QPushButton* m_supportButton = nullptr;
    QLabel* m_supportEmailLabel = nullptr;

    bool m_darkTheme = false;
};
