#include <QApplication>

#include "gui/main_window.hpp"


// Run qt gui application for rrt analyzer
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}
