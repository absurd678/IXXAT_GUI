#ifndef INTERFACECAN_H
#define INTERFACECAN_H
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <fstream>
#include <chrono>
#include <atomic>
#include <thread>
#include <cmath>
#include <csignal>

// Маски и флаги для настройки CAN интерфейса
/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error message frame */
/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFU /* omit EFF, RTR, ERR flags */

#define BYTESSIZE 8 // Количество байт в пакете CAN

using json = nlohmann::json;

struct Param {      // Параметр из json файла (некоторые из полей, не все берутся)
    int id = 0; // Идентификатор кадра(dec)
    int param_num = 0;  // Параметр
    std::string param_name; // Наименование параметра
    int16_t data; // Инфа (заполнять случайными значениями)
    float freq = 0.0; // Частота выдачи
    std::string desc; // Описание
};

struct frameParam {     // Пакет CAN до конвертации
    Param param1, param2, param3, param4;
    uint64_t lastSent = 0;  // Чтобы соблюдать частоту отправки (счетчик), мс
    float frameFreq = 0.0;  // Частота всего параметра (максимальная среди 4-х)
};

class ICAN{ // Интерфейс отправителя и получателя

public:
    //-------------Поля----------------
    std::vector<frameParam> DataArray;     // Пакеты CAN в формате структуры (подг. перед байтовой отправкой)
    // Для CAN
    int DescriptorCAN;
    //----------------Методы------------------
    // Функция для вычисления максимальной частоты в frameParam
    ICAN(std::string canNAME);  // Нужно задать canX, X=0,...,N
    ~ICAN() {}
    int getMaxFreq(const frameParam& fp);   // Найти максимальную частоту в структуре frameParam
    void JSONtoDataArray(const std::string& filepath);  // Чтение JSON файла и заполнение DataArray

};

class SenderToCAN : public ICAN{    // Отправитель
 public:
    //----------------Поля--------------------
    std::atomic<bool> flagSend;  // Разрешение на отправку
    //----------------Методы------------------
    SenderToCAN(std::string canNAME) : ICAN(canNAME) {
        flagSend = false;
    }
    ~SenderToCAN() {}
    // Функция преобразования в CAN-формат
    void convertToCAN(const frameParam& frame, unsigned char output[BYTESSIZE]);
    // Отправка на CAN-сокет
    int sendToCAN(frameParam frameJSON);
    // процесс для отправки данных
    void canSendProcess();

};

class ReceiverFromCAN : public ICAN{    // Получатель
public:
    //----------------Методы------------------
    ReceiverFromCAN(std::string canNAME) : ICAN(canNAME) {}
    ~ReceiverFromCAN() {}

    // Функция преобразования из CAN-формата
    frameParam convertFromCAN(const unsigned char* canData);
    // Прием данных
    void receiveFromCAN();

};

#endif // INTERFACECAN_H
