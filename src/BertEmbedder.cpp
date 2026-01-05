#include "../include/BertEmbedder.h"
#include "../include/BertTokenizer.h"
#include <iostream>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#include <chrono>

#define LOG_TAG "BertEmbedder"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#ifndef DISABLE_BERT

BertEmbedder::BertEmbedder() : initialized_(false), embedding_dim_(0), max_seq_len_(128) {
    tokenizer_ = std::unique_ptr<BertTokenizer>(new BertTokenizer());
}

BertEmbedder::~BertEmbedder() {}

bool BertEmbedder::initialize(const std::string& model_path, const std::string& vocab_path) {
    try {
        if (!tokenizer_->load_vocab(vocab_path)) {
            LOGE("加载词表失败: %s", vocab_path.c_str());
            return false;
        }
        
        env_ = std::unique_ptr<Ort::Env>(new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "BertEmbedder"));
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(4); // 增加线程数提高性能
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        
        LOGI("正在加载模型: %s", model_path.c_str());
        session_ = std::unique_ptr<Ort::Session>(new Ort::Session(*env_, model_path.c_str(), session_options));
        memory_info_ = std::unique_ptr<Ort::MemoryInfo>(new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)));
        
        // 获取输入/输出节点信息
        size_t num_input_nodes = session_->GetInputCount();
        size_t num_output_nodes = session_->GetOutputCount();
        
        Ort::AllocatorWithDefaultOptions allocator;
        input_node_names_allocated_.clear();
        input_node_names_.clear();
        for (size_t i = 0; i < num_input_nodes; i++) {
            auto name_ptr = session_->GetInputNameAllocated(i, allocator);
            LOGI("输入节点 [%zu]: %s", i, name_ptr.get());
            input_node_names_allocated_.push_back(std::move(name_ptr));
            input_node_names_.push_back(input_node_names_allocated_.back().get());
        }

        output_node_names_allocated_.clear();
        output_node_names_.clear();
        for (size_t i = 0; i < num_output_nodes; i++) {
            auto name_ptr = session_->GetOutputNameAllocated(i, allocator);
            output_node_names_allocated_.push_back(std::move(name_ptr));
            output_node_names_.push_back(output_node_names_allocated_.back().get());
            LOGI("输出节点 [%zu]: %s", i, output_node_names_.back());
        }
        
        // 获取输出节点维度
        auto output_node_type_info = session_->GetOutputTypeInfo(0);
        auto output_node_tensor_info = output_node_type_info.GetTensorTypeAndShapeInfo();
        auto output_shape = output_node_tensor_info.GetShape();
        embedding_dim_ = output_shape.back(); // 获取最后一个维度
        
        LOGI("模型加载成功，维度: %d", embedding_dim_);
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        LOGE("BERT 初始化失败: %s", e.what());
        return false;
    }
}

std::vector<float> BertEmbedder::embed(const std::string& text) {
    if (!initialized_) {
        LOGE("BertEmbedder 未初始化，无法执行 embed");
        return std::vector<float>();
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    try {
        // 1. 分词
        std::vector<int64_t> input_ids = tokenizer_->tokenize(text, max_seq_len_);
        if (input_ids.empty()) {
            LOGE("分词结果为空: %s", text.c_str());
            return std::vector<float>();
        }
        
        // 2. 准备 attention_mask 和 token_type_ids
        std::vector<int64_t> attention_mask(max_seq_len_, 0);
        std::vector<int64_t> token_type_ids(max_seq_len_, 0);
        
        int64_t pad_id = tokenizer_->get_pad_id();
        for (size_t i = 0; i < input_ids.size(); ++i) {
            if (input_ids[i] != pad_id) {
                attention_mask[i] = 1;
            }
        }
        
        // 3. 准备输入 Tensor
        std::vector<int64_t> input_shape;
        input_shape.push_back(1);
        input_shape.push_back((int64_t)max_seq_len_);
        
        std::vector<Ort::Value> input_tensors;
        input_tensors.reserve(input_node_names_.size());

        for (size_t i = 0; i < input_node_names_.size(); i++) {
            std::string name(input_node_names_[i]);
            
            // 更加鲁棒的名称匹配逻辑
            if (name.find("type") != std::string::npos || name.find("segment") != std::string::npos || name == "input.3") {
                // token_type_ids / segment_ids
                input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                    *memory_info_, token_type_ids.data(), token_type_ids.size(), 
                    input_shape.data(), input_shape.size()));
                LOGI("输入节点 [%zu] '%s' -> 映射为 token_type_ids", i, name.c_str());
            } else if (name.find("mask") != std::string::npos || name == "input.2") {
                // attention_mask
                input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                    *memory_info_, attention_mask.data(), attention_mask.size(), 
                    input_shape.data(), input_shape.size()));
                LOGI("输入节点 [%zu] '%s' -> 映射为 attention_mask", i, name.c_str());
            } else {
                // 默认认为是 input_ids (input.1)
                input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                    *memory_info_, input_ids.data(), input_ids.size(), 
                    input_shape.data(), input_shape.size()));
                LOGI("输入节点 [%zu] '%s' -> 映射为 input_ids", i, name.c_str());
            }
        }
        
        // 4. 运行推理
        auto output_tensors = session_->Run(Ort::RunOptions{nullptr}, input_node_names_.data(), input_tensors.data(), input_tensors.size(), output_node_names_.data(), output_node_names_.size());
        
        if (output_tensors.empty()) {
            LOGE("推理输出为空");
            return std::vector<float>();
        }

        // 5. 处理输出
        float* output_data = output_tensors[0].GetTensorMutableData<float>();
        auto output_shape = output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();

        std::vector<float> res;
        if (output_shape.size() == 3) {
            // [1, seq_len, dim]
            int seq_len = output_shape[1];
            int dim = output_shape[2];
            res.resize(dim, 0.0f);
            
            // 对于 CoROM 等句子嵌入模型，通常采用 [CLS] 位置的向量 (即 index 0)
            LOGI("使用 CLS Pooling (Index 0)");
            for (int j = 0; j < dim; ++j) {
                res[j] = output_data[j];
            }
        } else if (output_shape.size() == 2) {
            // [1, dim]
            int dim = output_shape[1];
            res.assign(output_data, output_data + dim);
        }
        
        // 6. L2 归一化
        float norm = 0;
        for (float v : res) norm += v * v;
        norm = std::sqrt(norm);
        
        // 记录原始向量的一些统计信息
        float mean = 0;
        for (float v : res) mean += v;
        mean /= res.size();

        if (norm > 1e-6) {
            for (float& v : res) v /= norm;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        LOGI("BERT 推理完成: text='%s', 耗时=%lldms, dim=%zu, norm=%f, mean=%f, first_5=[%f, %f, %f, %f, %f]", 
             text.c_str(), duration, res.size(), norm, mean, 
             res.size() > 0 ? res[0] : 0, res.size() > 1 ? res[1] : 0, 
             res.size() > 2 ? res[2] : 0, res.size() > 3 ? res[3] : 0, 
             res.size() > 4 ? res[4] : 0);
        
        return res;
    } catch (const std::exception& e) {
        LOGE("推理异常: %s", e.what());
        return std::vector<float>();
    }
}

int BertEmbedder::get_embedding_dim() const {
    return embedding_dim_;
}

size_t BertEmbedder::get_memory_usage() const {
    // 简单估算，模型文件大小约 20-30MB
    return initialized_ ? 30 * 1024 * 1024 : 0;
}

#else // DISABLE_BERT

BertEmbedder::BertEmbedder() : initialized_(false), embedding_dim_(0) {}
BertEmbedder::~BertEmbedder() {}
bool BertEmbedder::initialize(const std::string& model_path, const std::string& vocab_path) {
    std::cerr << "BERT 功能已禁用 (编译时未包含 ONNX Runtime)" << std::endl;
    return false;
}
std::vector<float> BertEmbedder::embed(const std::string& text) { return std::vector<float>(); }
int BertEmbedder::get_embedding_dim() const { return 0; }
size_t BertEmbedder::get_memory_usage() const { return 0; }

#endif
