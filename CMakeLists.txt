cmake_minimum_required(VERSION 3.16)

project(new-chess-in-cpp-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(Qt6_FOUND)
    qt_add_executable(new-chess-in-cpp-gui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
        chessview.h chessview.cpp
        infoview.h infoview.cpp
        chessscene.h chessscene.cpp
        chesspieceitem.h chesspieceitem.cpp
        Attacks.cpp Attacks.h
        Board.cpp Board.h
        BoardManager.cpp BoardManager.h
        BoardState.h
        MoveGenerator.cpp MoveGenerator.h
        RepetitionTable.cpp RepetitionTable.h
        UCIParsing.cpp UCIParsing.h
        Utils.h
        Zobrist.cpp Zobrist.h
        chessviewmodel.h chessviewmodel.cpp
        settingsDialog.h settingsDialog.cpp
        Settings.h
        ISearcher.h
        openingloader.h openingloader.cpp
        assets/openings.txt
        resultmanager.h resultmanager.cpp
        filemanager.h filemanager.cpp
        oldSearcher/OldSearcher.cpp oldsearcher/OldSearcher.h
        improvedSearcher/Evaluation.cpp improvedSearcher/Evaluation.h improvedSearcher/LMR.cpp improvedSearcher/LMR.h improvedSearcher/MoveOrdering.cpp improvedSearcher/MoveOrdering.h improvedSearcher/Searcher.cpp improvedSearcher/Searcher.h improvedSearcher/TranspositionTable.cpp improvedSearcher/TranspositionTable.h
        oldSearcher/OldEvaluation.cpp oldSearcher/OldEvaluation.h oldSearcher/OldLMR.cpp oldSearcher/OldLMR.h oldSearcher/OldMoveOrdering.cpp oldSearcher/OldMoveOrdering.h oldSearcher/OldTranspositionTable.cpp oldSearcher/OldTranspositionTable.h
        Move.h MoveList.h
        oldSearcher/OldPrecomputedEvaluationData.cpp oldSearcher/OldPrecomputedEvaluationData.h
        improvedSearcher/PrecomputedEvaluationData.cpp improvedSearcher/PrecomputedEvaluationData.h
    )
endif()

if(MINGW)
    target_compile_options(new-chess-in-cpp-gui PRIVATE -g)
endif()

target_link_libraries(new-chess-in-cpp-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent)

set_target_properties(new-chess-in-cpp-gui PROPERTIES
    WIN32_EXECUTABLE FALSE
)

target_include_directories(new-chess-in-cpp-gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/oldSearcher
    ${CMAKE_CURRENT_SOURCE_DIR}/improvedSearcher
)

include(GNUInstallDirs)
install(TARGETS new-chess-in-cpp-gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(Qt6_FOUND)
    qt_finalize_executable(new-chess-in-cpp-gui)
endif()
