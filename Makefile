TARGET = fx
CXX = g++
INC_DIR = inc
SRC_DIR = src
CFLAGS = -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -std=c++20 -O3 -g

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
