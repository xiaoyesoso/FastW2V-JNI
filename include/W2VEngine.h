#ifndef W2V_ENGINE_H
#define W2V_ENGINE_H

#include "TextEmbedder.h"
#include "SimilaritySearch.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>

class W2VEngine {
public:
    W2VEngine() : embedder_(new TextEmbedder()), searcher_(new SimilaritySearch()) {}

    bool initialize(const std::string& model_path) {
        return embedder_->initialize(model_path);
    }

    bool initialize_bert(const std::string& model_path, const std::string& vocab_path) {
        return embedder_->initialize_bert(model_path, vocab_path);
    }

    bool load_qa_from_file(const std::string& file_path) {
        if (!embedder_->is_initialized()) return false;
        if (!searcher_->initialize(embedder_->get_embedding_dim())) return false;

        std::ifstream file(file_path);
        if (!file.is_open()) return false;

        std::string line;
        std::vector<std::string> questions, answers;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            size_t pos = line.find(',');
            if (pos != std::string::npos) {
                questions.push_back(line.substr(0, pos));
                answers.push_back(line.substr(pos + 1));
            }
        }
        
        if (questions.empty()) return true;

        auto embeddings = embedder_->embed_batch(questions);
        return searcher_->add_qa_batch(questions, answers, embeddings);
    }

    bool load_qa_from_memory(const std::vector<std::string>& questions, const std::vector<std::string>& answers) {
        if (!embedder_->is_initialized()) return false;
        if (!searcher_->initialize(embedder_->get_embedding_dim())) return false;
        
        auto embeddings = embedder_->embed_batch(questions);
        return searcher_->add_qa_batch(questions, answers, embeddings);
    }

    std::pair<std::string, std::string> search(const std::string& query, float* similarity) {
        auto embedding = embedder_->embed(query);
        auto result = searcher_->search(embedding);
        if (similarity) *similarity = result.similarity;
        return std::make_pair(result.question, result.answer);
    }

    std::vector<std::pair<std::string, std::string> > search_batch(const std::vector<std::string>& queries, std::vector<float>* similarities) {
        auto embeddings = embedder_->embed_batch(queries);
        auto results = searcher_->search_batch(embeddings);
        
        std::vector<std::pair<std::string, std::string> > final_results;
        if (similarities) similarities->clear();
        
        for (const auto& res : results) {
            final_results.push_back(std::make_pair(res.question, res.answer));
            if (similarities) similarities->push_back(res.similarity);
        }
        return final_results;
    }

    size_t get_qa_count() const { return searcher_->size(); }
    int get_embedding_dim() const { return embedder_->get_embedding_dim(); }
    size_t get_memory_usage() const { return embedder_->get_memory_usage(); }
    void release() { embedder_->release(); searcher_->clear(); }

private:
    std::unique_ptr<TextEmbedder> embedder_;
    std::unique_ptr<SimilaritySearch> searcher_;
};

#endif
