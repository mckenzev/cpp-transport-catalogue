#include <iostream>
#include <string>
// #include <Windows.h>

// #include "input_reader.h"
#include "json_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    // SetConsoleOutputCP(CP_UTF8);

    JsonReader reader(cin, cout);
    reader.ParseBaseRequests();
    reader.ParseStatRequests();
}