#include "mainwindow.h"

#include <QApplication>
#include "openingloader.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();

    OpeningLoader::loadOpenings("assets/openings.txt");
    std::string randomFen = OpeningLoader::getRandomFen();
    std::cout << randomFen << std::endl;

    return a.exec();
}
