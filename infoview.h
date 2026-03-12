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

protected:
    void resizeEvent(QResizeEvent* event) override;

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

    QWidget* resultBox;
    QLabel* whiteKing;
    QLabel* blackKing;
    QLabel* scoreLabel;
    QLabel* statusLabel;

    GameResult gameRes = GameResult::GAME_DID_NOT_END;

    QGridLayout* movesLayout;
    int currentRow = 0;
    int buttonAmount = 0;

    QPushButton* btnSettings;
    QPushButton* btnNewGame;
    QPushButton* btnUndo;
    QPushButton* btnGiveUp;

    QString numStyle = "color: #cccccc; font-size: 14px; font-weight: bold;";

    void updateResultDisplay();
};

#endif
