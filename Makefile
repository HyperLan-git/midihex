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

EXE = bin/midihex
IMGUI_DIR = ../imgui
OBJDIR = bin/obj
INCLUDEFOLDER = include
SRCFOLDER = src
SOURCES = $(shell find $(SRCFOLDER) -type f -name '*.cpp' | sed -z 's/\n/ /g')
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += lib/stbi/stb_image.c
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
INCLUDE = $(addprefix -I ,$(shell find $(INCLUDEFOLDER) -type d | sed -z 's/\n/ /g'))
INCLUDE += -I lib/stbi -I lib/portable-file-dialogs
O1 = $(SOURCES:$(IMGUI_DIR)/%.cpp=$(OBJDIR)/imgui/%.o)
O2 = $(O1:lib/%.c=bin/obj/lib/%.o)
OBJS = $(O2:$(SRCFOLDER)/%.cpp=$(OBJDIR)/%.o)
UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++23 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -Wall -Wformat -Wno-unknown-pragmas -Wno-class-memaccess
ifdef DEBUG
CXXFLAGS += -g -fsanitize=address
else
CXXFLAGS += -O3
endif
LIBS =

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib -L/opt/homebrew/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include -I/opt/homebrew/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif


all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------
# libs
bin/obj/lib/stbi/stb_image.o: lib/stbi/stb_image.c lib/stbi/stb_image.h
	@mkdir -p 'bin/obj/lib/stbi'
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ -D STBI_NO_BMP -D STBI_NO_PSD -D STBI_NO_TGA -D STBI_NO_GIF -D STBI_NO_HDR -D STBI_NO_PIC -D STBI_NO_PNM

$(OBJDIR)/%.o: $(SRCFOLDER)/%.cpp $(INCLUDEFOLDER)/%.hpp
	@mkdir -p '$(@D)'
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

$(OBJDIR)/%.o: $(SRCFOLDER)/%.cpp
	@mkdir -p '$(@D)'
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

$(OBJDIR)/main.o: $(SRCFOLDER)/main.cpp
	@mkdir -p '$(@D)'
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

$(OBJDIR)/imgui/%.o:$(IMGUI_DIR)/%.cpp
	@mkdir -p '$(@D)'
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c -o $@ $<

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -rf bin/*
