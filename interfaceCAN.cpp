#include "interfaceCAN.h"

//------------------ICAN----------------------
ICAN::ICAN(std::string canNAME){
    sockaddr_can addrSockCAN;
    ifreq ifrCAN;
    // ===================== Настройка CAN =================
    printf("CAN Socket start up\r\n");

    if ((this->DescriptorCAN = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return;
    }

    strcpy(ifrCAN.ifr_name, canNAME.c_str());
    ioctl(this->DescriptorCAN, SIOCGIFINDEX, &ifrCAN);

    memset(&addrSockCAN, 0, sizeof(addrSockCAN));
    addrSockCAN.can_family = AF_CAN;
    addrSockCAN.can_ifindex = ifrCAN.ifr_ifindex;

    if (bind(this->DescriptorCAN, (struct sockaddr *)&addrSockCAN, sizeof(addrSockCAN)) < 0) {
        perror("Bind");
        return;
    }
}

int ICAN::getMaxFreq(const frameParam& fp){
    int maxFreq = 0;
    if (fp.param1.freq > maxFreq) maxFreq = fp.param1.freq;
    if (fp.param2.freq > maxFreq) maxFreq = fp.param2.freq;
    if (fp.param3.freq > maxFreq) maxFreq = fp.param3.freq;
    if (fp.param4.freq > maxFreq) maxFreq = fp.param4.freq;
    return maxFreq;
}
void ICAN::JSONtoDataArray(const std::string& filepath){
    std::ifstream fin(filepath);
    json parseData = json::parse(fin);

    // Временное хранилище для параметров
    std::vector<Param> tempParams;

    // Перебор элементов массива
    for (auto& elem : parseData["Перечень параметров"]) {
        Param newParam;

        newParam.id = elem["Идентификатор кадра(dec)"].is_number_integer() ? elem["Идентификатор кадра(dec)"].get<int>() : stoi(elem["Идентификатор кадра(dec)"].get<std::string>());

        newParam.param_num = elem["Параметр"].is_number_integer() ? elem["Параметр"].get<int>() : stoi(elem["Параметр"].get<std::string>());;

        newParam.param_name = elem["Наименование параметра"].get<std::string>();

        newParam.freq = elem["Частота выдачи"].is_number_float() || elem["Частота выдачи"].is_number_integer() ? elem["Частота выдачи"].get<float>() : stof(elem["Частота выдачи"].get<std::string>());

        newParam.desc = elem["Описание"].get<std::string>();

        newParam.data = (int16_t)rand(); // навешиваем случайное значение
        tempParams.push_back(newParam);
    }

    // Группировка параметров по 4 в frameParam
    size_t inc = 0;

    while (inc < tempParams.size()) {
        frameParam fp;
        Param paramsSelected[4] = {Param{}, Param{}, Param{}, Param{}};    // Чтобы выбрать 4 параметра или оставить ост. пустыми (не всегда их по факту 4)

        // Ну первое поле по-любому нужно
        paramsSelected[0] = tempParams[inc];

        // Теперь разберемся, а есть ли остальные 3 параметра с таким же id
        for (int i=1; i<sizeof(paramsSelected)/sizeof(Param); i++){
            if (inc+1 < tempParams.size() && tempParams[inc+1].id == paramsSelected[0].id) {
                paramsSelected[i] = tempParams[inc+1];
                inc++;      // Дальше по списку
            } else { // Если дальше нет нужного id - ну и не будет, остальные N можно не смотреть
                break;
            }
        }

        // Парсим в структуру
        fp.param1 = paramsSelected[0];
        fp.param2 = paramsSelected[1];
        fp.param3 = paramsSelected[2];
        fp.param4 = paramsSelected[3];

        fp.lastSent = std::chrono::system_clock::now().time_since_epoch() /
    std::chrono::milliseconds(1);   // Текущее время в мс
        fp.frameFreq = getMaxFreq(fp);  // Определяем максимальную частоту в параметре
        DataArray.push_back(fp);        // ЗАПОЛНЕНИЕ DATAARRAY

        inc++; // Дальше по списку
    }
}


// ----------------SenderToCAN----------------

// Функция преобразования в CAN-формат
void SenderToCAN::convertToCAN(const frameParam& frame, unsigned char output[BYTESSIZE]){
    unsigned char dataCAN[8]; // Статический буфер для возвращаемых данных
    int16_t package[4] = {frame.param1.data, frame.param2.data, frame.param3.data, frame.param4.data};

    // Преобразуем в массив байт
    int j=0;

    for (int i = 0; i < 4; i++) {
        dataCAN[j] = (package[i]>>8);
        dataCAN[++j] = (package[i]);
        j++;
    }


    std::copy(dataCAN, dataCAN+8, output);
}
// Отправка на CAN-сокет
int SenderToCAN::sendToCAN(frameParam frameJSON){
    can_frame frameCAN;

    frameCAN.can_id = CAN_EFF_FLAG;    // Попытка сделать id 29-битным
    frameCAN.can_id |= frameJSON.param1.id;
    frameCAN.can_dlc = 8;

    unsigned char candata[8];
    printf("\njson0 %x [8] %x %x %x %x\n", frameJSON.param1.id, frameJSON.param1.data, frameJSON.param2.data, frameJSON.param3.data, frameJSON.param4.data);
    convertToCAN(frameJSON, candata);
    //printf("\ncan0 %x [8] %x%x\n", frameJSON.param1.id, candata);
    memcpy(frameCAN.data, candata, 8);
    if (write(this->DescriptorCAN, &frameCAN, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
        perror("Write");
        return 1;
    }
    return 0;
}
// процесс для отправки данных
void SenderToCAN::canSendProcess(){

    while (this->flagSend) {    // Когда нажали кнопку начинаем отправку
        // Получаем текущее время в миллисекундах
        auto now = std::chrono::steady_clock::now();
        uint64_t curr_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        //bool allSent = true;  // Флаг, что все фреймы были отправлены в этой итерации

        for (auto& frame : this->DataArray) {
            // Рассчитываем период для этого фрейма в миллисекундах
            float period_ms = 1000.0f / frame.frameFreq;

            // Проверяем, пришло ли время отправки
            if (curr_time - frame.lastSent >= static_cast<uint64_t>(std::round(period_ms))) {
                // Конвертация и отправка в CAN
                sendToCAN(frame);
                // Обновляем время последней отправки
                frame.lastSent = curr_time;

            } else {
                // Если хотя бы один фрейм не был отправлен, устанавливаем флаг в false
                //allSent = false;
            }
        }

        /*// Если все фреймы были отправлены в этой итерации
        if (allSent) {
            // В реальной системе здесь может быть пауза
            // Но в нашем случае просто продолжаем цикл
            //break;
        }*/

        // Небольшая задержка для снижения нагрузки на CPU
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}


// -----------------------ReceiverFromCAN---------------------

// Функция преобразования из CAN-формата
frameParam ReceiverFromCAN::convertFromCAN(const unsigned char* canData){
    frameParam result;

    // Собираем 64-битное значение из массива байт (little-endian)
    uint64_t combined = 0;
    for (int i = 0; i < 8; ++i) {
        combined |= static_cast<uint64_t>(canData[i]) << (i * 8);
    }

    // Извлекаем значения с помощью обратных сдвигов и масок
    result.param1.data = static_cast<int16_t>((combined >> 0) & 0xFFFF);
    result.param2.data = static_cast<int16_t>((combined >> 16) & 0xFFFF);
    result.param3.data = static_cast<int16_t>((combined >> 32) & 0xFFFF);
    result.param4.data = static_cast<int16_t>((combined >> 48) & 0xFFFF);

    return result;
}
// Прием данных
void ReceiverFromCAN::receiveFromCAN(){
    // Создаем словарь для отслеживания получения параметров
     std::map<int, bool> receivedMap;
     // Ост. переменные
     can_frame frameCAN; // Временная переменная для хранения пакета CAN

     // Инициализируем словарь (все ID не получены)
     for (const auto& frame : this->DataArray) {
         // Предполагаем, что все параметры в frame имеют одинаковый ID
         receivedMap[frame.param1.id] = false;
     }



     while (true) {
         // Чтение данных из CAN-сокета
         ssize_t bytes_read = read(this->DescriptorCAN, &frameCAN, sizeof(can_frame));
         if (bytes_read < 0) {
             // Обработка ошибки
             continue;
         }

         // Преобразование в структуру
         frameParam newFrame = convertFromCAN(frameCAN.data);
          //Определить формат CAN ID
          /*printf("Print the received CAN ID = %d\n", */
             (frameCAN.can_id & CAN_EFF_FLAG) ? (frameCAN.can_id &= CAN_EFF_MASK)
                                           : (frameCAN.can_id &= CAN_SFF_MASK);

         newFrame.param1.id = frameCAN.can_id;

         // Поиск соответствующего фрейма в массиве по ID
         for (auto& frame : this->DataArray) {
             // Предполагаем, что ID хранится в param1
             if (frame.param1.id == newFrame.param1.id) {
                 // Обновляем только данные
                 frame.param1.data = newFrame.param1.data;
                 frame.param2.data = newFrame.param2.data;
                 frame.param3.data = newFrame.param3.data;
                 frame.param4.data = newFrame.param4.data;

                 // Помечаем ID как полученный
                 receivedMap[frame.param1.id] = true;
                 //std::cout<<frame.param1.id<<" "<<frame.param2.id<<" "<<frame.param3.id<<" "<<frame.param4.id<<std::endl;
             }
         }

         // Проверяем, все ли данные получены
         bool allReceived = true;
         for (const auto& [id, status] : receivedMap) {
             if (!status) {
                 allReceived = false;
                 break;
             }
         }

         if (allReceived) {
             printf("All parameters received!\n");
             //break;
         }
     }
}

