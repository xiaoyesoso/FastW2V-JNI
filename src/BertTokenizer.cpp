#include "../include/BertTokenizer.h"
#include <fstream>
#include <iostream>
#include <sstream>

#include <algorithm>
#include <cctype>

bool is_punctuation(unsigned char c) {
    if (c < 128) {
        return std::ispunct(c);
    }
    return false;
}

// 检查是否是中文标点或特殊字符 (UTF-8)
bool is_chinese_punctuation(const std::string& s) {
    // 常见中文标点符号的 UTF-8 编码
    static const char* punctuations[] = {
        "。", "，", "；", "：", "？", "！", "（", "）", "【", "】", "《", "》", "“", "”", "‘", "’", "、"
    };
    for (size_t i = 0; i < sizeof(punctuations) / sizeof(punctuations[0]); ++i) {
        if (s == punctuations[i]) return true;
    }
    return false;
}

bool BertTokenizer::load_vocab(const std::string& vocab_path) {
    std::ifstream file(vocab_path);
    if (!file.is_open()) {
        std::cerr << "无法打开词表文件: " << vocab_path << std::endl;
        return false;
    }
    
    std::string line;
    int64_t id = 0;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        vocab_[line] = id++;
    }
    
    // 获取特殊字符 ID
    if (vocab_.count("[CLS]")) cls_id_ = vocab_["[CLS]"];
    if (vocab_.count("[SEP]")) sep_id_ = vocab_["[SEP]"];
    if (vocab_.count("[UNK]")) unk_id_ = vocab_["[UNK]"];
    if (vocab_.count("[PAD]")) pad_id_ = vocab_["[PAD]"];
    
    initialized_ = true;
    return true;
}

// 检查是否是控制字符
bool is_control(unsigned char c) {
    if (c == '\t' || c == '\n' || c == '\r') return false;
    if (c < 32 || c == 127) return true;
    return false;
}

std::vector<std::string> BertTokenizer::split_text(const std::string& text) {
    std::vector<std::string> tokens;
    std::string clean_text;
    
    // 预处理：转小写，过滤控制字符
    for (size_t k = 0; k < text.length(); ++k) {
        unsigned char c = (unsigned char)text[k];
        if (c == 0 || is_control(c)) {
            continue;
        }
        if (c >= 'A' && c <= 'Z') {
            clean_text += (char)(c + ('a' - 'A'));
        } else {
            clean_text += text[k];
        }
    }

    size_t i = 0;
    while (i < clean_text.length()) {
        unsigned char c = (unsigned char)clean_text[i];
        if (isspace(c)) {
            i++;
            continue;
        }

        if (is_punctuation(c)) {
            tokens.push_back(std::string(1, clean_text[i++]));
            continue;
        }

        if (c < 128) {
            // 英文数字连在一起，直到遇到空格、标点或非ASCII
            std::string s;
            while (i < clean_text.length()) {
                unsigned char cur = (unsigned char)clean_text[i];
                if (isspace(cur) || is_punctuation(cur) || cur >= 128) {
                    break;
                }
                s += clean_text[i++];
            }
            if (!s.empty()) tokens.push_back(s);
        } else {
            // 中文字符：按字符切分
            size_t char_len = 1;
            if (c >= 0xF0) char_len = 4;
            else if (c >= 0xE0) char_len = 3;
            else if (c >= 0xC0) char_len = 2;
            
            if (i + char_len > clean_text.length()) char_len = clean_text.length() - i;
            std::string utf8_char = clean_text.substr(i, char_len);
            
            // 检查是否是中文标点
            if (is_chinese_punctuation(utf8_char)) {
                tokens.push_back(utf8_char);
            } else {
                tokens.push_back(utf8_char);
            }
            i += char_len;
        }
    }
    return tokens;
}

void BertTokenizer::wordpiece_tokenize(const std::string& token, std::vector<int64_t>& ids) {
    if (vocab_.count(token)) {
        ids.push_back(vocab_[token]);
        return;
    }
    
    std::vector<int64_t> sub_ids;
    size_t start = 0;
    while (start < token.length()) {
        size_t end = token.length();
        std::string cur_substr;
        bool found = false;
        
        while (start < end) {
            std::string sub = token.substr(start, end - start);
            if (start > 0) sub = "##" + sub;
            
            if (vocab_.count(sub)) {
                cur_substr = sub;
                found = true;
                break;
            }
            end--;
        }
        
        if (!found) {
            ids.push_back(unk_id_);
            return;
        }
        
        sub_ids.push_back(vocab_[cur_substr]);
        start = end;
    }
    
    ids.insert(ids.end(), sub_ids.begin(), sub_ids.end());
}

std::vector<int64_t> BertTokenizer::tokenize(const std::string& text, size_t max_len) {
    std::vector<int64_t> ids;
    ids.push_back(cls_id_);
    
    std::vector<std::string> tokens = split_text(text);
    for (const auto& token : tokens) {
        wordpiece_tokenize(token, ids);
        if (ids.size() >= max_len - 1) break;
    }
    
    if (ids.size() < max_len) {
        ids.push_back(sep_id_);
    } else {
        ids[max_len - 1] = sep_id_;
    }
    
    // Padding
    while (ids.size() < max_len) {
        ids.push_back(pad_id_);
    }
    
    return ids;
}
