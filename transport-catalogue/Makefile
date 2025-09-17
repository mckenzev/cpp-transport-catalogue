GXX = g++
FLAGS = -Werror -Wextra -Wall -O3
STD20 = -std=c++20
SOURCE = geo.cpp \
		 domain.cpp \
		 transport_catalogue.cpp \
		 transport_router.cpp \
		 svg.cpp \
		 map_renderer.cpp \
		 request_handler.cpp \
		 json.cpp \
		 json_builder.cpp \
		 json_reader.cpp \
		 main.cpp
EXE = run.exe

build:
	$(GXX) $(FLAGS) $(STD20) $(SOURCE) -o $(EXE)

clean:
	rm -rf *.o $(EXE)