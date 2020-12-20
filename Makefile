CC = g++
CC_FLAGS = -c -std=c++11
LD_FLAGS = -pthread

SRC_DIR = src
RQTRANS_SRC_DIR = $(SRC_DIR)/rqtrans
OBJ_DIR = build
RQTRANS_OBJ_DIR = $(OBJ_DIR)/rqtrans
BIN_DIR = bin

RQTRANS_SRCS = $(wildcard $(RQTRANS_SRC_DIR)/*.cpp)
RQTRANS_OBJS = $(patsubst $(RQTRANS_SRC_DIR)/%.cpp,$(RQTRANS_OBJ_DIR)/%.o,$(RQTRANS_SRCS))
RQTRANS_DEPS = $(patsubst $(RQTRANS_SRC_DIR)/%.cpp,$(RQTRANS_OBJ_DIR)/%.d,$(RQTRANS_SRCS))

.PHONY: all
all: rqtrans

rqtrans: $(RQTRANS_OBJS)
	@mkdir -p bin
	$(CC) $(LD_FLAGS) -o $(BIN_DIR)/$@ $^

$(RQTRANS_OBJ_DIR)/%.o: $(RQTRANS_SRC_DIR)/%.cpp $(RQTRANS_OBJ_DIR)/%.d
	@mkdir -p build
	@mkdir -p build/rqtrans
	$(CC) $(CC_FLAGS) -o $@ $<

$(RQTRANS_OBJ_DIR)/%.d: $(RQTRANS_SRC_DIR)/%.cpp
	@mkdir -p build
	@mkdir -p build/rqtrans
	@echo "generating dependencies for $<"
	@set -e; rm -f $@; \
	$(CC) -MM $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(RQTRANS_DEPS)


clean:
	rm -rf $(BIN_DIR)/*
	rm -rf $(OBJ_DIR)/*
