# Makefile for YanbianVillage (MinGW-w64 g++ 17 + OpenGL 3.3 Core Profile)
CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -O2 -Wall -DPROJECT_ROOT='"$(CURDIR)"' -DGLM_ENABLE_EXPERIMENTAL
INCLUDES := -Isrc -Ithird_party/glad/include -Ithird_party/glfw/include -Ithird_party/glm -Ithird_party/stb
LDFLAGS  := -Lthird_party/glfw/lib-mingw-w64 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32

CPP_SRCS := $(wildcard src/*.cpp src/core/*.cpp src/math/*.cpp src/scene/*.cpp src/fx/*.cpp src/ui/*.cpp)
C_SRCS   := third_party/glad/src/glad.c
TARGET   := YanbianVillage.exe

all: $(TARGET)

$(TARGET): $(CPP_SRCS) $(C_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(TARGET)
