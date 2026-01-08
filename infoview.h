#ifndef INFOVIEW_H
#define INFOVIEW_H

#include "settingsDialog.h"

#include <QWidget>
#include <QPushButton>

class InfoView : public QWidget
{
    Q_OBJECT
public:
    static constexpr int MIN_WIDTH = 250;
    explicit InfoView(AllSettings& allS, QWidget *parent = nullptr);

signals:
    void newGameRequested();
    void undoRequested();
    void giveUpRequested();

    void settingsChanged(AllSettings& oldS, AllSettings& newS);

private:
    AllSettings& currentAllS;
    void openSettings();

    QPushButton* btnSettings;
    QPushButton* btnNewGame;
    QPushButton* btnUndo;
    QPushButton* btnGiveUp;

    int currentDifficulty = 3;
    bool soundEnabled = true;
};

#endif
