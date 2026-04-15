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
#include <QTextEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AllSettings& allSettings, QWidget *parent = nullptr);

private slots:
    void onSaveClicked();

private:
    void applyStyles();
    void setupActionButtons(QVBoxLayout* layout);
    void setupRobotSignals();
    void setBotVsBotUI(bool active);
    void updateSearchTimeEnable();

    QWidget* createRobotTab();
    QWidget* createBoardTab();
    QWidget* createTimeControlTab();

    AllSettings& allS;

    QTabWidget* tabWidget;

    QCheckBox* checkWhiteRobot;
    QComboBox* comboWhiteDiff;
    QCheckBox* checkBlackRobot;
    QComboBox* comboBlackDiff;
    QCheckBox* checkBotVsBot;
    QLabel* labelSearchTime;
    QLineEdit *lineSearchTime;
    QLabel* labelSearchTimeWarning;

    QLabel* beginnerPosLabel;
    QTextEdit* beginnerPosFENTextEdit;
    QPushButton* setBeginnerFenBtn;
    QCheckBox* checkBoardFlipped;

    QComboBox* comboTimeMode = nullptr;
    QWidget* widgetTournamentTime;
    QLabel* labelTourTime;
    QLineEdit* lineTourTimeMin;
    QLabel* labelIncrement;
    QLineEdit* lineIncrementSec;
    void updateTimeUI();
};

#endif // SETTINGSDIALOG_H
