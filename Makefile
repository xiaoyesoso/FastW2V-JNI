# 简单的 Makefile 用于本地测试
CXX = g++
CXXFLAGS = -std=c++17 -I./src -I./jni -O2 -Wall -Wextra
LDFLAGS = 

# 目标文件
OBJS = src/TextEmbedder.o src/SimilaritySearch.o src/main.o

# 默认目标
all: w2v_test

# 编译目标文件
src/TextEmbedder.o: src/TextEmbedder.cpp src/TextEmbedder.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/SimilaritySearch.o: src/SimilaritySearch.cpp src/SimilaritySearch.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/main.o: src/main.cpp src/main.h src/TextEmbedder.h src/SimilaritySearch.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 测试程序
test/simple_test.o: test/simple_test.cpp src/main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

w2v_test: $(OBJS) test/simple_test.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# 清理
clean:
	rm -f $(OBJS) test/simple_test.o w2v_test
	rm -rf build

# 运行测试
test: w2v_test
	./w2v_test

# 查看项目结构
tree:
	@echo "项目结构:"
	@find . -type f -name "*.cpp" -o -name "*.h" -o -name "*.java" -o -name "*.txt" -o -name "*.sh" -o -name "*.md" -o -name "Makefile" | sort

.PHONY: all clean test tree