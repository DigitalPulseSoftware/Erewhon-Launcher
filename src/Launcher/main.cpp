#include <Launcher/Widgets/MainWindow.hpp>
#include <QtWidgets/QApplication>
#include <cassert>
#include <iostream>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow mainWindow;
	mainWindow.show();

	return app.exec();
}
