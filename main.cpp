#include "gui_can.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Инициализация главного окна
    GUI_CAN mainWindow;

    // Настройки окна
    mainWindow.setWindowTitle("CAN Interface Controller");
    mainWindow.resize(800, 600);  // Оптимальный размер для двух таблиц
    mainWindow.show();

    return app.exec();
}
