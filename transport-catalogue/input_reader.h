#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace Utils {

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

// Coordinates ParseCoordinates(std::string_view str);
Coordinates ParseCoordinates(std::string_view lat_sv, std::string_view lng_sv);
std::string_view Trim(std::string_view string);
std::vector<std::string_view> Split(std::string_view string, char delim);
std::vector<std::string_view> ParseRoute(std::string_view route);
CommandDescription ParseCommandDescription(std::string_view line);

} // namespace Utils

class InputReader {
public:
    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(TransportCatalogue& catalogue) const;

private:
    std::vector<Utils::CommandDescription> commands_;

    void AddStop(const Utils::CommandDescription& command, TransportCatalogue& catalogue) const;
    void SetDistances(const Utils::CommandDescription& command, TransportCatalogue& catalogue) const;
    std::pair<std::string_view, int> ParseDistanceToStop(std::string_view str) const;
};

