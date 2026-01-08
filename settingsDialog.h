#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "Settings.h"

#include <QDialog>
#include <QSlider>
#include <QCheckBox>
#include <QTabWidget>
#include <QComboBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(AllSettings& allS, QWidget *parent = nullptr);

    RobotSettings getRobotSettings() const;

private:
    QWidget* createRobotTab();
    QWidget* createBoardTab();

    QTabWidget* tabWidget;

    QCheckBox* checkWhiteRobot;
    QComboBox* comboWhiteDiff;

    QCheckBox* checkBlackRobot;
    QComboBox* comboBlackDiff;

    QCheckBox* checkBoardFlipped;

    AllSettings& allS;
};

#endif // SETTINGSDIALOG_H
