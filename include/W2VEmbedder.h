#ifndef W2V_EMBEDDER_H
#define W2V_EMBEDDER_H

#include <vector>
#include <string>
#include <unordered_map>

class W2VEmbedder {
public:
    W2VEmbedder();
    bool initialize(const std::string& model_path);
    std::vector<float> embed(const std::string& text);
    int get_embedding_dim() const { return embedding_dim_; }
    size_t get_memory_usage() const;
    bool is_initialized() const { return initialized_; }

private:
    std::unordered_map<std::string, std::vector<float> > word_vectors_;
    int embedding_dim_;
    int max_word_len_;
    bool initialized_;
    std::vector<float> zero_vector_;
    
    std::vector<std::string> tokenize_chinese(const std::string& text);
};

#endif // W2V_EMBEDDER_H
