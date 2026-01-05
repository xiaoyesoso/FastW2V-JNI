#ifndef TEXT_EMBEDDER_H
#define TEXT_EMBEDDER_H

#include <vector>
#include <string>
#include <memory>

class TextEmbedder {
public:
    enum ModelType {
        MODEL_W2V,
        MODEL_BERT,
        MODEL_AUTO
    };

    TextEmbedder();
    ~TextEmbedder();
    
    bool initialize(const std::string& model_path, ModelType type = MODEL_AUTO);
    
    // 对于 BERT，可能需要额外的配置，如词表路径
    bool initialize_bert(const std::string& model_path, const std::string& vocab_path);
    
    std::vector<float> embed(const std::string& text);
    
    std::vector<std::vector<float> > embed_batch(const std::vector<std::string>& texts);
    
    int get_embedding_dim() const;
    
    size_t get_memory_usage() const;
    
    bool is_initialized() const;
    
    void release();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // TEXT_EMBEDDER_H