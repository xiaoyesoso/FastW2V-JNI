# W2V Android 语义搜索 SDK 集成文档 (Word2Vec 版)

[English Version](README.md)

本 SDK 提供基于 Word2Vec 的语义搜索功能，支持本地离线运行，主要用于 Android 端的问答匹配（QA Matching）和文本相似度计算。

## 🚀 核心特性

- **极致速度**：纯 C++ 实现，单次搜索耗时 < 0.2ms。
- **余弦相似度得分**：返回标准余弦相似度（-1.0 到 1.0），真实反映匹配度。
- **无外部依赖**：JNI 库独立运行，无需安装额外的 Runtime。

## 1. 项目结构与核心文件

| 文件类型 | 文件路径 (Android Project) | 说明 |
| :--- | :--- | :--- |
| **JNI 库** | `app/src/main/jniLibs/arm64-v8a/libw2v_jni.so` | 核心计算引擎 |
| **Java 接口** | `app/src/main/java/com/example/w2v/W2VNative.java` | JNI 方法定义 |
| **模型文件** | `app/src/main/assets/light_Tencent_AILab_ChineseEmbedding.bin` | 预训练模型 |
| **QA 数据** | `app/src/main/assets/qa_list.csv` | 问答知识库 |

## 2. 快速集成步骤

### 第一步：配置 NDK 库加载
在 `app/build.gradle` 中确保配置了 `jniLibs` 路径：
```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### 第二步：初始化与搜索
```java
// 1. 初始化引擎 (建议在后台线程执行)
long enginePtr = W2VNative.initEngine(modelPath);

if (enginePtr != 0) {
    // 2. 加载 QA 数据
    W2VNative.loadQAFromFile(enginePtr, qaPath);
    
    // 3. 执行搜索
    W2VNative.SearchResult result = W2VNative.search(enginePtr, "你好");
    Log.d("W2V", "相似度: " + result.score); // 范围: -1.0 ~ 1.0
}
```

## 3. 技术规范

- **架构支持**: `arm64-v8a`, `armeabi-v7a`。
- **相似度算法**: 余弦相似度 (Cosine Similarity)。
- **内存占用**: 约 120MB (取决于模型大小)。

## 4. 仿真测试
运行 `./run_android_sim_test.sh` 即可启动自动化测试。
