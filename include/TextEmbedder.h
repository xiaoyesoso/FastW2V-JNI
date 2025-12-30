#ifndef TEXT_EMBEDDER_H
#define TEXT_EMBEDDER_H

#include <vector>
#include <string>
#include <memory>

class TextEmbedder {
public:
    TextEmbedder();
    ~TextEmbedder();
    
    bool initialize(const std::string& model_path);
    
    std::vector<float> embed(const std::string& text);
    
    std::vector<std::vector<float> > embed_batch(const std::vector<std::string>& texts);
    
    int get_embedding_dim() const;
    
    bool is_initialized() const;
    
    void release();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // TEXT_EMBEDDER_H