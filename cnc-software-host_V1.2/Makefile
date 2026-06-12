CXX = g++
MOC = moc

# pkg-config for automatic path finding
QT_CXXFLAGS = $(shell pkg-config --cflags Qt5Widgets Qt5Core Qt5Gui)
QT_LDFLAGS  = $(shell pkg-config --libs Qt5Widgets Qt5Core Qt5Gui) -pthread

CXXFLAGS = -Wall -Wextra -std=c++17 -O2 $(QT_CXXFLAGS) -fPIC -pthread
TARGET = CNC_Control
SRCS = main.cpp mainwindow.cpp gcodeviewer.cpp scurve_planner.cpp cnc_host_communicator.cpp

# files with Q_OBJECT Makro 
MOC_HDRS = mainwindow.h gcodeviewer.h
MOC_SRCS = $(MOC_HDRS:.h=.moc.cpp)

OBJS = $(SRCS:.cpp=.o) $(MOC_SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(QT_LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.moc.cpp: %.h
	$(MOC) $(QT_CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(MOC_SRCS)

.PHONY: all clean
