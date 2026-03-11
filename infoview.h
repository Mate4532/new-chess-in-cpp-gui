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

    void settingsChanged(AllSettings& oldS, AllSettings& newS);

public slots:
    void addMoveToDisplay(int moveNumber, const QString& move, Color color);
    void removeLastButFromDisplay();
    void clearMoveDisplay();

private:
    AllSettings& currentAllS;
    void openSettings();

    QGridLayout* movesLayout;
    int currentRow = 1;
    int buttonAmount = 0;

    QPushButton* btnSettings;
    QPushButton* btnNewGame;
    QPushButton* btnUndo;
    QPushButton* btnGiveUp;
};

#endif
