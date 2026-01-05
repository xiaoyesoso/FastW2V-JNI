#ifndef BERT_EMBEDDER_H
#define BERT_EMBEDDER_H

#include <string>
#include <vector>
#include <memory>

#ifndef DISABLE_BERT
#include "onnxruntime_cxx_api.h"
#endif

class BertEmbedder {
public:
    BertEmbedder();
    ~BertEmbedder();
    
    bool initialize(const std::string& model_path, const std::string& vocab_path);
    
    std::vector<float> embed(const std::string& text);
    int get_embedding_dim() const;
    size_t get_memory_usage() const;
    bool is_initialized() const { return initialized_; }

private:
    bool initialized_;
    int embedding_dim_;
    
#ifndef DISABLE_BERT
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::MemoryInfo> memory_info_;
    std::unique_ptr<class BertTokenizer> tokenizer_;
    
    std::vector<const char*> input_node_names_;
    std::vector<const char*> output_node_names_;
    std::vector<Ort::AllocatedStringPtr> input_node_names_allocated_;
    std::vector<Ort::AllocatedStringPtr> output_node_names_allocated_;
    size_t max_seq_len_ = 128;
#endif
};

#endif // BERT_EMBEDDER_H
