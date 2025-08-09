#include "gui_can.h"
#include <QHeaderView>
#include <QVBoxLayout>
#include <QTimer>
#include <QHBoxLayout>
#include "thread"

GUI_CAN::GUI_CAN(QString jsonPath, QWidget *parent)
    : QMainWindow(parent),
      writeCAN("can0"), // Писатель на CAN0, читатель - на CAN1
      readCAN("can1"),
    jsonPath(std::move(jsonPath)),
    tableSend(new QTableWidget(this)),
    tableReceive(new QTableWidget(this)),
    startButton(new QPushButton(tr("Старт"), this)),
    stopButton(new QPushButton(tr("Стоп"), this)),
    updateTimer(new QTimer(this))
{

    // Загрузка данных из JSON
    writeCAN.JSONtoDataArray(this->jsonPath.toStdString());
    readCAN.JSONtoDataArray(this->jsonPath.toStdString());

    // Настройка таблиц
    QStringList headers = {"№ CAN", "ID", "Кол. байтов", "Парам. 1", "Парам. 2", "Парам. 3", "Парам. 4"};
    tableSend->setColumnCount(headers.size());
    tableSend->setHorizontalHeaderLabels(headers);
    tableReceive->setColumnCount(headers.size());
    tableReceive->setHorizontalHeaderLabels(headers);

    // Заполнение таблиц
    initTable(tableSend, writeCAN.DataArray, "can0");
    initTable(tableReceive, readCAN.DataArray, "can1");

    // Компоновка
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Таблицы
    mainLayout->addWidget(tableSend);
    mainLayout->addWidget(tableReceive);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    mainLayout->addLayout(buttonLayout);

    setCentralWidget(centralWidget);

    // Подключение кнопок
    connect(startButton, &QPushButton::clicked, this, &GUI_CAN::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &GUI_CAN::onStopClicked);
    // Таймер для обновления таблицы приема
    connect(updateTimer, &QTimer::timeout, this, &GUI_CAN::updateReceivedTable);

    // Настройка потоков
    sendThread = std::thread([this]() {
        while (true){
            if (writeCAN.flagSend == false) continue;
            writeCAN.canSendProcess();
        }
    });

    receiveThread = std::thread([this]() {
        readCAN.receiveFromCAN();
    });

    updateTimer->start(100); // Обновление каждые 100 мс
}

GUI_CAN::~GUI_CAN()
{
    sendThread.join();
    receiveThread.join();

}

void GUI_CAN::initTable(QTableWidget *table, const std::vector<frameParam> &data,
                        std::string can_name)
{
    table->setRowCount(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        const frameParam &fp = data[i];

        // Вых. (номер кадра)
        table->setItem(i, 0, new QTableWidgetItem(can_name.c_str()));

        // ID
        table->setItem(i, 1, new QTableWidgetItem(QString::number(fp.param1.id)));

        // Кол. байтов (всегда 8 для CAN-фрейма)
        table->setItem(i, 2, new QTableWidgetItem("8"));

        // Параметры
        table->setItem(i, 3, new QTableWidgetItem(QString::number(fp.param1.data)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(fp.param2.data)));
        table->setItem(i, 5, new QTableWidgetItem(QString::number(fp.param3.data)));
        table->setItem(i, 6, new QTableWidgetItem(QString::number(fp.param4.data)));
    }

    // Автоматическое растягивание колонок
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void GUI_CAN::onStartClicked()
{
    writeCAN.flagSend = true;
}

void GUI_CAN::onStopClicked()
{
    writeCAN.flagSend = false;
}

void GUI_CAN::updateReceivedTable()
{
    // Обновляем только таблицу приема
    for (int i = 0; i < tableReceive->rowCount(); ++i) {
        const frameParam &fp = readCAN.DataArray[i];

        // Обновляем значения параметров
        tableReceive->item(i, 3)->setText(QString::number(fp.param1.data));
        tableReceive->item(i, 4)->setText(QString::number(fp.param2.data));
        tableReceive->item(i, 5)->setText(QString::number(fp.param3.data));
        tableReceive->item(i, 6)->setText(QString::number(fp.param4.data));
    }
}
