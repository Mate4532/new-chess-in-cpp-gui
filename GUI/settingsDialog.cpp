#include "settingsdialog.h"
#include "BoardManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QIntValidator>
#include <QFrame>

SettingsDialog::SettingsDialog(AllSettings& allSettings, QWidget *parent)
    : allS(allSettings), QDialog(parent)
{
    setWindowTitle("Beállítások");
    setMinimumSize(450, 400);
    resize(450, 400);

    applyStyles();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    tabWidget = new QTabWidget();
    tabWidget->addTab(createRobotTab(), "Robotok");
    tabWidget->addTab(createBoardTab(), "Tábla");
    mainLayout->addWidget(tabWidget);

    setupActionButtons(mainLayout);
}

void SettingsDialog::applyStyles() {
    setStyleSheet(R"(
        QDialog { background-color: #312E2B; color: #eee; font-family: 'Segoe UI', sans-serif; }

        QTabWidget::pane { border: 2px solid #262421; background-color: #312E2B; }

        QTabBar::tab { padding: 10px 20px; font-size: 14px; background: #262422; color: #888; }
        QTabBar::tab:selected { background: #312E2B; color: white; border-top: 2px solid #B48866; }

        QLabel { color: #eee; font-size: 15px; padding: 2px; }
        QLabel:disabled { color: #555; }

        QLineEdit, QTextEdit {
            background-color: #444; color: white; padding: 6px; border: 1px solid #666;
            border-radius: 4px; font-size: 14px; selection-background-color: #B48866; selection-color: white;
        }
        QLineEdit:focus { border: 1px solid #B48866; }
        QLineEdit:disabled { color: #777; background-color: #2a2a2a; border: 1px solid #444; }

        QCheckBox { font-size: 15px; spacing: 10px; color: #eee; }
        QCheckBox:disabled { color: #777; }
        QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #888; border-radius: 3px; background: #444; }
        QCheckBox::indicator:checked { background-color: #B48866; border: 1px solid #B48866; }
        QCheckBox::indicator:unchecked:hover { border: 1px solid #aaa; }

        QComboBox { background-color: #444; color: white; padding: 6px; border: 1px solid #666; border-radius: 4px; font-size: 14px; }
        QComboBox:disabled { color: #777; background-color: #2a2a2a; }
        QComboBox::drop-down { border: none; }
        QComboBox::down-arrow { image: none; border-left: 1px solid #555; width: 0px; }

        QComboBox QAbstractItemView {
            background-color: #444; color: white; selection-background-color: #B48866;
            selection-color: white; border: 1px solid #555;
        }

        QPushButton { background-color: #555; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-size: 14px; }
        QPushButton:hover { background-color: #666; }
        QPushButton:pressed { background-color: #444; }

        QPushButton[text="Mentés"] { background-color: #B48866; font-weight: bold; }
        QPushButton[text="Mentés"]:hover { background-color: #a37855; }
    )");
}

void SettingsDialog::setupActionButtons(QVBoxLayout* layout) {
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* btnCancel = new QPushButton("Mégse");
    QPushButton* btnSave = new QPushButton("Mentés");

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    connect(btnSave, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::onSaveClicked() {
    RobotSettings& rs = allS.robotSettings;
    BoardSettings& bs = allS.boardSettings;


    rs.isWhiteRobot = checkWhiteRobot->isChecked();
    rs.whiteRobotDifficulty = (Difficulty)comboWhiteDiff->currentIndex();
    rs.isBlackRobot = checkBlackRobot->isChecked();
    rs.blackRobotDifficulty = (Difficulty)comboBlackDiff->currentIndex();
    rs.isBotVsBot = checkBotVsBot->isChecked();
    rs.botSearchTimeMs = std::max(10, lineSearchTime->text().toInt());

    bs.isBoardFlipped = checkBoardFlipped->isChecked();
    bs.beginnerPosFEN = beginnerPosFENTextEdit->toPlainText().toStdString();

    accept();
}

QWidget* SettingsDialog::createRobotTab() {
    QWidget* tab = new QWidget();
    QGridLayout* layout = new QGridLayout(tab);

    layout->setSpacing(5);
    layout->setContentsMargins(30, 20, 30, 20);

    RobotSettings& rs = allS.robotSettings;
    QStringList levels = {"Kezdő", "Haladó", "Nehéz", "Mester"};

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

    checkBotVsBot = new QCheckBox("Robot vs Robot mód");
    checkBotVsBot->setChecked(rs.isBotVsBot);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::NoFrame);
    line->setFixedHeight(2);
    line->setStyleSheet("background-color: #262421; margin-top: 10px; margin-bottom: 10px;");

    labelSearchTime = new QLabel("Max gondolkodási idő (ms):");
    lineSearchTime = new QLineEdit(QString::number(rs.botSearchTimeMs));
    lineSearchTime->setValidator(new QIntValidator(10, 10000, this));
    lineSearchTime->setFixedWidth(80);

    setupRobotSignals();

    layout->addWidget(checkBlackRobot, 0, 0);
    layout->addWidget(comboBlackDiff, 0, 1);
    layout->setRowMinimumHeight(0, 40);

    layout->addWidget(checkWhiteRobot, 1, 0);
    layout->addWidget(comboWhiteDiff, 1, 1);
    layout->setRowMinimumHeight(1, 40);

    layout->addWidget(checkBotVsBot, 2, 0, 1, 2);
    layout->setRowMinimumHeight(2, 40);

    layout->addWidget(line, 3, 0, 1, 2);
    layout->setRowMinimumHeight(3, 30);

    QHBoxLayout* timeContainer = new QHBoxLayout();
    timeContainer->addWidget(labelSearchTime);
    timeContainer->addWidget(lineSearchTime);
    timeContainer->addStretch();

    layout->addLayout(timeContainer, 4, 0, 1, 2);
    layout->setRowMinimumHeight(4, 40);

    if (rs.isBotVsBot) setBotVsBotUI(true);
    updateSearchTimeEnable();

    layout->setRowStretch(5, 1);
    return tab;
}

void SettingsDialog::setupRobotSignals() {
    connect(checkWhiteRobot, &QCheckBox::toggled, this, [this](bool checked){
        comboWhiteDiff->setEnabled(checked);
        updateSearchTimeEnable();
    });
    connect(checkBlackRobot, &QCheckBox::toggled, this, [this](bool checked){
        comboBlackDiff->setEnabled(checked);
        updateSearchTimeEnable();
    });
    connect(checkBotVsBot, &QCheckBox::toggled, this, [this](bool checked) {
        setBotVsBotUI(checked);
        updateSearchTimeEnable();
    });
}

void SettingsDialog::updateSearchTimeEnable() {
    bool anyBot = checkWhiteRobot->isChecked() ||
                  checkBlackRobot->isChecked() ||
                  checkBotVsBot->isChecked();
    labelSearchTime->setEnabled(anyBot);
    lineSearchTime->setEnabled(anyBot);
}

void SettingsDialog::setBotVsBotUI(bool active) {

    bool setCheckBoxChecked = active ? true : false;
    bool setComboboxEnabled = active ? false : true;

    checkWhiteRobot->setChecked(setCheckBoxChecked);
    checkBlackRobot->setChecked(setCheckBoxChecked);
    checkWhiteRobot->setEnabled(setComboboxEnabled);
    checkBlackRobot->setEnabled(setComboboxEnabled);
    comboWhiteDiff->setEnabled(false);
    comboBlackDiff->setEnabled(false);
}

QWidget* SettingsDialog::createBoardTab() {
    QWidget* tab = new QWidget();
    QGridLayout* layout = new QGridLayout(tab);

    layout->setSpacing(5);
    layout->setContentsMargins(30, 20, 30, 20);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::NoFrame);
    line->setFixedHeight(2);
    line->setStyleSheet("background-color: #262421; margin-top: 10px; margin-bottom: 10px;");

    checkBoardFlipped = new QCheckBox("Sakktábla megfordítása");
    checkBoardFlipped->setChecked(allS.boardSettings.isBoardFlipped);

    layout->addWidget(checkBoardFlipped, 0, 0);
    layout->setRowMinimumHeight(0, 40);

    layout->addWidget(line, 1, 0, 1, 2);
    layout->setRowMinimumHeight(1, 30);

    beginnerPosLabel = new QLabel("Kezdő pozíció");
    beginnerPosFENTextEdit = new QTextEdit(QString::fromStdString(allS.boardSettings.beginnerPosFEN));

    beginnerPosFENTextEdit->setAcceptRichText(false);
    beginnerPosFENTextEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    beginnerPosFENTextEdit->setFixedHeight(70);
    beginnerPosFENTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    layout->addWidget(beginnerPosLabel, 2, 0);
    layout->setRowMinimumHeight(2, 40);

    layout->addWidget(beginnerPosFENTextEdit, 3, 0, 1, 2);
    layout->setRowMinimumHeight(3, 80);

    setBeginnerFenBtn = new QPushButton("Kezdő pozíció visszaállítása");
    layout->addWidget(setBeginnerFenBtn, 4, 0, 1, 2);
    layout->setRowMinimumHeight(4, 40);

    connect(setBeginnerFenBtn, &QPushButton::clicked, this, [this](){
        beginnerPosFENTextEdit->setPlainText(QString::fromStdString(newPosFen));
    });

    layout->setRowStretch(5, 1);
    return tab;
}
