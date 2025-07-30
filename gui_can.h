#ifndef GUI_CAN_H
#define GUI_CAN_H

#define GUI_CAN_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QThread>
#include "interfaceCAN.h"

#define PATH_TO_JSON "/home/user/dev/IXXAT_App/ПИВ_Параметры.json"

class GUI_CAN : public QMainWindow
{
    Q_OBJECT

public:
    GUI_CAN(QString jsonPath, QWidget *parent = nullptr);
    ~GUI_CAN();

private slots:
    void onStartClicked();
    void onStopClicked();
    void updateReceivedTable();

private:
    void initTable(QTableWidget *table, const std::vector<frameParam> &data,
                   std::string can_name);

    // Путь к json-файлу с параметрами
    QString jsonPath;

    // GUI элементы
    QTableWidget *tableSend;
    QTableWidget *tableReceive;
    QPushButton *startButton;
    QPushButton *stopButton;

    // CAN объекты
    SenderToCAN writeCAN;
    ReceiverFromCAN readCAN;

    // Потоки
    std::thread sendThread;
    std::thread receiveThread;
    QThread updateThread;

    // Таймер для обновления таблицы
    QTimer *updateTimer;

};


#endif // GUI_CAN_H
