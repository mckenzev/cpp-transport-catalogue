#include <iostream>

#include "json_reader.h"

using namespace std;

int main() {
    JsonReader reader(cin, cout);
    reader.ParseBaseRequests();
    reader.ParseStatRequests();
}