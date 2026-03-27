#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "settings.h"

#include <QDialog>
#include <QCheckBox>
#include <QTabWidget>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AllSettings& allSettings, QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    // UI építő és segéd metódusok
    void applyStyles();
    void setupActionButtons(QVBoxLayout* layout);
    void setupRobotSignals();
    void setBotVsBotUI(bool active);
    void updateSearchTimeEnable();

    QWidget* createRobotTab();
    QWidget* createBoardTab();

    // Adat referencia
    AllSettings& allS;

    // UI elemek
    QTabWidget* tabWidget;

    // Robot Tab elemek
    QCheckBox* checkWhiteRobot;
    QComboBox* comboWhiteDiff;
    QCheckBox* checkBlackRobot;
    QComboBox* comboBlackDiff;
    QCheckBox* checkBotVsBot;

    QLabel* labelSearchTime;
    QLineEdit *lineSearchTime;

    // Tábla Tab elemek
    QCheckBox* checkBoardFlipped;
};

#endif // SETTINGSDIALOG_H
