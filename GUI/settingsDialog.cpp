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
    setMinimumSize(525, 400);
    resize(525, 400);

    applyStyles();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    tabWidget = new QTabWidget();
    tabWidget->addTab(createRobotTab(), "Robotok");
    tabWidget->addTab(createBoardTab(), "Tábla");
    tabWidget->addTab(createTimeControlTab(), "Időkontroll");
    mainLayout->addWidget(tabWidget);

    setupActionButtons(mainLayout);
}

void SettingsDialog::applyStyles() {
    setStyleSheet(R"(
        QDialog { background-color: #312E2B; color: #eee; font-family: 'Segoe UI', sans-serif; }

        QTabWidget::pane {
            border: 1px solid #444;
            background-color: #312E2B;
            top: -1px;
            border-radius: 2px;
        }
        QTabWidget::tab-bar { left: 5px; }

        QTabBar::tab {
            background: #262422;
            color: #888;
            font-size: 14px;
            padding: 10px 20px;

            border: 1px solid #444;
            border-bottom: none;

            border-top-left-radius: 6px;
            border-top-right-radius: 6px;

            margin-right: 4px;
            min-width: 80px;
        }

        QTabBar::tab:selected {
            background: #312E2B;
            color: white;
            border: 1px solid #B48866;
            border-bottom: 1px solid #312E2B;
            border-top: 3px solid #B48866;
        }

        QTabBar::tab:hover:!selected {
            background: #383531;
            border: 1px solid #666;
            border-bottom: none;
        }

        QTabBar::tab:disabled {
            background: #222;
            color: #555;
            border: 1px solid #333;
        }

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
    TimeSettings& ts = allS.timeSettings;

    rs.isWhiteRobot = checkWhiteRobot->isChecked();
    rs.whiteRobotDifficulty = (Difficulty)comboWhiteDiff->currentIndex();
    rs.isBlackRobot = checkBlackRobot->isChecked();
    rs.blackRobotDifficulty = (Difficulty)comboBlackDiff->currentIndex();
    rs.isBotVsBot = checkBotVsBot->isChecked();
    rs.botSearchTimeMs = std::max(10, lineSearchTime->text().toInt());

    bs.isBoardFlipped = checkBoardFlipped->isChecked();
    bs.showLegalMoves = checkShowLegalMoves->isChecked();
    bs.beginnerPosFEN = beginnerPosFENTextEdit->toPlainText().toStdString();

    ts.gm = comboTimeMode->currentIndex() == 0 ? GameMode::UNLIMITED_THINKING_TIME : GameMode::TOURNAMENT_MODE;
    ts.rtum = comboTimeMode->currentIndex() == 0 ? RobotTimeUsageMode::FIXED_TIME : RobotTimeUsageMode::TOURNEMENT_TIME;

    int totalSeconds = (spinTourTimeMin->value() * 60) + spinTourTimeSec->value();
    ts.tournamentTimeSec = std::max(1, totalSeconds);
    ts.incrementSec = spinIncrementSec->value();

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

    labelSearchTimeWarning = new QLabel("<i>Torna mód miatt letiltva.</i>");
    labelSearchTimeWarning->setStyleSheet("color: #B48866; font-size: 13px;");
    labelSearchTimeWarning->setVisible(false);

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

    QVBoxLayout* timeVLayout = new QVBoxLayout();
    timeVLayout->setSpacing(2);

    QHBoxLayout* timeContainer = new QHBoxLayout();
    timeContainer->addWidget(labelSearchTime);
    timeContainer->addWidget(lineSearchTime);
    timeContainer->addStretch();

    timeVLayout->addLayout(timeContainer);
    timeVLayout->addWidget(labelSearchTimeWarning);

    layout->addLayout(timeVLayout, 4, 0, 1, 2);
    layout->setRowMinimumHeight(4, 50);

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

    bool isFixedTime = true;
    if (comboTimeMode) {
        isFixedTime = (comboTimeMode->currentIndex() == 0);
    }

    labelSearchTime->setEnabled(anyBot && isFixedTime);
    lineSearchTime->setEnabled(anyBot && isFixedTime);

    if (anyBot && !isFixedTime) {
        labelSearchTimeWarning->setVisible(true);
    } else {
        labelSearchTimeWarning->setVisible(false);
    }
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

    QString lineStyle = "background-color: #262421; margin-top: 10px; margin-bottom: 10px;";

    checkBoardFlipped = new QCheckBox("Sakktábla megfordítása");
    checkBoardFlipped->setChecked(allS.boardSettings.isBoardFlipped);

    checkShowLegalMoves = new QCheckBox("Legális lépések megjelenítése");
    checkShowLegalMoves->setChecked(allS.boardSettings.showLegalMoves);

    layout->addWidget(checkBoardFlipped, 0, 0);
    layout->addWidget(checkShowLegalMoves, 1, 0);
    layout->setRowMinimumHeight(0, 35);
    layout->setRowMinimumHeight(1, 35);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::NoFrame);
    line->setFixedHeight(2);
    line->setStyleSheet(lineStyle);
    layout->addWidget(line, 2, 0, 1, 2);
    layout->setRowMinimumHeight(2, 25);

    beginnerPosLabel = new QLabel("Kezdő pozíció (FEN):");
    beginnerPosFENTextEdit = new QTextEdit(QString::fromStdString(allS.boardSettings.beginnerPosFEN));

    beginnerPosFENTextEdit->setAcceptRichText(false);
    beginnerPosFENTextEdit->setLineWrapMode(QTextEdit::WidgetWidth);
    beginnerPosFENTextEdit->setFixedHeight(70);
    beginnerPosFENTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    beginnerPosFENTextEdit->setStyleSheet(R"(
        QTextEdit {
            background-color: #444;
            color: white;
            padding: 8px;
            border: 1px solid #666;
            border-radius: 4px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 15px;
        }
        QTextEdit:focus { border: 1px solid #B48866; }
    )");

    layout->addWidget(beginnerPosLabel, 3, 0);
    layout->setRowMinimumHeight(3, 40);

    layout->addWidget(beginnerPosFENTextEdit, 4, 0, 1, 2);
    layout->setRowMinimumHeight(4, 80);

    setBeginnerFenBtn = new QPushButton("Kezdő pozíció visszaállítása");
    setBeginnerFenBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(setBeginnerFenBtn, 5, 0, 1, 2);
    layout->setRowMinimumHeight(5, 45);

    connect(setBeginnerFenBtn, &QPushButton::clicked, this, [this](){
        beginnerPosFENTextEdit->setPlainText(QString::fromStdString(newPosFen));
    });

    layout->setRowStretch(6, 1);

    return tab;
}

QWidget* SettingsDialog::createTimeControlTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(tab);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 20, 30, 20);

    TimeSettings& ts = allS.timeSettings;

    int labelWidth = 193;

    QHBoxLayout* modeLayout = new QHBoxLayout();
    QLabel* labelMode = new QLabel("Játékmód:");
    labelMode->setFixedWidth(labelWidth);

    comboTimeMode = new QComboBox();
    comboTimeMode->addItems({"Óra nélküli", "Torna mód"});
    comboTimeMode->setCurrentIndex(ts.rtum == RobotTimeUsageMode::FIXED_TIME ? 0 : 1);
    comboTimeMode->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    modeLayout->addWidget(labelMode);
    modeLayout->addWidget(comboTimeMode);
    modeLayout->setAlignment(Qt::AlignLeft);
    mainLayout->addLayout(modeLayout);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::NoFrame);
    line->setFixedHeight(2);
    line->setStyleSheet("background-color: #262421; margin-top: 5px; margin-bottom: 5px;");
    mainLayout->addWidget(line);

    widgetTournamentTime = new QWidget();
    QGridLayout* tourLayout = new QGridLayout(widgetTournamentTime);
    tourLayout->setContentsMargins(0, 0, 0, 0);
    tourLayout->setSpacing(10);

    int currentMin = ts.tournamentTimeSec / 60;
    int currentSec = ts.tournamentTimeSec % 60;

    QLabel* labelTourTimeTitle = new QLabel("Alapidő:");

    QFont bigFont;
    bigFont.setPointSize(12);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Base, QColor("#444444"));
    darkPalette.setColor(QPalette::Text, Qt::white);

    spinTourTimeMin = new QSpinBox();
    spinTourTimeMin->setRange(0, 180);
    spinTourTimeMin->setValue(currentMin);
    spinTourTimeMin->setFixedWidth(75);
    spinTourTimeMin->setFixedHeight(35);
    spinTourTimeMin->setFont(bigFont);
    spinTourTimeMin->setPalette(darkPalette);
    QLabel* labelUnitMin = new QLabel("p");

    spinTourTimeSec = new QSpinBox();
    spinTourTimeSec->setRange(0, 59);
    spinTourTimeSec->setValue(currentSec);
    spinTourTimeSec->setFixedWidth(75);
    spinTourTimeSec->setFixedHeight(35);
    spinTourTimeSec->setFont(bigFont);
    spinTourTimeSec->setPalette(darkPalette);
    QLabel* labelUnitSec = new QLabel("mp");

    QHBoxLayout* baseTimeLayout = new QHBoxLayout();
    baseTimeLayout->setSpacing(0);
    baseTimeLayout->addWidget(spinTourTimeMin);
    baseTimeLayout->addSpacing(4);
    baseTimeLayout->addWidget(labelUnitMin);
    baseTimeLayout->addSpacing(20);
    baseTimeLayout->addWidget(spinTourTimeSec);
    baseTimeLayout->addSpacing(4);
    baseTimeLayout->addWidget(labelUnitSec);
    baseTimeLayout->addStretch();

    labelIncrement = new QLabel("Bónuszidő lépésenként:");

    spinIncrementSec = new QDoubleSpinBox();
    spinIncrementSec->setRange(0.0, 60.0);
    spinIncrementSec->setDecimals(1);
    spinIncrementSec->setSingleStep(0.1);
    spinIncrementSec->setValue(ts.incrementSec);
    spinIncrementSec->setFixedWidth(75);
    spinIncrementSec->setFixedHeight(35);
    spinIncrementSec->setFont(bigFont);
    spinIncrementSec->setPalette(darkPalette);
    QLabel* labelUnitInc = new QLabel("mp");

    QHBoxLayout* incrementLayout = new QHBoxLayout();
    incrementLayout->setSpacing(0);
    incrementLayout->addWidget(spinIncrementSec);
    incrementLayout->addSpacing(4);
    incrementLayout->addWidget(labelUnitInc);
    incrementLayout->addStretch();

    tourLayout->addWidget(labelTourTimeTitle, 0, 0);
    tourLayout->addLayout(baseTimeLayout, 0, 1);

    tourLayout->addWidget(labelIncrement, 1, 0);
    tourLayout->addLayout(incrementLayout, 1, 1);

    mainLayout->addWidget(widgetTournamentTime);
    mainLayout->addStretch();

    connect(comboTimeMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::updateTimeUI);
    updateTimeUI();

    return tab;
}

void SettingsDialog::updateTimeUI() {
    bool isTournement = (comboTimeMode->currentIndex() == 1);
    widgetTournamentTime->setVisible(isTournement);

    updateSearchTimeEnable();
}
