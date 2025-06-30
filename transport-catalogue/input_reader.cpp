#include "input_reader.h"

using namespace std;
using namespace Utils;

void InputReader::ParseLine(string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    for (auto& command : commands_) {
        if (command.command == "Stop") {
            catalogue.AddStop(move(command.id), ParseCoordinates(command.description));
        } else if (command.command == "Bus") {
            catalogue.AddBus(move(command.id), ParseRoute(command.description));
        }
    }
}

namespace Utils {

Coordinates ParseCoordinates(string_view str) {
    static const double nan = std::nan("");

    size_t not_space = str.find_first_not_of(' ');
    size_t comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    size_t not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = stod(std::string(str.substr(not_space2)));

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