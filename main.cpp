#include "gui_can.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <iostream>
#include <fstream>


// Глобальные переменные для путей
QString jsonPath;

// Объявление функций
bool validateJsonFile(const QString& path); // Проверить существование указанного json-файла
bool selectJsonFiles();     // Окно выбора json-файла


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    // ВЫБРАТЬ JSON-ФАЙЛ
    bool filesSelected = false;
    while (!filesSelected) {
        filesSelected = selectJsonFiles();

        // Если пользователь отменил выбор
        if (!filesSelected) {
            int ret = QMessageBox::question(
                nullptr,
                "Подтверждение",
                "Вы не выбрали JSON-файлы. Хотите попробовать снова?",
                QMessageBox::Yes | QMessageBox::No
            );

            if (ret == QMessageBox::No) {
                return 0;
            }
        }
    }

    // Инициализация главного окна
    GUI_CAN mainWindow(jsonPath);

    // Настройки окна
    mainWindow.setWindowTitle("CAN Interface Controller");
    mainWindow.resize(800, 600);  // Оптимальный размер для двух таблиц
    mainWindow.show();

    return app.exec();
}

bool validateJsonFile(const QString& path) {
    // Проверка существования файла
    if (!QFile::exists(path)) {
        QMessageBox::warning(nullptr, "Ошибка",
            QString("Файл не существует:\n%1").arg(path));
        return false;
    }

    // Проверка, что это валидный JSON
    try {
        std::ifstream file(path.toStdString());
        nlohmann::json j;
        file >> j;
        return true;
    } catch (...) {
        QMessageBox::warning(nullptr, "Ошибка",
            QString("Некорректный JSON-файл:\n%1").arg(path));
        return false;
    }
}

// Реализации функций

bool selectJsonFiles() {
    // Показываем диалог выбора файлов
    QString userPath = QFileDialog::getOpenFileName(
        nullptr,
        "Выберите JSON-файл для ОТПРАВКИ",
        QDir::homePath(),
        "JSON Files (*.json);;All Files (*)"
    );

    if (userPath.isEmpty()) return false;

    // Проверяем первый файл
    if (!validateJsonFile(userPath)) {
        return false;
    }

    jsonPath = userPath;
    return true;
}
