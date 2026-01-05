#include "../include/TextEmbedder.h"
#include "../include/W2VEmbedder.h"
#include "../include/BertEmbedder.h"
#include <memory>
#include <algorithm>

#include <android/log.h>
#define LOG_TAG "TextEmbedder"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class TextEmbedder::Impl {
public:
    std::unique_ptr<W2VEmbedder> w2v_ptr;
    std::unique_ptr<BertEmbedder> bert_ptr;
    bool is_bert = false;

    bool initialize(const std::string& model_path, ModelType type) {
        LOGI("初始化 Embedder: path=%s, type=%d", model_path.c_str(), type);
        if (type == MODEL_AUTO) {
            if (model_path.find(".onnx") != std::string::npos) {
                type = MODEL_BERT;
            } else {
                type = MODEL_W2V;
            }
        }

        if (type == MODEL_BERT) {
            LOGI("选择 BERT 引擎");
            bert_ptr = std::unique_ptr<BertEmbedder>(new BertEmbedder());
            std::string vocab_path;
            size_t last_slash = model_path.find_last_of("/\\");
            if (last_slash != std::string::npos) {
                vocab_path = model_path.substr(0, last_slash + 1) + "vocab.txt";
            } else {
                vocab_path = "vocab.txt";
            }
            if (bert_ptr->initialize(model_path, vocab_path)) {
                is_bert = true;
                return true;
            }
            return false;
        } else {
            LOGI("选择 Word2Vec 引擎");
            w2v_ptr = std::unique_ptr<W2VEmbedder>(new W2VEmbedder());
            if (w2v_ptr->initialize(model_path)) {
                is_bert = false;
                return true;
            }
            return false;
        }
    }

    bool initialize_bert(const std::string& model_path, const std::string& vocab_path) {
        LOGI("强制初始化 BERT: model=%s, vocab=%s", model_path.c_str(), vocab_path.c_str());
        bert_ptr = std::unique_ptr<BertEmbedder>(new BertEmbedder());
        if (bert_ptr->initialize(model_path, vocab_path)) {
            is_bert = true;
            return true;
        }
        return false;
    }

    std::vector<float> embed(const std::string& text) {
        if (is_bert) {
            if (bert_ptr) {
                return bert_ptr->embed(text);
            } else {
                LOGI("错误: is_bert=true 但 bert_ptr 为空");
            }
        } else {
            if (w2v_ptr) {
                return w2v_ptr->embed(text);
            } else {
                LOGI("错误: is_bert=false 但 w2v_ptr 为空");
            }
        }
        return std::vector<float>();
    }

    int get_embedding_dim() const {
        if (is_bert && bert_ptr) return bert_ptr->get_embedding_dim();
        if (!is_bert && w2v_ptr) return w2v_ptr->get_embedding_dim();
        return 0;
    }

    size_t get_memory_usage() const {
        if (is_bert && bert_ptr) return bert_ptr->get_memory_usage();
        if (!is_bert && w2v_ptr) return w2v_ptr->get_memory_usage();
        return 0;
    }

    bool is_initialized() const {
        if (is_bert) return bert_ptr && bert_ptr->is_initialized();
        return w2v_ptr && w2v_ptr->is_initialized();
    }
};

TextEmbedder::TextEmbedder() : impl_(std::unique_ptr<Impl>(new Impl())) {}
TextEmbedder::~TextEmbedder() = default;

bool TextEmbedder::initialize(const std::string& model_path, ModelType type) {
    return impl_->initialize(model_path, type);
}

bool TextEmbedder::initialize_bert(const std::string& model_path, const std::string& vocab_path) {
    return impl_->initialize_bert(model_path, vocab_path);
}

std::vector<float> TextEmbedder::embed(const std::string& text) {
    return impl_->embed(text);
}

std::vector<std::vector<float> > TextEmbedder::embed_batch(const std::vector<std::string>& texts) {
    std::vector<std::vector<float> > results;
    for (const auto& text : texts) {
        results.push_back(embed(text));
    }
    return results;
}

int TextEmbedder::get_embedding_dim() const {
    return impl_->get_embedding_dim();
}

size_t TextEmbedder::get_memory_usage() const {
    return impl_->get_memory_usage();
}

bool TextEmbedder::is_initialized() const {
    return impl_->is_initialized();
}

void TextEmbedder::release() {
    impl_ = std::unique_ptr<Impl>(new Impl());
}
