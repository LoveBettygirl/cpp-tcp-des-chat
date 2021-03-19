EPOLL_NAME = epoll_chat
BUILD_DIR ?= .
OBJ_DIR ?= $(BUILD_DIR)/obj
EPOLL_BINARY ?= $(BUILD_DIR)/$(EPOLL_NAME)

.DEFAULT_GOAL = epoll

# Compilation flags
CXX = g++
LD = g++
CXXFLAGS   += -O2 -MMD -std=c++11

# Files to be compiled
SRCS = $(shell find ./ -name "*.cpp")
OBJS = $(SRCS:./%.cpp=$(OBJ_DIR)/%.o)
CIPHER_SRCS = $(shell find ./cipher/ -name "*.cpp")
CIPHER_OBJS = $(CIPHER_SRCS:./%.cpp=$(OBJ_DIR)/%.o)
EPOLL_SRCS = $(shell find ./epoll/ -name "*.cpp")
EPOLL_OBJS = $(EPOLL_SRCS:./%.cpp=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: ./%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

# Depencies
-include $(OBJS:.o=.d)

# Some convinient rules

.PHONY: epoll run clean

epoll: $(EPOLL_BINARY)

# Link
$(EPOLL_BINARY): $(CIPHER_OBJS) $(EPOLL_OBJS)
	@$(LD) -O2 -o $@ $^

clean: 
	rm -rf $(OBJ_DIR)
	rm -rf $(BUILD_DIR)/$(EPOLL_NAME)

