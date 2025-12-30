#include "../include/main.h"
#include "../include/TextEmbedder.h"
#include "../include/SimilaritySearch.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <memory>

class W2VEngine::Impl {
private:
    std::unique_ptr<TextEmbedder> embedder_;
    std::unique_ptr<SimilaritySearch> searcher_;
    std::vector<std::pair<std::string, std::string> > qa_pairs_;
    bool initialized_;
    size_t memory_usage_;
    
    // 计算内存使用量
    void update_memory_usage() {
        memory_usage_ = 0;
        
        // 估算QA对的内存使用
        for (const auto& qa : qa_pairs_) {
            memory_usage_ += qa.first.capacity() + qa.second.capacity();
        }
        
        // 估算向量化器的内存使用
        if (embedder_ && embedder_->is_initialized()) {
            memory_usage_ += embedder_->get_memory_usage();
            memory_usage_ += embedder_->get_embedding_dim() * sizeof(float) * qa_pairs_.size();
        }
        
        // 加上其他开销
        memory_usage_ += sizeof(*this);
    }
    
    // 解析QA文件（支持CSV格式）
    bool parse_qa_file(const std::string& file_path, 
                      std::vector<std::pair<std::string, std::string> >& qa_pairs) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << file_path << std::endl;
            return false;
        }
        
        std::string line;
        int line_num = 0;
        bool is_first_line = true;
        
        while (std::getline(file, line)) {
            line_num++;
            
            // 跳过空行
            if (line.empty()) {
                continue;
            }
            
            // 检查是否为CSV头部行（包含question,answer,category等字段名）
            if (is_first_line) {
                is_first_line = false;
                // 检查是否包含CSV头部关键词
                if (line.find("question") != std::string::npos && 
                    line.find("answer") != std::string::npos) {
                    continue; // 跳过CSV头部行
                }
            }
            
            // 跳过注释行
            if (line[0] == '#') {
                continue;
            }
            
            // 查找分隔符（支持多种格式）
            size_t sep_pos = line.find('|');
            if (sep_pos == std::string::npos) {
                sep_pos = line.find('\t');
            }
            if (sep_pos == std::string::npos) {
                sep_pos = line.find(',');
            }
            
            if (sep_pos == std::string::npos || sep_pos == 0 || sep_pos == line.length() - 1) {
                std::cerr << "第 " << line_num << " 行格式错误: " << line << std::endl;
                continue;
            }
            
            std::string question = line.substr(0, sep_pos);
            std::string answer = line.substr(sep_pos + 1);
            
            // 去除首尾空格
            question.erase(0, question.find_first_not_of(" \t\n\r"));
            question.erase(question.find_last_not_of(" \t\n\r") + 1);
            
            answer.erase(0, answer.find_first_not_of(" \t\n\r"));
            answer.erase(answer.find_last_not_of(" \t\n\r") + 1);
            
            // 处理CSV中可能存在的引号
            if (question.size() >= 2 && question[0] == '"' && question.back() == '"') {
                question = question.substr(1, question.size() - 2);
            }
            if (answer.size() >= 2 && answer[0] == '"' && answer.back() == '"') {
                answer = answer.substr(1, answer.size() - 2);
            }
            
            if (!question.empty() && !answer.empty()) {
                qa_pairs.emplace_back(question, answer);
            }
        }
        
        file.close();
        return !qa_pairs.empty();
    }

public:
    Impl() : initialized_(false), memory_usage_(0) {
        embedder_.reset(new TextEmbedder());
        searcher_.reset(new SimilaritySearch());
    }
    
    ~Impl() {
        release();
    }
    
    bool initialize(const std::string& model_path) {
        if (initialized_) {
            release();
        }
        
        if (!embedder_->initialize(model_path)) {
            std::cerr << "初始化向量化器失败" << std::endl;
            return false;
        }
        
        initialized_ = true;
        return true;
    }
    
    bool load_qa_from_file(const std::string& file_path) {
        if (!initialized_) {
            std::cerr << "引擎未初始化" << std::endl;
            return false;
        }
        
        std::vector<std::pair<std::string, std::string> > new_qa_pairs;
        if (!parse_qa_file(file_path, new_qa_pairs)) {
            std::cerr << "解析QA文件失败" << std::endl;
            return false;
        }
        
        // 提取问题文本用于训练向量化器
        std::vector<std::string> questions;
        questions.reserve(new_qa_pairs.size());
        for (const auto& qa : new_qa_pairs) {
            questions.push_back(qa.first);
        }
        
        // 训练向量化器（基于TF-IDF）
        // 注意：这里简化了，实际应该使用预训练模型
        
        // 生成向量
        std::vector<std::vector<float> > embeddings = embedder_->embed_batch(questions);
        if (embeddings.size() != new_qa_pairs.size()) {
            std::cerr << "向量化失败" << std::endl;
            return false;
        }
        
        // 初始化搜索器
        int embedding_dim = embedder_->get_embedding_dim();
        if (!searcher_->initialize(embedding_dim)) {
            std::cerr << "初始化搜索器失败" << std::endl;
            return false;
        }
        
        // 添加QA对到搜索器
        std::vector<std::string> question_texts;
        std::vector<std::string> answer_texts;
        question_texts.reserve(new_qa_pairs.size());
        answer_texts.reserve(new_qa_pairs.size());
        
        for (const auto& qa : new_qa_pairs) {
            question_texts.push_back(qa.first);
            answer_texts.push_back(qa.second);
        }
        
        if (!searcher_->add_qa_batch(question_texts, answer_texts, embeddings)) {
            std::cerr << "添加QA对失败" << std::endl;
            return false;
        }
        
        // 更新内存中的QA对
        qa_pairs_ = std::move(new_qa_pairs);
        
        // 更新内存使用量
        update_memory_usage();
        
        std::cout << "成功加载 " << qa_pairs_.size() << " 个QA对" << std::endl;
        std::cout << "内存使用量: " << memory_usage_ / 1024 / 1024 << " MB" << std::endl;
        
        return true;
    }
    
    bool load_qa_from_memory(const std::vector<std::pair<std::string, std::string> >& qa_pairs) {
        if (!initialized_) {
            std::cerr << "引擎未初始化" << std::endl;
            return false;
        }
        
        if (qa_pairs.empty()) {
            std::cerr << "QA对列表为空" << std::endl;
            return false;
        }
        
        // 提取问题文本
        std::vector<std::string> questions;
        questions.reserve(qa_pairs.size());
        for (const auto& qa : qa_pairs) {
            questions.push_back(qa.first);
        }
        
        // 生成向量
        std::vector<std::vector<float> > embeddings = embedder_->embed_batch(questions);
        if (embeddings.size() != qa_pairs.size()) {
            std::cerr << "向量化失败" << std::endl;
            return false;
        }
        
        // 初始化搜索器
        int embedding_dim = embedder_->get_embedding_dim();
        if (!searcher_->initialize(embedding_dim)) {
            std::cerr << "初始化搜索器失败" << std::endl;
            return false;
        }
        
        // 添加QA对到搜索器
        std::vector<std::string> question_texts;
        std::vector<std::string> answer_texts;
        question_texts.reserve(qa_pairs.size());
        answer_texts.reserve(qa_pairs.size());
        
        for (const auto& qa : qa_pairs) {
            question_texts.push_back(qa.first);
            answer_texts.push_back(qa.second);
        }
        
        if (!searcher_->add_qa_batch(question_texts, answer_texts, embeddings)) {
            std::cerr << "添加QA对失败" << std::endl;
            return false;
        }
        
        // 更新内存中的QA对
        qa_pairs_ = qa_pairs;
        
        // 更新内存使用量
        update_memory_usage();
        
        std::cout << "成功加载 " << qa_pairs_.size() << " 个QA对" << std::endl;
        std::cout << "内存使用量: " << memory_usage_ / 1024 / 1024 << " MB" << std::endl;
        
        return true;
    }
    
    std::pair<std::string, std::string> search(const std::string& query, float* similarity_score) {
        if (!initialized_ || qa_pairs_.empty()) {
            if (similarity_score) *similarity_score = 0.0f;
            return std::make_pair("", "");
        }
        
        // 向量化查询
        std::vector<float> query_embedding = embedder_->embed(query);
        if (query_embedding.empty()) {
            if (similarity_score) *similarity_score = 0.0f;
            return std::make_pair("", "");
        }
        
        // 搜索
        SearchResult result = searcher_->search(query_embedding);
        
        if (similarity_score) {
            *similarity_score = result.similarity;
        }
        
        return std::make_pair(result.question, result.answer);
    }
    
    std::vector<std::pair<std::string, std::string> > search_batch(const std::vector<std::string>& queries,
                                                                 std::vector<float>* similarity_scores = nullptr) {
        std::vector<std::pair<std::string, std::string> > results;
        
        if (!initialized_ || qa_pairs_.empty()) {
            if (similarity_scores) similarity_scores->clear();
            return results;
        }
        
        // 向量化所有查询
        std::vector<std::vector<float> > query_embeddings = embedder_->embed_batch(queries);
        if (query_embeddings.empty()) {
            if (similarity_scores) similarity_scores->clear();
            return results;
        }
        
        // 批量搜索
        std::vector<SearchResult> search_results = searcher_->search_batch(query_embeddings);
        
        // 转换结果格式
        results.reserve(search_results.size());
        if (similarity_scores) {
            similarity_scores->reserve(search_results.size());
        }
        
        for (const auto& result : search_results) {
            results.emplace_back(result.question, result.answer);
            if (similarity_scores) {
                similarity_scores->push_back(result.similarity);
            }
        }
        
        return results;
    }
    
    size_t get_qa_count() const {
        return qa_pairs_.size();
    }
    
    int get_embedding_dim() const {
        return embedder_->get_embedding_dim();
    }
    
    size_t get_memory_usage() const {
        return memory_usage_;
    }
    
    void release() {
        if (embedder_) {
            embedder_->release();
        }
        
        if (searcher_) {
            searcher_->clear();
        }
        
        qa_pairs_.clear();
        initialized_ = false;
        memory_usage_ = 0;
    }
};

// W2VEngine 公共接口实现
W2VEngine::W2VEngine() : impl_(new Impl()) {}

W2VEngine::~W2VEngine() {}

bool W2VEngine::initialize(const std::string& model_path) {
    return impl_->initialize(model_path);
}

bool W2VEngine::load_qa_from_file(const std::string& file_path) {
    return impl_->load_qa_from_file(file_path);
}

bool W2VEngine::load_qa_from_memory(const std::vector<std::pair<std::string, std::string> >& qa_pairs) {
    return impl_->load_qa_from_memory(qa_pairs);
}

std::pair<std::string, std::string> W2VEngine::search(const std::string& query, float* similarity_score) {
    return impl_->search(query, similarity_score);
}

std::vector<std::pair<std::string, std::string> > W2VEngine::search_batch(const std::vector<std::string>& queries,
                                                                         std::vector<float>* similarity_scores) {
    return impl_->search_batch(queries, similarity_scores);
}

size_t W2VEngine::get_qa_count() const {
    return impl_->get_qa_count();
}

int W2VEngine::get_embedding_dim() const {
    return impl_->get_embedding_dim();
}

size_t W2VEngine::get_memory_usage() const {
    return impl_->get_memory_usage();
}

void W2VEngine::release() {
    impl_->release();
}