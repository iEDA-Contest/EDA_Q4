BUILD_DIR = build
BIN = EDA_CHALLENGE_Q4.out
OUTPUT_DIR = output
TEST_CASE = /root/EDA_Q4/EDA_Q4/resources/test1
ARGV ?=\
-f$(TEST_CASE)/configure.xml \
-s$(TEST_CASE)/constraint.xml

.PHONY:

run:clean
	mkdir $(OUTPUT_DIR)
	cmake . -B $(BUILD_DIR) 
	make -C $(BUILD_DIR)
	cd $(BUILD_DIR) && ./$(BIN) $(ARGV)

build:clean
	mkdir $(OUTPUT_DIR)
	cmake . -B $(BUILD_DIR) -Ddebug=1
	make -C $(BUILD_DIR)

gdb:build
	cd $(BUILD_DIR) && gdb -s $(BIN) --args $(BIN) $(ARGV)

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)
