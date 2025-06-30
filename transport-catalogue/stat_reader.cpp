#include "stat_reader.h"

#include "input_reader.h"

using namespace std;

void DisplayBusInfo(const TransportCatalogue& tansport_catalogue, string_view bus_id, ostream& output) {
    output << tansport_catalogue.BusInfo(bus_id) << endl;
}

void DisplayStopInfo(const TransportCatalogue& tansport_catalogue, string_view stop_name, ostream& output) {
    output << tansport_catalogue.StopInfo(stop_name) << endl;
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