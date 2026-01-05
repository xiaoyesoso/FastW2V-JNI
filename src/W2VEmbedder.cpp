#include "../include/W2VEmbedder.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

W2VEmbedder::W2VEmbedder() : embedding_dim_(0), max_word_len_(0), initialized_(false) {}

bool W2VEmbedder::initialize(const std::string& model_path) {
    std::ifstream file(model_path, std::ios::binary);
    if (!file.is_open()) return false;
    
    std::string header;
    std::getline(file, header);
    std::istringstream header_stream(header);
    int vocab_size = 0;
    header_stream >> vocab_size >> embedding_dim_;
    
    if (vocab_size <= 0 || embedding_dim_ <= 0) {
        file.close();
        // 尝试无头格式
        std::ifstream file2(model_path);
        std::string line;
        while (std::getline(file2, line)) {
            std::istringstream iss(line);
            std::string word;
            iss >> word;
            std::vector<float> vec;
            float val;
            while (iss >> val) vec.push_back(val);
            if (!vec.empty()) {
                if (embedding_dim_ == 0) embedding_dim_ = vec.size();
                word_vectors_[word] = vec;
                max_word_len_ = std::max(max_word_len_, (int)word.length());
            }
        }
    } else {
        for (int i = 0; i < vocab_size; ++i) {
            std::string word;
            file >> word;
            file.get(); // skip space
            std::vector<float> vec(embedding_dim_);
            file.read((char*)vec.data(), embedding_dim_ * sizeof(float));
            word_vectors_[word] = vec;
            max_word_len_ = std::max(max_word_len_, (int)word.length());
        }
    }
    
    zero_vector_.assign(embedding_dim_, 0.0f);
    initialized_ = true;
    return true;
}

std::vector<std::string> W2VEmbedder::tokenize_chinese(const std::string& text) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < text.length()) {
        if ((text[i] & 0x80) == 0) {
            if (isspace(text[i])) { i++; continue; }
            std::string token;
            while (i < text.length() && (text[i] & 0x80) == 0 && (isalnum(text[i]) || text[i] == '_')) {
                token += text[i++];
            }
            if (!token.empty()) tokens.push_back(token);
            else if (i < text.length()) i++;
            continue;
        }
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

std::vector<float> W2VEmbedder::embed(const std::string& text) {
    if (!initialized_) return std::vector<float>();
    std::vector<std::string> tokens = tokenize_chinese(text);
    if (tokens.empty()) return zero_vector_;
    
    std::vector<float> res(embedding_dim_, 0.0f);
    int count = 0;
    for (const auto& token : tokens) {
        if (word_vectors_.count(token)) {
            const auto& vec = word_vectors_[token];
            for (int i = 0; i < embedding_dim_; ++i) res[i] += vec[i];
            count++;
        }
    }
    
    if (count > 0) {
        float norm = 0;
        for (int i = 0; i < embedding_dim_; ++i) {
            res[i] /= count;
            norm += res[i] * res[i];
        }
        norm = std::sqrt(norm);
        if (norm > 1e-6) {
            for (int i = 0; i < embedding_dim_; ++i) res[i] /= norm;
        }
    }
    return res;
}

size_t W2VEmbedder::get_memory_usage() const {
    size_t total = 0;
    for (const auto& pair : word_vectors_) {
        total += pair.first.capacity();
        total += pair.second.capacity() * sizeof(float);
        total += 32; // map node overhead approx
    }
    return total;
}
