BUILD_DIR = build
BIN = EDA_CHALLENGE_Q4.out
OUTPUT_DIR = output
TEST_CASE = /home/liangyuyao/EDA_CHALLENGE_Q4/resources/test0
ARGV ?=\
-f$(TEST_CASE)/configure.xml \
--constraint=$(TEST_CASE)/constraint.xml

.PHONY:

run:clean
	mkdir $(OUTPUT_DIR)
	cmake . -B $(BUILD_DIR) 
	make -C $(BUILD_DIR)
	$(BUILD_DIR)/$(BIN) $(ARGV)

build:clean
	mkdir $(OUTPUT_DIR)
	cmake . -B $(BUILD_DIR) -Ddebug=1
	make -C $(BUILD_DIR)

gdb:build
	gdb -s $(BUILD_DIR)/$(BIN) --args $(BUILD_DIR)/$(BIN) $(ARGV)

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)
