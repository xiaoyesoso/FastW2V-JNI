#include "../include/TextEmbedder.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <sstream>
#include <algorithm>

class TextEmbedder::Impl {
private:
    // 预训练词向量模型
    std::unordered_map<std::string, std::vector<float> > word_vectors_;
    int embedding_dim_;
    int max_word_len_; // 词表中词的最大长度（字节）
    bool initialized_;
    
    // 未知词的处理：使用零向量或随机向量
    std::vector<float> zero_vector_;
    std::vector<float> unknown_vector_;
    
    // 中文分词（基于词表的正向最大匹配）
    std::vector<std::string> tokenize_chinese(const std::string& text) {
        std::vector<std::string> tokens;
        size_t i = 0;
        while (i < text.length()) {
            // 1. 处理 ASCII 字符（英文、数字等）
            if ((text[i] & 0x80) == 0) {
                if (isspace(text[i])) {
                    i++;
                    continue;
                }
                std::string token;
                while (i < text.length() && (text[i] & 0x80) == 0 && 
                       (isalnum(text[i]) || text[i] == '_')) {
                    token += text[i++];
                }
                if (!token.empty()) {
                    tokens.push_back(token);
                } else if (i < text.length()) {
                    i++; // 跳过非字母数字的 ASCII
                }
                continue;
            }

            // 2. 处理中文字符 - 正向最大匹配
            bool matched = false;
            size_t remaining_len = text.length() - i;
            size_t match_limit = std::min((size_t)max_word_len_, remaining_len);
            
            for (size_t len = match_limit; len > 0; --len) {
                std::string sub = text.substr(i, len);
                if (word_vectors_.count(sub)) {
                    tokens.push_back(sub);
                    i += len;
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                // 如果没有匹配到词表中的任何词，则切分出一个 UTF-8 字符
                size_t char_len = 1;
                unsigned char c = (unsigned char)text[i];
                if (c >= 0xF0) char_len = 4;
                else if (c >= 0xE0) char_len = 3;
                else if (c >= 0xC0) char_len = 2;
                
                if (i + char_len > text.length()) char_len = text.length() - i;
                tokens.push_back(text.substr(i, char_len));
                i += char_len;
            }
        }
        return tokens;
    }
    
    // 加载预训练词向量模型（word2vec格式）
    bool load_pretrained_model(const std::string& model_path) {
        std::ifstream file(model_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法打开模型文件: " << model_path << std::endl;
            return false;
        }
        
        // 尝试读取模型头信息（word2vec格式：词汇量 维度）
        std::string header;
        std::getline(file, header);
        std::istringstream header_stream(header);
        
        int vocab_size = 0;
        header_stream >> vocab_size >> embedding_dim_;
        
        // 如果读取失败，尝试其他格式
        if (vocab_size <= 0 || embedding_dim_ <= 0) {
            std::cerr << "尝试其他模型格式..." << std::endl;
            
            // 重新打开文件
            file.close();
            file.open(model_path, std::ios::binary);
            
            // 尝试直接读取二进制格式
            if (!file.read(reinterpret_cast<char*>(&vocab_size), sizeof(int))) {
                std::cerr << "无法读取词汇量" << std::endl;
                return false;
            }
            
            if (!file.read(reinterpret_cast<char*>(&embedding_dim_), sizeof(int))) {
                std::cerr << "无法读取维度" << std::endl;
                return false;
            }
            
            // 检查字节序（如果需要）
            // 这里假设是小端字节序
        }
        
        std::cout << "加载预训练词向量模型: " << vocab_size << " 个词, 维度: " << embedding_dim_ << std::endl;
        
        // 初始化零向量和未知词向量
        zero_vector_.resize(embedding_dim_, 0.0f);
        unknown_vector_.resize(embedding_dim_, 0.0f);
        
        // 用小的随机值初始化未知词向量
        for (int i = 0; i < embedding_dim_; i++) {
            unknown_vector_[i] = (rand() % 100 - 50) / 1000.0f; // 小随机值
        }
        
        // 读取词向量
        word_vectors_.clear();
        word_vectors_.reserve(vocab_size);
        max_word_len_ = 0;
        
        std::string word;
        std::vector<float> vector(embedding_dim_);
        
        for (int i = 0; i < vocab_size; i++) {
            // 读取单词（直到空格或换行）
            word.clear();
            char c;
            while (file.get(c)) {
                if (c == ' ' || c == '\n' || c == '\t') {
                    break;
                }
                word.push_back(c);
            }
            
            if (word.empty()) {
                std::cerr << "读取单词失败，索引: " << i << std::endl;
                return false;
            }
            
            // 读取向量（二进制格式）
            file.read(reinterpret_cast<char*>(vector.data()), embedding_dim_ * sizeof(float));
            
            if (!file) {
                std::cerr << "读取向量失败，索引: " << i << std::endl;
                return false;
            }
            
            // 存储词向量
            word_vectors_[word] = vector;
            if ((int)word.length() > max_word_len_) {
                max_word_len_ = (int)word.length();
            }
            
            // 跳过可能的换行符
            if (file.peek() == '\n') {
                file.get();
            }
            
            if (i % 10000 == 0) {
                std::cout << "已加载 " << i << " / " << vocab_size << " 个词向量" << std::endl;
            }
        }
        
        file.close();
        
        std::cout << "模型加载完成，共 " << word_vectors_.size() << " 个词向量" << std::endl;
        return true;
    }
    
    // 加载简化的文本模型（为了演示）
    bool load_simple_model() {
        embedding_dim_ = 100;  // 简化模型的维度
        
        // 初始化零向量和未知词向量
        zero_vector_.resize(embedding_dim_, 0.0f);
        unknown_vector_.resize(embedding_dim_, 0.0f);
        
        // 创建一些示例词向量（实际应该从文件加载）
        std::vector<std::string> common_words;
        common_words.push_back("你好");
        common_words.push_back("问题");
        common_words.push_back("答案");
        common_words.push_back("学习");
        common_words.push_back("编程");
        common_words.push_back("系统");
        common_words.push_back("状态");
        common_words.push_back("内存");
        common_words.push_back("限制");
        common_words.push_back("如何");
        common_words.push_back("什么");
        common_words.push_back("推荐");
        common_words.push_back("今天");
        common_words.push_back("天气");
        
        for (const auto& word : common_words) {
            std::vector<float> vec(embedding_dim_);
            for (int i = 0; i < embedding_dim_; i++) {
                vec[i] = (rand() % 1000) / 1000.0f - 0.5f; // -0.5到0.5的随机值
            }
            
            // 归一化
            float norm = 0.0f;
            for (float val : vec) {
                norm += val * val;
            }
            if (norm > 0.0f) {
                norm = std::sqrt(norm);
                for (float& val : vec) {
                    val /= norm;
                }
            }
            
            word_vectors_[word] = vec;
            if ((int)word.length() > max_word_len_) {
                max_word_len_ = (int)word.length();
            }
        }
        
        std::cout << "使用简化词向量模型，维度: " << embedding_dim_ << std::endl;
        std::cout << "词汇表大小: " << word_vectors_.size() << std::endl;
        
        return true;
    }
    
    // 获取词的向量
    const std::vector<float>& get_word_vector(const std::string& word) {
        auto it = word_vectors_.find(word);
        if (it != word_vectors_.end()) {
            return it->second;
        }
        return unknown_vector_;  // 返回未知词向量
    }
    
    // 计算文本向量（词向量的平均值）
    std::vector<float> compute_text_vector(const std::string& text) {
        if (embedding_dim_ == 0) {
            return std::vector<float>();
        }
        
        std::vector<float> result(embedding_dim_, 0.0f);
        auto tokens = tokenize_chinese(text);
        
        if (tokens.empty()) {
            return zero_vector_;
        }
        
        // 累加词向量
        int valid_tokens = 0;
        for (const auto& token : tokens) {
            const auto& vec = get_word_vector(token);
            for (int i = 0; i < embedding_dim_; i++) {
                result[i] += vec[i];
            }
            valid_tokens++;
        }
        
        // 计算平均值
        if (valid_tokens > 0) {
            for (int i = 0; i < embedding_dim_; i++) {
                result[i] /= valid_tokens;
            }
        }
        
        // 归一化
        float norm = 0.0f;
        for (float val : result) {
            norm += val * val;
        }
        
        if (norm > 0.0f) {
            norm = std::sqrt(norm);
            for (float& val : result) {
                val /= norm;
            }
        }
        
        return result;
    }

public:
    Impl() : embedding_dim_(0), max_word_len_(0), initialized_(false) {}
    
    ~Impl() {
        release();
    }
    
    bool initialize(const std::string& model_path) {
        if (initialized_) {
            release();
        }
        
        bool success = false;
        
        if (!model_path.empty()) {
            // 尝试加载预训练模型文件
            success = load_pretrained_model(model_path);
        }
        
        if (!success) {
            // 如果加载失败或没有指定模型路径，使用简化模型
            std::cout << "使用内置简化词向量模型" << std::endl;
            success = load_simple_model();
        }
        
        initialized_ = success;
        return success;
    }
    
    std::vector<float> embed(const std::string& text) {
        if (!initialized_) {
            return std::vector<float>();
        }
        return compute_text_vector(text);
    }
    
    std::vector<std::vector<float> > embed_batch(const std::vector<std::string>& texts) {
        std::vector<std::vector<float> > results;
        
        if (!initialized_) {
            return results;
        }
        
        results.reserve(texts.size());
        for (const auto& text : texts) {
            results.push_back(compute_text_vector(text));
        }
        
        return results;
    }
    
    int get_embedding_dim() const {
        return embedding_dim_;
    }
    
    size_t get_memory_usage() const {
        size_t total = 0;
        // 词向量 map 的内存
        for (const auto& pair : word_vectors_) {
            total += pair.first.capacity();
            total += pair.second.capacity() * sizeof(float);
        }
        // map 自身的开销 (估算)
        total += word_vectors_.size() * (sizeof(std::string) + sizeof(std::vector<float>) + 32); 
        total += zero_vector_.capacity() * sizeof(float);
        total += unknown_vector_.capacity() * sizeof(float);
        total += sizeof(*this);
        return total;
    }
    
    bool is_initialized() const {
        return initialized_;
    }
    
    void release() {
        word_vectors_.clear();
        zero_vector_.clear();
        unknown_vector_.clear();
        embedding_dim_ = 0;
        max_word_len_ = 0;
        initialized_ = false;
    }
};

// TextEmbedder 公共接口实现
TextEmbedder::TextEmbedder() : impl_(new Impl()) {}

TextEmbedder::~TextEmbedder() {}

bool TextEmbedder::initialize(const std::string& model_path) {
    return impl_->initialize(model_path);
}

std::vector<float> TextEmbedder::embed(const std::string& text) {
    return impl_->embed(text);
}

std::vector<std::vector<float> > TextEmbedder::embed_batch(const std::vector<std::string>& texts) {
    return impl_->embed_batch(texts);
}

int TextEmbedder::get_embedding_dim() const {
    return impl_->get_embedding_dim();
}

size_t TextEmbedder::get_memory_usage() const {
    return impl_->get_memory_usage();
}

bool TextEmbedder::is_initialized() const {
    return impl_->is_initialized();
}

void TextEmbedder::release() {
    impl_->release();
}