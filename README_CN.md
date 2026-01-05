# FastW2V-JNI

[English Version](README.md)

高性能 C++ 语义搜索与问答引擎，支持 **Word2Vec** 和 **BERT (ONNX)** 双引擎切换。专为移动端 (Android) 和嵌入式设备设计，实现毫秒级的语义匹配。

## 🚀 项目特点

- ✅ **双引擎支持**：支持传统 Word2Vec 词向量和现代 BERT (CoROM) 深度学习模型。
- ✅ **极致性能**：采用原生 C++17 实现，Word2Vec 搜索耗时 <1ms，BERT 推理耗时低。
- ✅ **跨平台 JNI**：提供完善的 Java Native Interface，轻松集成至 Android 或 Java 项目。
- ✅ **完全离线**：支持端侧部署，无需联网，内存占用可控。
- ✅ **工业级对齐**：BERT 引擎使用 `[CLS]` 向量作为句子表示，完美对齐 ModelScope CoROM 池化策略。
- ✅ **标准相似度**：采用标准余弦相似度计算匹配得分（-1 到 1），结果分布可预测。

## 📂 项目结构

```text
.
├── src/                # 核心 C++ 源代码
│   ├── BertEmbedder.cpp    # BERT ONNX 推理实现
│   ├── BertTokenizer.cpp   # BERT WordPiece 分词器
│   ├── SimilaritySearch.cpp # 向量搜索与余弦相似度计算
│   ├── TextEmbedder.cpp    # 嵌入器基类
│   └── W2VEmbedder.cpp     # Word2Vec 词嵌入实现
├── include/            # C++ 头文件
│   ├── BertEmbedder.h      # BERT 引擎头文件
│   ├── BertTokenizer.h     # 分词器头文件
│   ├── SimilaritySearch.h  # 搜索引擎头文件
│   ├── TextEmbedder.h      # 嵌入器基类接口
│   ├── W2VEmbedder.h       # Word2Vec 引擎头文件
│   ├── W2VEngine.h         # 复合引擎 (JNI 包装类)
│   └── com_example_w2v_W2VNative.h # JNI 生成的头文件
├── jni/                # JNI 接口层
│   ├── CMakeLists.txt      # NDK 编译配置
│   ├── W2VNative.java      # Java 原生方法定义 (参考)
│   └── com_example_w2v_W2VNative.cpp # C++ JNI 实现
├── android_test/       # Android 平台示例
│   ├── w2v_version/    # Word2Vec 集成示例 (完整 App)
│   └── bert_version/   # BERT (ONNX Runtime) 集成示例 (完整 App)
├── scripts/            # Python 工具脚本
│   └── convert_model.py    # 模型转换与导出脚本
├── data/               # 示例问答数据
│   └── qa_list.csv         # 默认问答知识库
└── CMakeLists.txt      # 根目录 CMake 编译配置
```

## 🛠️ 快速开始

### 1. 模型准备 (Model Preparation)

本项目支持两种引擎，模型获取与转换步骤如下：

#### **BERT (CoROM-Tiny) 导出为 ONNX**
1. **下载模型**：
   - 来源：[ModelScope - CoROM 句子向量 (Chinese-Tiny)](https://www.modelscope.cn/models/iic/nlp_corom_sentence-embedding_chinese-tiny)
   - 下述脚本将自动处理下载。
2. **环境准备**：
   ```bash
   pip install torch transformers modelscope onnx
   ```
2. **执行导出脚本**：
   使用提供的脚本从 ModelScope 下载模型并导出为 ONNX 格式：
   ```bash
   python scripts/convert_model.py
   ```
   脚本会自动：
   - 从 ModelScope 下载 `iic/nlp_corom_sentence-embedding_chinese-tiny`。
   - 提取 `[CLS]` 向量作为句子表示。
   - 导出 `model.onnx` 并提取 `vocab.txt` 到 `export/` 目录。

#### **Word2Vec (Tencent AILab) 获取与压缩**
1. **下载原始模型**：
   - 来源：[Hugging Face - 腾讯 AI Lab 中文词向量 (Light)](https://huggingface.co/alextomcat/light_Tencent_AILab_ChineseEmbedding)
   - 下载预训练的中文词向量（推荐下载 `light` 版本）。
2. **转换为二进制格式**：
   本项目使用的 `.bin` 格式为：首行 `vocab_size dim`，后续每行为 `word` + `二进制 float 向量`。
   你可以使用简单的 Python 脚本进行转换：
   ```python
   # 示例：将 txt 格式转换为二进制 .bin
   import struct
   with open('input.txt', 'r') as f, open('output.bin', 'wb') as f_out:
       header = f.readline()
       f_out.write(header.encode('utf-8')) # 写入头信息 (txt)
       for line in f:
           parts = line.strip().split()
           word = parts[0]
           vec = [float(x) for x in parts[1:]]
           f_out.write(f"{word} ".encode('utf-8'))
           f_out.write(struct.pack(f'{len(vec)}f', *vec))
   ```

### 2. 初始化引擎
```java
// Word2Vec 模式
long enginePtr = W2VNative.initEngine(modelPath);

// BERT 模式
long enginePtr = W2VNative.initBertEngine(onnxPath, vocabPath);
```

### 2. 加载数据并搜索
```java
// 加载 QA 知识库 (CSV 格式)
W2VNative.loadQAFromFile(enginePtr, "qa_list.csv");

// 执行语义搜索
W2VNative.SearchResult result = W2VNative.search(enginePtr, "系统如何重启");

if (result != null) {
    System.out.println("匹配问题: " + result.question);
    System.out.println("置信度 (Cos): " + result.score);
}
```

## 📱 Android 集成指南

### 1. 复制资源与库
- **JNI 库**: 复制 `libw2v_jni.so` 到 `app/src/main/jniLibs/arm64-v8a/`。
- **Java 接口**: 复制 `W2VNative.java` 到对应的包路径。
- **模型**: 将模型和词表放入 `assets` 目录。

### 2. 项目配置 (build.gradle)
```gradle
android {
    defaultConfig {
        ndk { abiFilters 'arm64-v8a' }
    }
}
dependencies {
    // 仅 BERT 模式需要
    implementation 'com.microsoft.onnxruntime:onnxruntime-android:latest.release'
}
```

### 3. 代码示例
由于 Android 无法直接访问 assets 路径，需先将模型复制到私有目录：
```java
String modelPath = context.getFilesDir().getPath() + "/model.onnx";
// ... 复制 assets 到 modelPath ...
long enginePtr = W2VNative.initBertEngine(modelPath, vocabPath);
```

## 💡 优化与故障排除

- **内存管理**：模型加载后常驻内存（W2V 约 120MB，BERT 约 30MB）。建议在 `onDestroy` 中显式调用 `releaseEngine`。
- **线程安全**：JNI 调用不应在 UI 线程执行，建议使用后台线程。
- **常见问题**：
    - `UnsatisfiedLinkError`：检查 `abiFilters` 是否包含 `arm64-v8a`，且库路径正确。
    - 模型加载失败：检查文件路径是否为绝对路径，且具备读取权限。

## 📊 性能与指标 (Android ARM64)

| 引擎 | 模型大小 | 内存占用 | 单次搜索耗时 |
| :--- | :--- | :--- | :--- |
| **Word2Vec** | ~50MB | ~120MB | < 0.2 ms |
| **BERT (Tiny)** | ~33MB | ~30MB | ~100 ms |

> **注意**：BERT 引擎依赖 `onnxruntime-android`，Word2Vec 引擎无外部依赖。

## 📄 许可证

本项目遵循 MIT 协议开源。仅供学习与技术研究使用。
