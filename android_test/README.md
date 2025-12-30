# W2V Android 语义搜索 SDK 集成文档

本 SDK 提供基于 Word2Vec 的语义搜索功能，支持本地离线运行，主要用于 Android 端的问答匹配（QA Matching）和文本相似度计算。

## 1. 项目结构与核心文件

为了方便集成，请确保将以下文件放置在 Android 项目的对应目录下：

| 文件类型 | 文件路径 (Android Project) | 说明 |
| :--- | :--- | :--- |
| **JNI 库 (64位)** | `app/src/main/jniLibs/arm64-v8a/libw2v_jni.so` | 核心计算引擎 (ARM64) |
| **JNI 库 (32位)** | `app/src/main/jniLibs/armeabi-v7a/libw2v_jni.so` | 兼容旧型设备 (ARMv7) |
| **Java 接口** | `app/src/main/java/com/example/w2v/W2VNative.java` | JNI 方法定义与核心 API |
| **模型文件** | `app/src/main/assets/light_Tencent_AILab_ChineseEmbedding.bin` | 预训练词向量模型 |
| **QA 数据** | `app/src/main/assets/qa_list.csv` | 问答知识库 (CSV 格式) |

> **注意：** 本库已采用 **静态链接 C++ 标准库** (`libc++_static`)，集成时**不需要**额外提供 `libc++_shared.so`。

## 2. 快速集成步骤

### 第一步：配置 NDK 库加载
在 `app/build.gradle` 中确保配置了 `jniLibs` 路径，并建议指定 ABI 过滤：
```gradle
android {
    ...
    defaultConfig {
        ...
        ndk {
            // 根据需要选择支持的架构
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
    sourceSets {
        main {
            jniLibs.srcDirs = ['src/main/jniLibs']
        }
    }
}
```

### 第二步：自定义包名（可选）
如果你的项目包名不是 `com.example.w2v`，你需要重新构建 JNI 库以匹配你的 Java 类路径：

1. 修改 `W2VNative.java` 的 `package` 声明为你项目的实际包名。
2. 在 `android_test` 目录下运行构建脚本，并传入你的完整类名路径：
   ```bash
   # 格式：./build_android.sh "包名/类名"
   ./build_android.sh "com/binx/w2vexample/W2VNative"
   ```
3. 脚本会自动重新编译 `arm64-v8a` 和 `armeabi-v7a` 的 `.so` 文件并输出到 `app/src/main/jniLibs`。

### 第三步：使用 W2VNative 进行搜索
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
- `getMemoryUsage(enginePtr)`: 获取当前引擎占用的内存 (单位: Bytes)。
- `releaseEngine(enginePtr)`: 销毁引擎，释放原生内存。**不再使用时务必调用**。

## 4. 技术规范与注意事项

- **架构支持**: 同时提供 `arm64-v8a` (64位) 和 `armeabi-v7a` (32位) 架构支持，覆盖 99% 以上的 Android 设备。
- **静态链接**: 已集成 `libc++_static`，彻底解决 `libc++_shared.so not found` 报错问题。
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

## 5. Android 仿真测试 (模拟器/真机)

为了验证 `libw2v_jni.so` 在真实 Android 环境下的表现，你可以按照以下步骤进行仿真测试：

### 5.1 自动化仿真流程
我们提供了一个演示 App (位于 `android_test` 目录)，它模拟了真实的业务调用流程：
1. **安装并启动**: 在 Android Studio 中运行项目，或使用 `adb install` 安装生成的 APK。
2. **加载模型**: 点击界面上的 **"LOAD QA FROM ASSETS"** 按钮。这会触发底层引擎初始化及 1500+ 条 QA 数据的 Embedding 计算。
3. **执行搜索**: 点击 **"TEST BATCH SEARCH"** 按钮。这会模拟批量搜索请求，并输出性能及内存占用日志。

### 5.2 验证结果 (Logcat)
在测试过程中，可以通过 Android Logcat 查看详细的运行日志。建议使用以下过滤命令：
```bash
adb logcat -s AndroidJavaTest
```

**预期输出示例**:
```text
I/AndroidJavaTest: --- W2V 性能与内存测试开始 ---
I/AndroidJavaTest: [1/3] 加载状态: 成功, QA总数: 1534
I/AndroidJavaTest: [2/3] 内存占用: 28.47 MB
I/AndroidJavaTest: [3/3] 批量搜索完成 (10条查询)
I/AndroidJavaTest:     - 查询: '你好', 结果: '你好！', 耗时: 1.2ms
I/AndroidJavaTest:     - 查询: '怎么重置密码', 结果: '点击设置-安全-重置密码', 耗时: 0.8ms
...
I/AndroidJavaTest: --- 测试全部通过 ---
```