#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "Settings.h"

#include <QDialog>
#include <QSlider>
#include <QCheckBox>
#include <QTabWidget>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AllSettings& allS, QWidget *parent = nullptr);

    RobotSettings getRobotSettings() const;

private:
    QWidget* createRobotTab();
    QWidget* createBoardTab();
    QWidget* creatBotVsBot();

    QTabWidget* tabWidget;

    QCheckBox* checkWhiteRobot;
    QComboBox* comboWhiteDiff;
    QCheckBox* checkBlackRobot;
    QComboBox* comboBlackDiff;
    QCheckBox* checkBotVsBot;

    QLabel* labelSearchTime;
    QLineEdit *lineSearchTime;

    QCheckBox* checkBoardFlipped;

    AllSettings& allS;
};

#endif // SETTINGSDIALOG_H
