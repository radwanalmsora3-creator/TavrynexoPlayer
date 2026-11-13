#include "ui/IconFactory.hpp"
#include "ui/MainWindow.hpp"
#include <QApplication>
#include <clocale>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    std::setlocale(LC_NUMERIC, "C");

    app.setOrganizationName("Tavrynexo");
    app.setApplicationName("TavrynexoPlayer");
    app.setApplicationVersion("2.0.0");
    app.setWindowIcon(Tavrynexo::UI::IconFactory::appIcon());

    QString initialFile;
    if (argc > 1) {
        initialFile = QString::fromLocal8Bit(argv[1]);
    }

    Tavrynexo::UI::MainWindow w(initialFile);
    w.show();
    return app.exec();
}
