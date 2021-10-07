BUILD_DIR = build
BIN = EDA_CHALLENGE_Q4.out
ARGV ?=\
-f/home/liangyuyao/EDA_CHALLENGE_Q4/resources/test0/configure0.xml \
--constraint=/home/liangyuyao/EDA_CHALLENGE_Q4/resources/test0/constraint0.xml

.PHONY:

run:clean
	cmake . -B $(BUILD_DIR)
	make -C $(BUILD_DIR)
	$(BUILD_DIR)/$(BIN) $(ARGV)

build:clean
	cmake . -B $(BUILD_DIR) -Ddebug=1
	make -C $(BUILD_DIR)

gdb:build
	gdb -s $(BUILD_DIR)/$(BIN) --args $(BUILD_DIR)/$(BIN) $(ARGV)

clean:
	rm -rf $(BUILD_DIR)