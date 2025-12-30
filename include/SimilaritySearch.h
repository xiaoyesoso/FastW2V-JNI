#ifndef SIMILARITY_SEARCH_H
#define SIMILARITY_SEARCH_H

#include <vector>
#include <string>
#include <memory>
#include <utility>

struct SearchResult {
    std::string question;
    std::string answer;
    float similarity;
    
    SearchResult(const std::string& q, const std::string& a, float s)
        : question(q), answer(a), similarity(s) {}
};

class SimilaritySearch {
public:
    SimilaritySearch();
    ~SimilaritySearch();
    
    bool initialize(int embedding_dim);
    
    bool add_qa(const std::string& question, const std::string& answer, const std::vector<float>& embedding);
    
    bool add_qa_batch(const std::vector<std::string>& questions,
                     const std::vector<std::string>& answers,
                     const std::vector<std::vector<float> >& embeddings);
    
    SearchResult search(const std::vector<float>& query_embedding, int top_k = 1);
    
    std::vector<SearchResult> search_batch(const std::vector<std::vector<float> >& query_embeddings, int top_k = 1);
    
    size_t size() const;
    
    void clear();
    
    void optimize();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // SIMILARITY_SEARCH_H