#pragma once

#include <iostream>
#include <string_view>

#include "transport_catalogue.h"

/**
 * Парсинг запроса request с выводом результата
 */
void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, std::string_view request, std::ostream& output);

