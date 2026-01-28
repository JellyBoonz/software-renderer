CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(shell pkg-config --cflags glfw3) -I./imgui
LDFLAGS = $(shell pkg-config --libs glfw3) -framework OpenGL -framework Cocoa -framework IOKit

# Source files
SOURCES = main.cpp pipeline.cpp color.cpp imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = output

# Default target
all: $(TARGET)

# Link object files into executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f imgui/*.o imgui/backends/*.o

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild
