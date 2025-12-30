#ifndef W2V_MAIN_H
#define W2V_MAIN_H

#include <string>
#include <vector>
#include <memory>

class W2VEngine {
public:
    W2VEngine();
    ~W2VEngine();
    
    bool initialize(const std::string& model_path);
    
    bool load_qa_from_file(const std::string& file_path);
    
    bool load_qa_from_memory(const std::vector<std::pair<std::string, std::string> >& qa_pairs);
    
    std::pair<std::string, std::string> search(const std::string& query, float* similarity_score = nullptr);
    
    std::vector<std::pair<std::string, std::string> > search_batch(const std::vector<std::string>& queries,
                                                                 std::vector<float>* similarity_scores = nullptr);
    
    size_t get_qa_count() const;
    
    int get_embedding_dim() const;
    
    size_t get_memory_usage() const;
    
    void release();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // W2V_MAIN_H