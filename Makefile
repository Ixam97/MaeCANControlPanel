#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

#CXX = g++
#CXX = clang++

EXE = linux_test
SRC_DIR = src/
IMGUI_DIR = include/ImGuiDocking/
SOURCES = $(SRC_DIR)main.cpp
SOURCES += $(SRC_DIR)busmonitor.cpp $(SRC_DIR)can.cpp $(SRC_DIR)configworker.cpp $(SRC_DIR)devicemanager.cpp $(SRC_DIR)feedbackmonitor.cpp $(SRC_DIR)globals.cpp $(SRC_DIR)gui.cpp $(SRC_DIR)guihelpers.cpp $(SRC_DIR)updater.cpp
SOURCES += $(IMGUI_DIR)imgui.cpp $(IMGUI_DIR)imgui_draw.cpp $(IMGUI_DIR)imgui_impl_glfw.cpp $(IMGUI_DIR)imgui_impl_opengl3.cpp $(IMGUI_DIR)imgui_tables.cpp $(IMGUI_DIR)imgui_widgets.cpp
SOURCES +=
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL -lpthread

CXXFLAGS = -I$(IMGUI_DIR) -Iinclude/GLFW
CXXFLAGS += -g -Wall -Wformat -std=c++17 -DGLFW
LIBS =

##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

## This assumes a GL ES library available in the system, e.g. libGLESv2.so
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:$(SRC_DIR)%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)
	rm -f $(OBJS)

clean:
	rm -f $(EXE) $(OBJS)
