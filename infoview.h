#ifndef INFOVIEW_H
#define INFOVIEW_H

#include <QWidget>
#include <QPushButton>

class InfoView : public QWidget
{
    Q_OBJECT
public:
    static constexpr int MIN_WIDTH = 200;
    explicit InfoView(QWidget *parent = nullptr);

signals:
    void newGameRequested();
    void undoRequested();
    void giveUp();

private:
    QPushButton* btnNewGame;
    QPushButton* btnUndo;
    QPushButton* btnGiveUp;
};

#endif
