GXX = g++
FLAGS = -Werror -Wextra -Wall -O3
TEST_TYPE = uint8_t
TEST_TYPE_FLAG = -DTEST_TYPE=$(TEST_TYPE)
DEBUG = -g
STD20 = -std=c++20
SOURCE = geo.cpp \
		 transport_catalogue.cpp \
		 domain.cpp \
		 json_reader.cpp \
		 json.cpp \
		 map_renderer.cpp \
		 request_handler.cpp \
		 main.cpp
EXE = run.exe

build:
	$(GXX) $(FLAGS) $(STD20) $(SOURCE) -o $(EXE)

clean:
	rm -rf *.o $(EXE)