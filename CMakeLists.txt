cmake_minimum_required(VERSION 3.16)

project(new-chess-in-cpp-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

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
        Evaluation.cpp Evaluation.h
        LMR.cpp LMR.h
        Move.h
        MoveGenerator.cpp MoveGenerator.h
        MoveList.h
        MoveOrdering.cpp MoveOrdering.h
        PrecomputedEvaluationData.cpp PrecomputedEvaluationData.h
        RepetitionTable.cpp RepetitionTable.h
        Searcher.cpp Searcher.h
        TranspositionTable.cpp TranspositionTable.h
        UCIParsing.cpp UCIParsing.h
        Utils.h
        Zobrist.cpp Zobrist.h
        chessviewmodel.h chessviewmodel.cpp
        settingsDialog.h settingsDialog.cpp
        Settings.h
    )
endif()

target_link_libraries(new-chess-in-cpp-gui PRIVATE Qt6::Widgets)

set_target_properties(new-chess-in-cpp-gui PROPERTIES
    WIN32_EXECUTABLE TRUE
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
