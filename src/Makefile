CC=gcc
CFLAGS+=-W -g -DNDEBUG -Dlinux
CFLAGS += -std=c++11 -I$(IMGUI_DIR)/include -I$(IMGUI_DIR)/backends
CFLAGS += -g -Wall -Wformat
LIBS = -lHL -lHLU -lHDU -lHD -lGLU -lGL -lglut -lstdc++ -lm -lglfw -lGLEW

TARGET=app
HDRS=
IMGUI_DIR = ../
SRCS=app.cpp
SRCS += $(IMGUI_DIR)/include/imgui.cpp $(IMGUI_DIR)/include/imgui_demo.cpp $(IMGUI_DIR)/include/imgui_draw.cpp $(IMGUI_DIR)/include/imgui_tables.cpp $(IMGUI_DIR)/include/imgui_widgets.cpp
SRCS += $(IMGUI_DIR)/backends/imgui_impl_glut.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl2.cpp

#OBJS=$(SRCS:.cpp=.o)
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LIBS)

.PHONY: clean
clean:
	-rm -f $(OBJS) $(TARGET)
