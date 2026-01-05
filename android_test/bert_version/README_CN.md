# W2V Android 语义搜索 SDK 集成文档 (BERT 版)

本 SDK 提供基于 **Chinese BERT (CoROM-Tiny)** 的深度学习语义搜索功能。相比传统的 Word2Vec，BERT 能更好地理解上下文语义，提供更高质量的文本匹配结果。

## 🚀 核心特性

- **深度语义理解**：基于 transformer 架构，支持复杂的上下文匹配。
- **工业级对齐**：采用 CLS Pooling 策略，完美对齐 ModelScope 预训练模型。
- **原始得分**：返回原始余弦相似度（-1.0 到 1.0），真实反映匹配度。

## 1. 项目结构与核心文件

| 文件类型 | 文件路径 (Android Project) | 说明 |
| :--- | :--- | :--- |
| **JNI 库** | `app/src/main/jniLibs/arm64-v8a/libw2v_jni.so` | 核心引擎 |
| **Java 接口** | `app/src/main/java/com/example/w2v/W2VNative.java` | JNI 方法定义 |
| **BERT 模型** | `app/src/main/assets/model.onnx` | ONNX 格式模型 |
| **词表文件** | `app/src/main/assets/vocab.txt` | Tokenizer 词表 |

## 2. 快速集成步骤

### 第一步：添加 ONNX Runtime 依赖
BERT 引擎依赖微软的 ONNX Runtime。请在 `app/build.gradle` 中添加：
```gradle
dependencies {
    implementation 'com.microsoft.onnxruntime:onnxruntime-android:latest.release'
}
```

### 第二步：配置 NDK 库加载
```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### 第三步：初始化与搜索
```java
// 1. 初始化 BERT 引擎 (建议在后台线程执行)
long enginePtr = W2VNative.initBertEngine(onnxPath, vocabPath);

if (enginePtr != 0) {
    // 2. 加载 QA 数据
    W2VNative.loadQAFromFile(enginePtr, qaPath);
    
    // 3. 执行搜索
    W2VNative.SearchResult result = W2VNative.search(enginePtr, "系统如何重启");
    System.out.println("相似度: " + result.score); // 范围: -1.0 ~ 1.0
}
```

## 3. 技术规范

- **模型架构**: CoROM-Tiny (256维)。
- **推理后端**: ONNX Runtime (CPU)。
- **性能**: ARM64 设备单次推理约 100ms。

## 4. 仿真测试
运行 `./run_android_sim_test.sh` 即可启动自动化测试。
