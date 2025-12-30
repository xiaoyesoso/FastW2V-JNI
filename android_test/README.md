# W2V Android 语义搜索 SDK 集成文档

本 SDK 提供基于 Word2Vec 的语义搜索功能，支持本地离线运行，主要用于 Android 端的问答匹配（QA Matching）和文本相似度计算。

## 1. 项目结构与核心文件

为了方便集成，请确保将以下文件放置在 Android 项目的对应目录下：

| 文件类型 | 文件路径 (Android Project) | 说明 |
| :--- | :--- | :--- |
| **JNI 库** | `app/src/main/jniLibs/arm64-v8a/libw2v_jni.so` | 核心计算引擎 (ARM64) |
| **Java 接口** | `app/src/main/java/com/example/w2v/W2VNative.java` | JNI 方法定义与核心 API |
| **模型文件** | `app/src/main/assets/light_Tencent_AILab_ChineseEmbedding.bin` | 预训练词向量模型 |
| **QA 数据** | `app/src/main/assets/qa_list.csv` | 问答知识库 (CSV 格式) |

## 2. 快速集成步骤

### 第一步：配置 NDK 库加载
在 `app/build.gradle` 中确保配置了 `jniLibs` 路径（通常默认已包含）：
```gradle
android {
    ...
    sourceSets {
        main {
            jniLibs.srcDirs = ['src/main/jniLibs']
        }
    }
}
```

### 第二步：使用 W2VNative 进行搜索
直接使用 `W2VNative` 提供的静态方法。由于 Android 无法直接访问 Assets 路径，需要先将资源复制到应用的私有存储目录。

**重要建议：** 为了实现毫秒级搜索响应，请务必在应用启动或进入相关模块时**预先初始化**。初始化会将模型和所有问题的向量（Embedding）一次性加载并常驻内存，避免在每次搜索时产生重复加载的开销。

#### 场景 A：从文件加载 QA 数据 (CSV)
```java
// 1. 初始化引擎 (必须在后台线程执行)
new Thread(() -> {
    // modelPath 和 qaPath 是通过 Assets 复制到 getFilesDir() 的绝对路径
    long enginePtr = W2VNative.initEngine(modelPath);
    if (enginePtr != 0) {
        boolean success = W2VNative.loadQAFromFile(enginePtr, qaPath);
        if (success) {
            // 2. 执行搜索
            W2VNative.SearchResult result = W2VNative.search(enginePtr, "你好");
        }
    }
}).start();
```

#### 场景 B：从内存加载 QA 数据 (如数据库查询结果)
如果你想从数据库或其他动态来源加载 QA 对，可以使用 `loadQAFromMemory`：
```java
new Thread(() -> {
    long enginePtr = W2VNative.initEngine(modelPath);
    if (enginePtr != 0) {
        String[] questions = {"你好", "你是谁"};
        String[] answers = {"你好！", "我是 AI 助手"};
        
        boolean success = W2VNative.loadQAFromMemory(enginePtr, questions, answers);
        if (success) {
            W2VNative.SearchResult result = W2VNative.search(enginePtr, "你好");
        }
    }
}).start();
```

## 3. 核心 API 说明 (W2VNative)

- `initEngine(modelPath)`: 初始化引擎并加载模型。返回 `enginePtr` (long)。
- `loadQAFromFile(enginePtr, filePath)`: 从 CSV 文件加载 QA 知识库。
- `loadQAFromMemory(enginePtr, questions, answers)`: 从内存加载 QA 数据（Java 字符串数组）。
- `search(enginePtr, query)`: 执行单次搜索，返回最高分结果 `SearchResult`。
- `searchBatch(enginePtr, queries)`: 批量搜索，返回 `SearchResult[]`。
- `getQACount(enginePtr)`: 获取已加载的 QA 总数。
- `getEmbeddingDim(enginePtr)`: 获取向量维度。
- `getMemoryUsage(enginePtr)`: 获取当前引擎占用的内存 (单位: MB)。
- `releaseEngine(enginePtr)`: 销毁引擎，释放原生内存。**不再使用时务必调用**。

## 4. 技术规范与注意事项

- **架构支持**: 目前仅提供 `arm64-v8a` 架构的 `.so` 库。
- **匹配逻辑**: 仅对 `query` 和 `question` 进行向量化匹配，`answer` 仅作为结果返回，不参与向量计算。
- **线程安全**: 底层引擎支持并发搜索，建议全局持有一个 `enginePtr`。
- **内存消耗与性能**: 
  - **预加载**: `loadQA` 过程会完成所有问题文本的向量化（Embedding）并缓存。由于这涉及到大规模浮点运算和模型加载，建议在后台静默完成。
  - **常驻内存**: 加载后的模型和向量数据会常驻内存，以确保搜索请求能立即得到响应（纯内存计算）。
  - **监控**: 可以通过 `W2VNative.getMemoryUsage(enginePtr)` 实时监控内存状态。
- **混淆配置**: 如果开启了 Proguard，请保留 JNI 接口类：
  ```proguard
  -keep class com.example.w2v.** { *; }
  -keepclasseswithmembernames class com.example.w2v.** {
      native <methods>;
  }
  ```

## 5. 本地仿真测试 (macOS)
在交给 Android 同事之前，你可以运行以下脚本在本地模拟 Android 环境测试逻辑：
```bash
./run_local_test.sh
```
该脚本会自动连接 `linux_java_test` 中的真实引擎进行准确度验证。