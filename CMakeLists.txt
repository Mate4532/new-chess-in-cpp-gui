cmake_minimum_required(VERSION 3.16)

project(new-chess-in-cpp-gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

if(MSVC)
    add_compile_options(/O2 /Oi /Ot /GL /arch:AVX2 /fp:fast)
    add_link_options(/LTCG)
    add_compile_definitions(USE_AVX2)
else()
    add_compile_options(-O3 -march=native -mtune=native -flto)
    add_link_options(-flto -O3)
    
    add_compile_definitions(USE_AVX2)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

set(PROJECT_SOURCES
        main.cpp
	mainwindow.ui
)

if(Qt6_FOUND)
    qt_add_executable(new-chess-in-cpp-gui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        assets/openings.txt Board/Attacks.cpp Board/Attacks.h Board/Board.cpp Board/Board.h Board/BoardManager.cpp Board/BoardManager.h Board/BoardState.h Board/filemanager.cpp Board/filemanager.h Board/ISearcher.h Board/Move.h Board/MoveGenerator.cpp Board/MoveGenerator.h Board/MoveList.h Board/openingloader.cpp Board/openingloader.h Board/RepetitionTable.cpp Board/RepetitionTable.h Board/resultmanager.cpp Board/resultmanager.h Board/UCIParsing.cpp Board/UCIParsing.h Board/Utils.h Board/Zobrist.cpp Board/Zobrist.h botvsbotresults/botvsbot.txt GUI/chesspieceitem.cpp GUI/chesspieceitem.h GUI/chessscene.cpp GUI/chessscene.h GUI/chessview.cpp GUI/chessview.h GUI/chessviewmodel.cpp GUI/chessviewmodel.h GUI/infoview.cpp GUI/infoview.h GUI/mainwindow.cpp GUI/mainwindow.h GUI/Settings.h GUI/settingsDialog.cpp GUI/settingsDialog.h improvedSearcher/Evaluation.cpp  improvedSearcher/Evaluation.h improvedSearcher/LMR.cpp improvedSearcher/LMR.h improvedSearcher/MoveOrdering.cpp improvedSearcher/MoveOrdering.h improvedSearcher/PrecomputedEvaluationData.cpp improvedSearcher/PrecomputedEvaluationData.h improvedSearcher/Searcher.cpp improvedSearcher/Searcher.h improvedSearcher/TranspositionTable.cpp improvedSearcher/TranspositionTable.h nnue-probe-master/src/misc.cpp nnue-probe-master/src/misc.h nnue-probe-master/src/nnue.cpp nnue-probe-master/src/nnue.h nnue-probe-master/LICENSE nnue-probe-master/README.md oldsearcher/OldEvaluation.cpp oldsearcher/OldEvaluation.h oldsearcher/OldLMR.cpp oldsearcher/OldLMR.h oldsearcher/OldMoveOrdering.cpp oldsearcher/OldMoveOrdering.h oldsearcher/OldPrecomputedEvaluationData.cpp oldsearcher/OldPrecomputedEvaluationData.h oldsearcher/oldsearcher.cpp oldsearcher/OldSearcher.h oldsearcher/OldTranspositionTable.cpp oldsearcher/OldTranspositionTable.h resources/black_bishop.png resources/black_king.png resources/black_knight.png resources/black_pawn.png resources/black_queen.png resources/black_rook.png resources/chessboard.png resources/chessboard_flipped.png resources/white_bishop.png resources/white_king.png resources/white_knight.png resources/white_pawn.png resources/white_queen.png resources/white_rook.png main.cpp
        resources.qrc
        GUI/customQItemStyles/customButtonStyles.h
        GUI/boardAndPlayerPanel.h
        GUI/boardAndPlayerPanel.cpp
        GUI/playerpanel.h GUI/playerpanel.cpp
        improvedSearcher/WorseEvaluation.cpp improvedSearcher/WorseEvaluation.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/Board
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
