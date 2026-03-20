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

    setMinimumSize(450, 350);
    resize(450, 350);

    setStyleSheet(R"(
    QDialog { background-color: #312E2B; color: #eee; font-family: 'Segoe UI', sans-serif; }

    QTabWidget::pane { border: 1px solid #555; background: #312E2B; }
    QTabBar::tab { padding: 10px 20px; font-size: 14px; background: #262422; color: #888; }
    QTabBar::tab:selected { background: #312E2B; color: white; border-top: 2px solid #B48866; }

    QLabel {
        color: #eee;
        font-size: 15px;
        padding: 2px;
    }

    QLineEdit {
        background-color: #444;
        color: white;
        padding: 6px;
        border: 1px solid #666;
        border-radius: 4px;
        font-size: 14px;
        selection-background-color: #B48866;
        selection-color: white;
    }
    QLineEdit:focus {
        border: 1px solid #B48866;
    }
    QLineEdit:disabled {
        color: #777;
        background-color: #2a2a2a;
        border: 1px solid #444;
    }

    QCheckBox { font-size: 15px; spacing: 10px; color: #eee; }
    QCheckBox:disabled { color: #777; }
    QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #888; border-radius: 3px; background: #444; }
    QCheckBox::indicator:checked { background-color: #B48866; border: 1px solid #B48866; }
    QCheckBox::indicator:unchecked:hover { border: 1px solid #aaa; }

    QComboBox {
        background-color: #444; color: white; padding: 6px;
        border: 1px solid #666; border-radius: 4px; font-size: 14px;
    }
    QComboBox:disabled { color: #777; background-color: #2a2a2a; }
    QComboBox::drop-down { border: none; }
    QComboBox::down-arrow { image: none; border-left: 1px solid #555; width: 0px; }
    QComboBox QAbstractItemView {
        background-color: #444; color: white;
        selection-background-color: #B48866; selection-color: white;
        border: 1px solid #555;
    }

    QPushButton { background-color: #555; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-size: 14px; }
    QPushButton:hover { background-color: #666; }
    QPushButton:pressed { background-color: #444; }
    QPushButton[text="Mentés"] { background-color: #B48866; font-weight: bold; }
    QPushButton[text="Mentés"]:hover { background-color: #a37855; }
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

        RobotSettings& rs = allS.robotSettings;
        BoardSettings& bs = allS.boardSettings;

        rs.isWhiteRobot = checkWhiteRobot->isChecked();
        rs.whiteRobotDifficulty = (Difficulty)comboWhiteDiff->currentIndex();

        rs.isBlackRobot = checkBlackRobot->isChecked();
        rs.blackRobotDifficulty = (Difficulty)comboBlackDiff->currentIndex();

        rs.isBotVsBot = checkBotVsBot->isChecked();
        rs.botVsBotSearchTimeMs = lineSearchTime->text().toInt();

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

    QStringList levels = {"Kezdő", "Haladó", "Nehéz", "Mester"};

    RobotSettings rs = allS.robotSettings;

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

    checkBotVsBot = new QCheckBox("Robot vs Robot");
    checkBotVsBot->setChecked(rs.isBotVsBot);

    labelSearchTime = new QLabel("Gondolkodás idő");

    QString botVsBotSearchString = QString::number(rs.botVsBotSearchTimeMs);

    lineSearchTime = new QLineEdit();
    lineSearchTime->setPlaceholderText(botVsBotSearchString);
    lineSearchTime->setText(botVsBotSearchString);
    lineSearchTime->setEnabled(rs.isBotVsBot);

    connect(checkWhiteRobot, &QCheckBox::toggled, comboWhiteDiff, &QWidget::setEnabled);
    connect(checkBlackRobot, &QCheckBox::toggled, comboBlackDiff, &QWidget::setEnabled);

    connect(checkBotVsBot, &QCheckBox::toggled, this, [=](bool checked) {
        lineSearchTime->setEnabled(checked);

        if (checked) {

            checkWhiteRobot->setChecked(true);
            checkBlackRobot->setChecked(true);

            checkWhiteRobot->setEnabled(false);
            checkBlackRobot->setEnabled(false);
            comboWhiteDiff->setEnabled(false);
            comboBlackDiff->setEnabled(false);
        } else {
            checkWhiteRobot->setEnabled(true);
            checkBlackRobot->setEnabled(true);
            checkWhiteRobot->setChecked(false);
            checkBlackRobot->setChecked(false);
            comboWhiteDiff->setEnabled(checkWhiteRobot->isChecked());
            comboBlackDiff->setEnabled(checkBlackRobot->isChecked());
        }
    });

    if (rs.isBotVsBot) {
        checkWhiteRobot->setChecked(true);
        checkBlackRobot->setChecked(true);
        checkWhiteRobot->setEnabled(false);
        checkBlackRobot->setEnabled(false);
        comboWhiteDiff->setEnabled(false);
        comboBlackDiff->setEnabled(false);
    }

    layout->addWidget(checkBlackRobot, 0, 0);
    layout->addWidget(comboBlackDiff, 0, 1);
    layout->addWidget(checkWhiteRobot, 1, 0);
    layout->addWidget(comboWhiteDiff, 1, 1);
    layout->setRowMinimumHeight(2, 30);
    layout->addWidget(checkBotVsBot, 3, 0);
    layout->addWidget(labelSearchTime, 4, 0);
    layout->addWidget(lineSearchTime, 4, 1);

    layout->setRowMinimumHeight(0, 35);
    layout->setRowStretch(5, 1);

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
