#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <algorithm>
#include "../include/main.h"
#include "../include/TextEmbedder.h"
#include "../include/SimilaritySearch.h"

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
    Impl() : embedder_(std::unique_ptr<TextEmbedder>(new TextEmbedder())),
             searcher_(std::unique_ptr<SimilaritySearch>(new SimilaritySearch())),
             initialized_(false), memory_usage_(0) {}
    
    ~Impl() {
        release();
    }
    
    bool initialize(const std::string& model_path) {
        if (initialized_) release();
        if (!embedder_->initialize(model_path)) {
            return false;
        }
        searcher_ = std::unique_ptr<SimilaritySearch>(new SimilaritySearch());
        searcher_->initialize(embedder_->get_embedding_dim());
        initialized_ = true;
        update_memory_usage();
        return true;
    }

    bool initialize_bert(const std::string& model_path, const std::string& vocab_path) {
        if (initialized_) release();
        if (!embedder_->initialize_bert(model_path, vocab_path)) {
            return false;
        }
        searcher_ = std::unique_ptr<SimilaritySearch>(new SimilaritySearch());
        searcher_->initialize(embedder_->get_embedding_dim());
        initialized_ = true;
        update_memory_usage();
        return true;
    }
    
    bool load_qa_from_file(const std::string& file_path) {
        if (!initialized_) return false;
        
        std::vector<std::pair<std::string, std::string> > new_qa_pairs;
        if (!parse_qa_file(file_path, new_qa_pairs)) return false;
        
        std::vector<std::string> questions;
        std::vector<std::string> answers;
        for (const auto& qa : new_qa_pairs) {
            questions.push_back(qa.first);
            answers.push_back(qa.second);
        }
        
        std::vector<std::vector<float> > embeddings = embedder_->embed_batch(questions);
        searcher_->add_qa_batch(questions, answers, embeddings);
        
        qa_pairs_.insert(qa_pairs_.end(), new_qa_pairs.begin(), new_qa_pairs.end());
        update_memory_usage();
        return true;
    }

    bool load_qa_from_memory(const std::vector<std::string>& questions,
                           const std::vector<std::string>& answers) {
        if (!initialized_) return false;
        if (questions.size() != answers.size()) return false;
        
        std::vector<std::vector<float> > embeddings = embedder_->embed_batch(questions);
        searcher_->add_qa_batch(questions, answers, embeddings);
        
        for (size_t i = 0; i < questions.size(); i++) {
            qa_pairs_.push_back(std::make_pair(questions[i], answers[i]));
        }
        update_memory_usage();
        return true;
    }
    
    SearchResult search(const std::string& query) {
        if (!initialized_) return SearchResult("", "", 0.0f);
        std::vector<float> query_embedding = embedder_->embed(query);
        return searcher_->search(query_embedding);
    }
    
    std::vector<SearchResult> search_batch(const std::vector<std::string>& queries) {
        if (!initialized_) return std::vector<SearchResult>();
        std::vector<std::vector<float> > query_embeddings = embedder_->embed_batch(queries);
        return searcher_->search_batch(query_embeddings);
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
        embedder_->release();
        if (searcher_) searcher_->clear();
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

bool W2VEngine::initialize_bert(const std::string& model_path, const std::string& vocab_path) {
    return impl_->initialize_bert(model_path, vocab_path);
}

bool W2VEngine::load_qa_from_file(const std::string& file_path) {
    return impl_->load_qa_from_file(file_path);
}

bool W2VEngine::load_qa_from_memory(const std::vector<std::string>& questions,
                                   const std::vector<std::string>& answers) {
    return impl_->load_qa_from_memory(questions, answers);
}

std::pair<std::string, std::string> W2VEngine::search(const std::string& query, float* similarity_score) {
    SearchResult res = impl_->search(query);
    if (similarity_score) *similarity_score = res.similarity;
    return std::make_pair(res.question, res.answer);
}

std::vector<std::pair<std::string, std::string> > W2VEngine::search_batch(const std::vector<std::string>& queries,
                                                                         std::vector<float>* similarity_scores) {
    std::vector<SearchResult> results = impl_->search_batch(queries);
    std::vector<std::pair<std::string, std::string> > final_results;
    if (similarity_scores) similarity_scores->clear();
    for (const auto& res : results) {
        final_results.push_back(std::make_pair(res.question, res.answer));
        if (similarity_scores) similarity_scores->push_back(res.similarity);
    }
    return final_results;
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