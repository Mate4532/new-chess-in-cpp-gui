#include "settingsdialog.h"
#include "BoardManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

    SettingsDialog::SettingsDialog(AllSettings& allSettings, QWidget *parent)
    : allS(allSettings), QDialog(parent)
{
    setWindowTitle("Beállítások");

    if (parent && parent->window()) {
        resize(std::max(parent->window()->width() / 2, 400),
               std::max(parent->window()->height() / 2, 350));
    } else {
        resize(450, 350);
    }

    setStyleSheet(R"(
        QDialog { background-color: #312E2B; color: #eee; }
        QTabWidget::pane { border: 1px solid #555; background: #312E2B; }
        QTabBar::tab { padding: 10px 20px; font-size: 14px; background: #262422; color: #888; }
        QTabBar::tab:selected { background: #312E2B; color: white; border-top: 2px solid #B48866; }

        QCheckBox { font-size: 15px; spacing: 10px; }
        QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #888; border-radius: 3px; }
        QCheckBox::indicator:checked { background-color: #B48866; border: 1px solid #B48866; }

        QComboBox {
            background-color: #444; color: white; padding: 6px;
            border: 1px solid #666; border-radius: 4px; font-size: 14px;
        }
        QComboBox:disabled { color: #777; background-color: #2a2a2a; }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView { background-color: #444; color: white; selection-background-color: #B48866; }

        QPushButton { background-color: #555; color: white; border: none; padding: 8px 16px; border-radius: 4px; }
        QPushButton[text="Mentés"] { background-color: #B48866; }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    tabWidget = new QTabWidget();
    tabWidget->addTab(createRobotTab(), "Robotok");
    tabWidget->addTab(createBoardTab(), "Tábla");

    mainLayout->addWidget(tabWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton* btnCancel = new QPushButton("Mégse");
    QPushButton* btnSave = new QPushButton("Mentés");
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    mainLayout->addLayout(btnLayout);

    connect(btnSave, &QPushButton::clicked, this, [this]() {

        allS.robotSettings.isWhiteRobot = checkWhiteRobot->isChecked();
        allS.robotSettings.whiteRobotDifficulty = (Difficulty)comboWhiteDiff->currentIndex();

        allS.robotSettings.isBlackRobot = checkBlackRobot->isChecked();
        allS.robotSettings.blackRobotDifficulty = (Difficulty)comboBlackDiff->currentIndex();

        allS.boardSettings.isBoardFlipped = checkBoardFlipped->isChecked();

        accept();
    });
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

QWidget* SettingsDialog::createRobotTab() {
    QWidget* tab = new QWidget();

    QGridLayout* layout = new QGridLayout(tab);
    layout->setSpacing(10);
    layout->setContentsMargins(30, 30, 30, 30);

    QStringList levels = {"Kezdő (Easy)", "Haladó (Medium)", "Nehéz (Hard)", "Mester (Impossible)"};

    RobotSettings& rs = allS.robotSettings;

    checkBlackRobot = new QCheckBox("Fekete Robot");
    checkBlackRobot->setChecked(rs.isBlackRobot);

    comboBlackDiff = new QComboBox();
    comboBlackDiff->addItems(levels);
    comboBlackDiff->setCurrentIndex((int)rs.blackRobotDifficulty);
    comboBlackDiff->setEnabled(rs.isBlackRobot);

    checkWhiteRobot = new QCheckBox("Fehér Robot");
    checkWhiteRobot->setChecked(rs.isWhiteRobot);

    comboWhiteDiff = new QComboBox();
    comboWhiteDiff->addItems(levels);
    comboWhiteDiff->setCurrentIndex((int)rs.whiteRobotDifficulty);
    comboWhiteDiff->setEnabled(rs.isWhiteRobot);

    connect(checkWhiteRobot, &QCheckBox::toggled, comboWhiteDiff, &QWidget::setEnabled);

    layout->addWidget(checkBlackRobot, 0, 0);
    layout->addWidget(comboBlackDiff, 0, 1);

    connect(checkBlackRobot, &QCheckBox::toggled, comboBlackDiff, &QWidget::setEnabled);

    layout->addWidget(checkWhiteRobot, 1, 0);
    layout->addWidget(comboWhiteDiff, 1, 1);

    layout->setRowMinimumHeight(0, 35);
    layout->setRowStretch(2, 1);

    return tab;
}

QWidget* SettingsDialog::createBoardTab() {
    QWidget* tab = new QWidget();

    QGridLayout* layout = new QGridLayout(tab);
    layout->setSpacing(10);
    layout->setContentsMargins(30, 30, 30, 30);

    BoardSettings& bs = allS.boardSettings;

    checkBoardFlipped = new QCheckBox("Sakktábla megfordítása");
    checkBoardFlipped->setChecked(bs.isBoardFlipped);

    layout->addWidget(checkBoardFlipped);
    layout->setRowMinimumHeight(0, 35);
    layout->setRowStretch(1, 1);

    return tab;
}
