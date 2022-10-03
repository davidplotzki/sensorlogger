# Options to choose which interfaces and libraries should be supported:
OPTION_CURL = true
OPTION_MQTT = true
OPTION_TINKERFORGE = true

CXX      := -g++
CXXFLAGS := -pthread -Wall -Wextra -Wno-unused-parameter -O2
LDFLAGS  := -L/usr/lib -lstdc++ -lm
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := .
TARGET   := sensorlogger
INCLUDE  := -Iinclude/
SRC      := $(wildcard src/*.cpp)

# Libcurl support to make HTTP(s) requests.
ifeq ($(OPTION_CURL), true)
    CXXFLAGS += -DOPTION_CURL
    LDFLAGS  += -lcurl
endif

# MQTT Support
ifeq ($(OPTION_MQTT), true)
	CXXFLAGS += -DOPTION_MQTT
	LDFLAGS  += -lpaho-mqttpp3 -lpaho-mqtt3as
endif

# Tinkerforge Support
ifeq ($(OPTION_TINKERFORGE), true)
	CXXFLAGS += -DOPTION_TINKERFORGE
	INCLUDE  += -Itinkerforge/
	SRC      += $(wildcard tinkerforge/*.cpp)
endif

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(LDFLAGS) -o $(APP_DIR)/$(TARGET) $(OBJECTS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*