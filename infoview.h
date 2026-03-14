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
    QFont resizeFontSize(QFont f);
    int getCurrentFontMinWidth();
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

    int panelCurrentWidth;

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

    QPushButton* createMoveButton(const QString& move, int movePly);

    void updateInfoPanel();

    QString numStyle = "color: #cccccc; padding: 10px";
    QString moveBtnStyle = R"(
        QPushButton {
            background-color: transparent;
            color: #cccccc;
            font-weight: bold;
            border: none;
            border-radius: 4px;
            padding: 2px 5px;
        }
        QPushButton:hover {
            background-color: #4f4b47;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #262421;
            color: white;
        }
    )";

    QString mainButtonStyle = R"(
        QPushButton {
            background-color: #B48866; color: white; border: none; padding: 10px;
            font-weight: bold; border-radius: 4px;
        }
        QPushButton:hover { background-color: #906C51; }
        QPushButton:pressed { background-color: #644B38; }
    )";
};

#endif
