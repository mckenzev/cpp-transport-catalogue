#include "input_reader.h"

#include <stdexcept>

using namespace std;
using namespace Utils;

void InputReader::ParseLine(string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    // Создание остановок (название - координаты)
    for (auto& command : commands_) {
        if (command.command == "Stop") {
            AddStop(command, catalogue);
        }
    }

    // Установка расстояний между соседними остановками
    for (auto& command : commands_) {
        if (command.command == "Stop") {
            SetDistances(command, catalogue);
        }
    }

    // Создание автобусов с маршрутами
    for (auto& command : commands_) {
        if (command.command == "Bus") {
            catalogue.AddBus(command.id, ParseRoute(command.description));
        }
    }
}

void InputReader::AddStop(const CommandDescription& command, TransportCatalogue& catalogue) const {
    // Под индексом 0 и 1 окажутся долгота и широта. С индекса 2 - строки формата "D[i]m to Stop[i]" - расстояния до других остановок
    auto tokens = Split(command.description, ',');
    auto coord = ParseCoordinates(tokens[0], tokens[1]);
    catalogue.AddStop(command.id, coord);
}

void InputReader::SetDistances(const Utils::CommandDescription& command, TransportCatalogue& catalogue) const {
    auto tokens = Split(command.description, ',');
    // Индексация дистанций начинается с индекса 2
    for (size_t i = 2; i < tokens.size(); ++i) {
        auto [to, distance] = ParseDistanceToStop(tokens[i]);
        catalogue.SetRoadDistance(command.id, to, distance);
    }
}

std::pair<std::string_view, int> InputReader::ParseDistanceToStop(std::string_view str) const {
    // В str строка формата "D[i]m to Stop[i]". Ориентир - 2 пробела
    // Если где то поиск пробела не увенчался успехом, значит передана строка недопустимого формата
    size_t distance_end = str.find(' ');
    if (distance_end > str.size()) {
        throw runtime_error("Invalid string format for the Stop request");
    }

    // Так как число идет перед буквой 'm', stoi без проблем распарсит число
    int distance = stoi(string(str.substr(0, distance_end)));

    size_t to_end = str.find(' ', distance_end + 1);
    if (to_end > str.size()) {
        throw runtime_error("Invalid string format for the Stop request");
    }
    string_view stop_name = str.substr(to_end + 1);

    return {stop_name, distance};
}

namespace Utils {

Coordinates ParseCoordinates(string_view lat_sv, string_view lng_sv) {
    double lat = stod(string(lat_sv));
    double lng = stod(string(lng_sv));

    return {lat, lng};
}

string_view Trim(string_view string) {
    const size_t start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

vector<string_view> Split(string_view string, char delim) {
    vector<string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        size_t delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }

        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

vector<string_view> ParseRoute(string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    vector<string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), next(stops.rbegin()), stops.rend()); // Изучить момент

    return results;
}

CommandDescription ParseCommandDescription(string_view line) {
    size_t colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    size_t space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    size_t not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {string(line.substr(0, space_pos)),
            string(line.substr(not_space, colon_pos - not_space)),
            string(line.substr(colon_pos + 1))};
}

} // namespace Utils