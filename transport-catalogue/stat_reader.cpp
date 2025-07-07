#include "stat_reader.h"

#include <algorithm>
#include <iomanip>

#include "input_reader.h"

using namespace std;

// RAII обертка для изменения и возвращения значения precision
class PrecisionSetter {
public:
    explicit PrecisionSetter(ostream& output, size_t precision)
    : output_(output), old_prec_(output_.precision()) {
        output_ << setprecision(precision);
    }

    ~PrecisionSetter() {
        output_ << setprecision(old_prec_);
    }

private:
    ostream& output_;
    size_t old_prec_;
};

void DisplayBusInfo(const TransportCatalogue& tansport_catalogue, string_view bus_id, ostream& output) {
    PrecisionSetter ps(output, static_cast<size_t>(6)); // на случай, если precision после вывода должен стать прежним
    auto result = tansport_catalogue.GetBusInfo(bus_id);

    if (result == nullopt) {
        output << "Bus " << bus_id << ": not found" << endl;
    } else {
        output << "Bus " << bus_id << ": " << result->total_stops << " stops on route, "
                                           << result->uniq_stops  << " unique stops, "
                                           << result->distance    << " route length" << endl;
    }
}

void DisplayStopInfo(const TransportCatalogue& tansport_catalogue, string_view stop_name, ostream& output) {
    PrecisionSetter ps(output, static_cast<size_t>(6));
    auto buses = tansport_catalogue.GetStopInfo(stop_name);
    
    if (buses == nullopt) {
        output << "Stop " << stop_name << ": not found" << endl;
    } else if (buses->empty()) {
        output << "Stop " << stop_name << ": no buses" << endl;
    } else {
        // Сортировка, т.к. GetStopInfo возвращает массив без сортировки
        vector<string_view> buses_names;
        buses_names.reserve(buses->size());
        transform(buses->begin(), buses->end(), back_inserter(buses_names), [](const auto& bus) { return bus->name; });
        sort(buses_names.begin(), buses_names.end());
        output << "Stop " << stop_name << ": buses";
        for (auto name : buses_names) {
            output << ' ' << name;
        }
        output << endl;
    }
}

void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, string_view request, ostream& output) {
    auto words = Utils::Split(request, ' ');
    if (words[0] == "Bus") {
        request.remove_prefix(4);
        DisplayBusInfo(tansport_catalogue, request, output);
    } else if (words[0] == "Stop") {
        request.remove_prefix(5);
        DisplayStopInfo(tansport_catalogue, request, output);
    }
}