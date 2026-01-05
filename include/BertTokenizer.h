#ifndef BERT_TOKENIZER_H
#define BERT_TOKENIZER_H

#include <string>
#include <vector>
#include <unordered_map>

class BertTokenizer {
public:
    BertTokenizer() : initialized_(false) {}
    
    bool load_vocab(const std::string& vocab_path);
    
    std::vector<int64_t> tokenize(const std::string& text, size_t max_len);
    
    bool is_initialized() const { return initialized_; }
    int64_t get_pad_id() const { return pad_id_; }

private:
    std::unordered_map<std::string, int64_t> vocab_;
    bool initialized_;
    
    int64_t cls_id_ = 101;
    int64_t sep_id_ = 102;
    int64_t unk_id_ = 100;
    int64_t pad_id_ = 0;

    std::vector<std::string> split_text(const std::string& text);
    void wordpiece_tokenize(const std::string& token, std::vector<int64_t>& ids);
};

#endif // BERT_TOKENIZER_H
