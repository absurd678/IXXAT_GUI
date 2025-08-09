#ifndef GUI_CAN_H
#define GUI_CAN_H

#define GUI_CAN_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QThread>
#include "interfaceCAN.h"

// Класс окна с таблицей параметров
class GUI_CAN : public QMainWindow
{
    Q_OBJECT

public:
    GUI_CAN(QString jsonPath, QWidget *parent = nullptr);
    ~GUI_CAN();

private slots:
    void onStartClicked();  // Обработчик кнопки "Старт"
    void onStopClicked();   // Обработчик кнопки "Стоп"
    void updateReceivedTable(); // Обновить таблицу

private:
    void initTable(QTableWidget *table, const std::vector<frameParam> &data,
                   std::string can_name);   // Заполнить таблицу

    // Путь к json-файлу с параметрами
    QString jsonPath;

    // GUI элементы
    QTableWidget *tableSend;    // Таблица с параметрами из json файла
    QTableWidget *tableReceive; // Таблица с полученными параметрами
    QPushButton *startButton;   // Кнопка старт
    QPushButton *stopButton;    // Кнопка стоп

    // CAN объекты
    SenderToCAN writeCAN;   // Объект писателя для отправки json параметров
    ReceiverFromCAN readCAN;    // Объект читателя для получения CAN пакетов

    // Потоки
    std::thread sendThread; // Поток записи
    std::thread receiveThread;  // Поток чтения
    QThread updateThread;

    // Таймер для обновления таблицы
    QTimer *updateTimer;
};


#endif // GUI_CAN_H
