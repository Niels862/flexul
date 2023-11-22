TARGET = fxi
CXX = g++
INC_DIR = inc ../shared
SRC_DIR = src
CFLAGS = -Wall -Wextra -Wpedantic -Wfatal-errors -std=c++11 -g -O3

CPPFLAGS = $(addprefix -I, $(INC_DIR))
SOURCES = $(sort $(shell find $(SRC_DIR) -name '*.cpp'))
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.d)

.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(CFLAGS) $(CPPFLAGS) -o $@ $^
%.o: %.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) -MMD -o $@ -c $<
clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)
-include $(DEPS)
