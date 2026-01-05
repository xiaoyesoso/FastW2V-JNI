#include "../include/SimilaritySearch.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

class SimilaritySearch::Impl {
private:
    struct QAEntry {
        std::string question;
        std::string answer;
        std::vector<float> embedding;
        
        QAEntry(const std::string& q, const std::string& a, const std::vector<float>& e)
            : question(q), answer(a), embedding(e) {}
    };
    
    std::vector<QAEntry> qa_entries_;
    int embedding_dim_;
    bool initialized_;
    
    // 计算余弦相似度
    float cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) {
        if (vec1.size() != vec2.size() || vec1.empty()) {
            return 0.0f;
        }
        
        float dot_product = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        
        for (size_t i = 0; i < vec1.size(); i++) {
            dot_product += vec1[i] * vec2[i];
            norm1 += vec1[i] * vec1[i];
            norm2 += vec2[i] * vec2[i];
        }
        
        if (norm1 < 1e-9f || norm2 < 1e-9f) {
            return 0.0f;
        }
        
        return dot_product / (std::sqrt(norm1) * std::sqrt(norm2));
    }
    
    // 搜索单个查询
    SearchResult search_single(const std::vector<float>& query_embedding, int top_k) {
        if (qa_entries_.empty() || !initialized_) {
            return SearchResult("", "", 0.0f);
        }
        
        // 使用简单的线性搜索（对于小规模数据足够）
        // 对于大规模数据，可以考虑使用更高效的数据结构
        float best_similarity = -1.0f;
        size_t best_index = 0;
        
        for (size_t i = 0; i < qa_entries_.size(); i++) {
            float similarity = cosine_similarity(query_embedding, qa_entries_[i].embedding);
            if (similarity > best_similarity) {
                best_similarity = similarity;
                best_index = i;
            }
        }
        
        if (best_similarity < -1.0f) {
            return SearchResult("", "", 0.0f);
        }
        
        // 直接使用原始余弦相似度 [-1, 1]
        // 首先进行裁剪以防止浮点精度问题
        float final_score = std::max(-1.0f, std::min(1.0f, best_similarity));
        
        return SearchResult(qa_entries_[best_index].question,
                          qa_entries_[best_index].answer,
                          final_score);
    }

public:
    Impl() : embedding_dim_(0), initialized_(false) {}
    
    ~Impl() {
        clear();
    }
    
    bool initialize(int embedding_dim) {
        if (embedding_dim <= 0) {
            return false;
        }
        
        embedding_dim_ = embedding_dim;
        initialized_ = true;
        return true;
    }
    
    bool add_qa(const std::string& question, const std::string& answer, const std::vector<float>& embedding) {
        if (!initialized_ || embedding.size() != static_cast<size_t>(embedding_dim_)) {
            return false;
        }
        
        qa_entries_.emplace_back(question, answer, embedding);
        return true;
    }
    
    bool add_qa_batch(const std::vector<std::string>& questions,
                     const std::vector<std::string>& answers,
                     const std::vector<std::vector<float> >& embeddings) {
        if (!initialized_ || questions.size() != answers.size() || questions.size() != embeddings.size()) {
            return false;
        }
        
        for (size_t i = 0; i < questions.size(); i++) {
            if (embeddings[i].size() != static_cast<size_t>(embedding_dim_)) {
                return false;
            }
            
            if (!add_qa(questions[i], answers[i], embeddings[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    SearchResult search(const std::vector<float>& query_embedding, int top_k) {
        return search_single(query_embedding, top_k);
    }
    
    std::vector<SearchResult> search_batch(const std::vector<std::vector<float> >& query_embeddings, int top_k) {
        std::vector<SearchResult> results;
        if (!initialized_) {
            return results;
        }
        
        results.reserve(query_embeddings.size());
        for (const auto& embedding : query_embeddings) {
            results.push_back(search_single(embedding, top_k));
        }
        
        return results;
    }
    
    size_t size() const {
        return qa_entries_.size();
    }
    
    void clear() {
        qa_entries_.clear();
        embedding_dim_ = 0;
        initialized_ = false;
    }
    
    void optimize() {
        // 这里可以添加优化逻辑，比如构建索引
        // 对于小规模数据，线性搜索已经足够
    }
};

// SimilaritySearch 公共接口实现
SimilaritySearch::SimilaritySearch() : impl_(new Impl()) {}

SimilaritySearch::~SimilaritySearch() {}

bool SimilaritySearch::initialize(int embedding_dim) {
    return impl_->initialize(embedding_dim);
}

bool SimilaritySearch::add_qa(const std::string& question, const std::string& answer, const std::vector<float>& embedding) {
    return impl_->add_qa(question, answer, embedding);
}

bool SimilaritySearch::add_qa_batch(const std::vector<std::string>& questions,
                                   const std::vector<std::string>& answers,
                                   const std::vector<std::vector<float> >& embeddings) {
    return impl_->add_qa_batch(questions, answers, embeddings);
}

SearchResult SimilaritySearch::search(const std::vector<float>& query_embedding, int top_k) {
    return impl_->search(query_embedding, top_k);
}

std::vector<SearchResult> SimilaritySearch::search_batch(const std::vector<std::vector<float> >& query_embeddings, int top_k) {
    return impl_->search_batch(query_embeddings, top_k);
}

size_t SimilaritySearch::size() const {
    return impl_->size();
}

void SimilaritySearch::clear() {
    impl_->clear();
}

void SimilaritySearch::optimize() {
    impl_->optimize();
}