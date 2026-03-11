#ifndef INFOVIEW_H
#define INFOVIEW_H

#include "settingsDialog.h"

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>

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
    void reviewRequested(int targetPly);

    void settingsChanged(AllSettings& oldS, AllSettings& newS);

public slots:
    void addMoveToDisplay(int moveNumber, int movePly, const QString& move, Color color);
    void removeLastButFromDisplay();
    void writeGameResultToDisplay(GameResult gr);
    void clearMoveDisplay();

private:
    AllSettings& currentAllS;
    void openSettings();

    QGridLayout* movesLayout;
    int currentRow = 0;
    int buttonAmount = 0;

    QPushButton* btnSettings;
    QPushButton* btnNewGame;
    QPushButton* btnUndo;
    QPushButton* btnGiveUp;

    QString numStyle = "color: #cccccc; font-size: 14px; font-weight: bold;";
};

#endif
