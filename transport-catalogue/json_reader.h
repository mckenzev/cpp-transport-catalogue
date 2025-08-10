#pragma once

#include <iostream>
#include <vector>

#include "domain.h"
#include "json.h"
#include "request_handler.h"

class JsonReader {
public:
    JsonReader(std::istream& input, std::ostream& output);
    JsonReader(const JsonReader&) = delete;
    JsonReader& operator=(const JsonReader&) = delete;
    void ParseBaseRequests();
    void ParseStatRequests();
    
private:
    std::ostream& output_;
    json::Document doc_;
    RequestHandler handler_;


    // Разделяет запросы из `base_requests` на запросы по созданию Stop и запросы по созданию Bus
    std::pair<std::vector<json::Dict>, std::vector<json::Dict>> SplitRequests(const json::Array& base_requests) const;

    void ParseStops(const std::vector<json::Dict>& stops_prop);
    void ParseBuses(const std::vector<json::Dict>& buses_prop);
    void SetRoadDistances(const std::vector<json::Dict>& stops_prop);
    std::vector<std::string_view> CreateRoute(const json::Array &stops) const;
    json::Dict MakeStopResponse(int id, const std::string& name) const;
    json::Dict MakeBusResponse(int id, const std::string& name) const;
    json::Dict RenderMap(int id) const;
    domain::dto::RenderSettings GetRenderSettings() const;
};
