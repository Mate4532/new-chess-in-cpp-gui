#ifndef INFOVIEW_H
#define INFOVIEW_H

#include "settingsDialog.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QScrollArea>

class InfoView : public QWidget
{
    Q_OBJECT

public:
    static constexpr int MIN_WIDTH = 250;
    explicit InfoView(AllSettings& allS, QWidget *parent = nullptr);

    QPushButton* getLastButton(int* r = nullptr, int* c = nullptr);
    void clearRowWidgets(int row);
    void addMoveToDisplay(int movePly, const QString& move, Color color, PieceType movedPiece);
    void removeLastButFromDisplay();
    void writeGameResultToDisplay(GameResult gr);
    void clearMoveDisplay();
    void updateInfoPanel();

    QFont resizeFontSize(QFont f);
    int getCurrentFontMinWidth();
    int getCurrentIconSize();
signals:
    void newGameRequested();
    void undoRequested(int amount = 1);
    void giveUpRequested();
    void reviewRequested(int targetPly);
    void settingsChanged(AllSettings& oldS, AllSettings& newS);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void handleNavigation(int id);
    void openSettings();

private:
    void setupGeneralSettings();
    void setupHeader(QVBoxLayout* layout);
    void setupMoveListArea(QVBoxLayout* layout);
    void setupResultArea(QVBoxLayout* layout);
    void setupNavigationArea(QVBoxLayout* layout);
    void setupActionButtons(QVBoxLayout* layout);

    QPushButton* createMoveButton(const QString& move, int movePly, Color Piececolor, PieceType movedPiece);

    AllSettings& currentAllS;
    int panelCurrentWidth;
    int currentReviewPly = 0;
    int currentRow = 0;
    int buttonAmount = 0;
    GameResult gameRes = GameResult::GAME_DID_NOT_END;

    QScrollArea* moveScrollArea;
    void scrollToMove(int ply);

    QGridLayout* movesLayout;
    QButtonGroup* moveButtonGroup;
    QButtonGroup* navButtonGroup;

    QPushButton* btnSettings;
    QPushButton* btnNewGame;
    QPushButton* btnUndo;
    QPushButton* btnGiveUp;

    QPushButton* btnFirst;
    QPushButton* btnPrev;
    QPushButton* btnNext;
    QPushButton* btnLast;

    QWidget* resultBox;
    QLabel* whiteKing;
    QLabel* blackKing;
    QLabel* scoreLabel;
    QLabel* statusLabel;

    QString numStyle = "color: #cccccc; padding: 10px; font-weight: bold;";

    QString moveBtnStyle = R"(
    QPushButton {
        background-color: transparent;
        color: #cccccc;
        font-weight: bold;
        border: none;
        border-radius: 4px;
        padding: 2px 5px;
    }
    QPushButton:hover { background-color: #4f4b47; color: #ffffff; }
    QPushButton:pressed { background-color: #262421; color: white; }
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
