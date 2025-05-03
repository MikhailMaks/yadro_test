#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>

using namespace std;

// Функция для преобразования времени из формата HH:MM в минуты
int timeToMinutes(const string& time) {
    int hours = stoi(time.substr(0, 2));
    int minutes = stoi(time.substr(3, 2));
    return hours * 60 + minutes;
}

// Функция для преобразования времени из минут в формат HH:MM
string minutesToTime(int totalMinutes) {
    int hours = (totalMinutes / 60) % 24;
    int minutes = totalMinutes % 60;
    stringstream ss;
    ss << (hours < 10 ? "0" : "") << hours << ":" << (minutes < 10 ? "0" : "") << minutes;
    return ss.str();
}


int main(int argc, char* argv[]) {
        if (argc != 2) { // Проверяем, передано ли имя входного файла как аргумент командной строки
            cerr << "Usage: " << argv[0] << " <input_file>" << endl; // Выводим сообщение об использовании программы
            return 1; // Возвращаем код ошибки
        }

        ifstream inputFile(argv[1]); // Открываем входной файл
        if (!inputFile.is_open()) { // Проверяем, удалось ли открыть файл
            cerr << "Error opening input file!" << endl; // Выводим сообщение об ошибке
            return 1; // Возвращаем код ошибки
        }

    int numTables;
    string startTimeStr, endTimeStr;
    int hourlyRate;

    // Чтение количества столов
    if (!(inputFile >> numTables)) {
        cerr << "Error reading number of tables." << endl;
        return 1;
    }

    // Чтение времени начала и окончания работы
    inputFile.ignore();
    string line;
    getline(inputFile, line);
    stringstream ss(line);
    ss >> startTimeStr >> endTimeStr;


    // Чтение стоимости часа
    if (!(inputFile >> hourlyRate)) {
        cerr << "Error reading hourly rate." << endl;
        return 1;
    }

    int startTime = timeToMinutes(startTimeStr);
    int endTime = timeToMinutes(endTimeStr);

    // Проверка корректности времени начала и окончания работы
    if (startTime >= endTime) {
        cerr << "Error: Start time must be before end time." << endl;
        return 1;
    }

    // Структуры данных для хранения информации о клиентах и столах
    map<string, pair<int, int>> clientTableMap; // Каждому клиенту соответствует номер стола, за которым он сидит (-1 если не сидит ни за каким), и время за которым он занял этот стол. 
    map<string, bool> clientInClub; // Информация о том, находится ли клиент в клубе
    queue<string> waitingQueue;      // Очередь ожидания клиентов
    vector<bool> tableAvailability(numTables + 1, true); // Доступность столов (индексация с 1)
    vector<int> tableRevenue(numTables + 1, 0); // Выручка каждого стола
    vector<int> tableUsageTime(numTables + 1, 0); // Время использования стола в минутах

    inputFile.ignore(); // Считываем пустую строку после hourlyRate

    vector<string> events;

    // Обработка событий
    while (getline(inputFile, line)) {
        stringstream ss(line);
        string timeStr;
        int eventId;
        string clientName;
        int tableNumber;
        string error = "";

        ss >> timeStr >> eventId;

        int eventTime = timeToMinutes(timeStr);

        //Проверка времени события на нахождение в рабочем диапазоне
        if (eventTime < startTime) {
            events.push_back(line);
            events.push_back(timeStr + " 13 NotOpenYet");
            continue;
        }

        switch (eventId) {
        case 1: { // Клиент пришел
            ss >> clientName;
            if (clientInClub.count(clientName)) {
                error = "YouShallNotPass";
            } else {
                clientInClub[clientName] = true;
                clientTableMap[clientName] = make_pair(-1, -1); // -1 означает, что клиент не сидит за столом
            }
            break;
        }
        case 2: { // Клиент сел за стол
            ss >> clientName >> tableNumber;
            if (!clientInClub.count(clientName)) {
                error = "ClientUnknown";
            } else if (!tableAvailability[tableNumber]) {
                error = "PlaceIsBusy";
            }
            else if (clientTableMap[clientName].first != -1) { //Клиент уже сидит за столом

                const auto& table_pair = clientTableMap[clientName];
                if (table_pair.first == tableNumber) {
                    error = "PlaceIsBusy"; //если хочет сесть за тот же стол
                } else {
                    // Подсчет выручки и времени использования стола
                    int timeDiff = eventTime - table_pair.second/*время когда клиент сел за стол*/; // Время от начала работы до ухода клиента
                    if (timeDiff > 0 && table_pair.first != -1) {
                        int roundedHours = (timeDiff + 59) / 60; // Округление до большего часа
                        tableRevenue[table_pair.first] += roundedHours * hourlyRate;
                        tableUsageTime[table_pair.first] += timeDiff;
                    }
                    tableAvailability[table_pair.first] = true; //освобождаем старый стол
                    clientTableMap[clientName] = make_pair(tableNumber, eventTime); //пересаживаем клиента за новый стол
                    tableAvailability[tableNumber] = false; //занимаем стол
                }

            }else {
                clientTableMap[clientName] = make_pair(tableNumber, eventTime);
                tableAvailability[tableNumber] = false;
            }
            break;
        }
        case 3: { // Клиент ожидает
            ss >> clientName;
            bool hasFreeTable = false;
            for (int i = 1; i <= numTables; ++i) {
                if (tableAvailability[i]) {
                    hasFreeTable = true;
                    break;
                }
            }
            if (hasFreeTable) {
                error = "ICanWaitNoLonger!";
            }
            else if (waitingQueue.size() > numTables) {
                events.push_back(timeStr + " 11 " + clientName);
                clientInClub.erase(clientName);
            } else {
                waitingQueue.push(clientName);
            }
            break;
        }
        case 4: { // Клиент ушел
            ss >> clientName;
            if (!clientInClub.count(clientName)) {
                error = "ClientUnknown";
            } else {
                clientInClub.erase(clientName);
                const auto& table_pair = clientTableMap[clientName];

                // Подсчет выручки и времени использования стола
                int timeDiff = eventTime - table_pair.second/*время когда клиент сел за стол*/; // Время от начала работы до ухода клиента
                if (timeDiff > 0 && table_pair.first != -1) {
                    int roundedHours = (timeDiff + 59) / 60; // Округление до большего часа
                    tableRevenue[table_pair.first] += roundedHours * hourlyRate;
                    tableUsageTime[table_pair.first] += timeDiff;
                }

                clientTableMap.erase(clientName);
                tableAvailability[table_pair.first] = true; // Освобождаем стол

                // Обработка очереди ожидания
                if (!waitingQueue.empty()) {
                    string nextClient = waitingQueue.front();
                    waitingQueue.pop();

                    // Находим первый свободный стол
                    int freeTable = -1;
                    for (int i = 1; i <= numTables; ++i) {
                        if (tableAvailability[i]) {
                            freeTable = i;
                            break;
                        }
                    }

                    //Если нашелся свободный стол
                    if (freeTable != -1)
                    {
                        tableAvailability[freeTable] = false;
                        clientTableMap[nextClient] = make_pair(freeTable, eventTime);
                        clientInClub[nextClient] = true;
                        events.push_back(timeStr + " 12 " + nextClient + " " + to_string(freeTable));
                    }
                }
            }
            break;
        }
        default:
            cerr << "Unknown event ID: " << eventId << endl;
            return 1;
        }

        events.push_back(line);

        if (!error.empty()) {
            events.push_back(timeStr + " 13 " + error);
        }
    }

    // Завершение рабочего дня (событие 11 для всех оставшихся клиентов)
    vector<string> leavingClients;
    for (auto const& pair : clientInClub) {
        leavingClients.push_back(pair.first);
    }

    sort(leavingClients.begin(), leavingClients.end());

    for (const string& client : leavingClients) {
        events.push_back(minutesToTime(endTime) + " 11 " + client);

        const auto& table_pair = clientTableMap[client];
        int timeDiff = endTime - table_pair.second; // Время от начала работы до конца рабочего дня
        if (timeDiff > 0 && table_pair.first != -1) {
            int roundedHours = (timeDiff + 59) / 60; // Округление до большего часа
            tableRevenue[table_pair.first] += roundedHours * hourlyRate;
            tableUsageTime[table_pair.first] += timeDiff;
        }
    }

    // Вывод результатов
    cout << minutesToTime(startTime) << endl;
    for (const string& event : events) {
        cout << event << endl;
    }
    cout << minutesToTime(endTime) << endl;

    for (int i = 1; i <= numTables; ++i) {
        cout << i << " " << tableRevenue[i] << " " << minutesToTime(tableUsageTime[i]) << endl;
    }

    return 0;
}
