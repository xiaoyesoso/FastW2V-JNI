# FastW2V-JNI

高性能 C++ Word2Vec 语义搜索与问答引擎，专为移动端 (Android) 和服务端 (Linux) 设计。基于腾讯 AI Lab 预训练词向量，实现毫秒级的语义匹配。

## 🚀 项目特点

- ✅ **极致性能**：采用原生 C++17 实现，核心搜索耗时 <1ms。
- ✅ **跨平台 JNI**：提供完善的 Java Native Interface，轻松集成至 Android 或 Java 项目。
- ✅ **端侧部署**：支持完全离线运行，内存占用优化至 300MB 左右。
- ✅ **灵活加载**：支持从 CSV 文件或内存数组加载问答知识库。
- ✅ **中文优化**：针对中文语境优化，支持多种相似度计算逻辑。

## 📂 项目结构

```text
w2v/
├── src/                    # 核心 C++ 源代码
│   ├── TextEmbedder.cpp    # 文本向量化
│   ├── SimilaritySearch.cpp # 向量相似度搜索
│   └── main.cpp            # 引擎主逻辑
├── include/                # 头文件
│   ├── TextEmbedder.h
│   ├── SimilaritySearch.h
│   └── com_example_w2v_W2VNative.h # JNI 生成头文件
├── jni/                    # JNI 接口层
│   ├── W2VNative.java      # Java 接口类
│   └── com_example_w2v_W2VNative.cpp # JNI 实现 (C++)
├── models/                 # 模型存放目录
│   └── light_Tencent_AILab_ChineseEmbedding.bin (需下载)
├── data/                   # 数据文件
│   ├── qa_list.csv         # 推荐 QA 格式 (CSV)
│   └── qa_list.txt         # 旧版 QA 格式 (Pipe 分隔)
├── scripts/                # 自动化脚本
│   ├── build_linux.sh      # Linux 构建脚本
│   └── build_android.sh    # Android 编译脚本
├── tests/                  # 测试用例
└── CMakeLists.txt          # 项目构建配置
```

## 📥 模型下载

本项目依赖腾讯 AI Lab 预训练模型，请下载后放置于 `models/` 目录：

- **Hugging Face**: [alextomcat/light_Tencent_AILab_ChineseEmbedding](https://huggingface.co/alextomcat/light_Tencent_AILab_ChineseEmbedding)
- **百度网盘**: [下载地址](https://pan.baidu.com/s/1La4U4XNFe8s5BJqxPQpeiQ) (提取码请查阅相关说明)

## 🛠️ 核心接口说明

### 1. 引擎初始化与知识库加载
```java
// 初始化引擎，返回原生对象指针
long enginePtr = W2VNative.initEngine(modelPath);

// 方式 A：从 CSV 加载 (推荐)
W2VNative.loadQAFromFile(enginePtr, "data/qa_list.csv");

// 方式 B：从内存加载 (动态构建)
String[] questions = {"如何注册账号?", "忘记密码怎么办?"};
String[] answers = {"点击右上角注册按钮。", "请点击找回密码并输入邮箱。"};
W2VNative.loadQAFromMemory(enginePtr, questions, answers);
```

### 2. 语义搜索
```java
// 执行搜索，返回最相似的结果对象
W2VNative.SearchResult result = W2VNative.search(enginePtr, "我想改密码");

if (result != null) {
    System.out.println("匹配问题: " + result.question);
    System.out.println("对应答案: " + result.answer);
    System.out.println("置信度: " + result.score);
}
```

## 📊 性能指标

| 指标项 | 表现 | 备注 |
| :--- | :--- | :--- |
| **模型规格** | 14.3万词 / 200维度 | 压缩版腾讯 AI Lab 模型 |
| **搜索耗时** | < 1ms | ARM64 设备测试结果 |
| **内存占用** | ~300MB | 包含模型权重与索引 |
| **支持容量** | 10,000+ QA 对 | 线性扩展，保持高性能 |

## ⚙️ 环境要求

- **硬件**：ARM64 (Android/RK3588等) 或 x86_64 设备。
- **软件**：Android 7.0+ (API 24) 或 Linux (Ubuntu/CentOS)。
- **编译**：C++17 兼容编译器。
- **Android NDK**：
    - 需要手动下载并安装 [Android NDK (推荐 r21+)](https://developer.android.com/ndk/downloads)。
    - 编译时需配置环境变量 `ANDROID_NDK` 指向你的安装目录。

## 📄 许可证

本项目遵循 MIT 协议开源。仅供学习与技术研究使用。
