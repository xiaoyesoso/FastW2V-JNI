# FastW2V-JNI

High-performance C++ semantic search and Q&A engine, supporting dual engines: **Word2Vec** and **BERT (ONNX)**. Specifically designed for mobile (Android) and embedded devices, achieving millisecond-level semantic matching.

## 🚀 Features

- ✅ **Dual Engine Support**: Supports traditional Word2Vec word embeddings and modern BERT (CoROM) deep learning models.
- ✅ **Extreme Performance**: Implemented in native C++17, Word2Vec search takes <1ms, BERT inference is highly efficient.
- ✅ **Cross-platform JNI**: Provides a comprehensive Java Native Interface for easy integration into Android or Java projects.
- ✅ **Fully Offline**: Supports on-device deployment without internet access, with controllable memory usage.
- ✅ **Industry-grade Alignment**: BERT engine perfectly aligns with ModelScope CoROM pooling strategy (CLS Pooling).
- ✅ **Raw Similarity**: Returns raw cosine similarity (-1 to 1), accurately reflecting model confidence.

## 📂 Project Structure

```text
.
├── src/                # Core C++ source code (W2V, BERT, Tokenizer, Search)
├── include/            # C++ header files
├── jni/                # JNI interface layer (Java definition & C++ implementation)
├── android_test/       # Android platform examples
│   ├── w2v_version/    # Word2Vec integration example
│   └── bert_version/   # BERT (ONNX Runtime) integration example
├── linux_java_test/    # Linux/Desktop Java test project
├── models/             # Model storage directory
├── data/               # Sample QA data (CSV format)
├── build_android.sh    # Global Android NDK build script
└── CMakeLists.txt      # CMake build configuration
```

## 🛠️ Quick Start

### 1. Model Preparation

This project supports two engines. Follow these steps for model acquisition and conversion:

#### **Exporting BERT (CoROM-Tiny) to ONNX**
1. **Download Model**:
   - Source: [ModelScope - CoROM Sentence Embedding (Chinese-Tiny)](https://www.modelscope.cn/models/iic/nlp_corom_sentence-embedding_chinese-tiny)
   - The script below will handle the download automatically.
2. **Environment Setup**:
   ```bash
   pip install torch transformers modelscope onnx
   ```
2. **Run Export Script**:
   Use the provided script to download the model from ModelScope and export it to ONNX format:
   ```bash
   python scripts/convert_model.py
   ```
   The script will automatically:
   - Download `iic/nlp_corom_sentence-embedding_chinese-tiny` from ModelScope.
   - Extract `[CLS]` vector as sentence representation.
   - Export `model.onnx` and extract `vocab.txt` to the `export/` directory.

#### **Acquiring and Compressing Word2Vec (Tencent AILab)**
1. **Download Raw Model**:
   - Source: [Hugging Face - Tencent AI Lab Chinese Embedding (Light)](https://huggingface.co/alextomcat/light_Tencent_AILab_ChineseEmbedding)
   - Download the pre-trained Chinese word embeddings (recommend `light` version).
2. **Convert to Binary Format**:
   The `.bin` format used in this project is: first line `vocab_size dim`, followed by each line as `word` + `binary float vector`.
   You can use a simple Python script for conversion:
   ```python
   # Example: Converting txt format to binary .bin
   import struct
   with open('input.txt', 'r') as f, open('output.bin', 'wb') as f_out:
       header = f.readline()
       f_out.write(header.encode('utf-8')) # Write header info (txt)
       for line in f:
           parts = line.strip().split()
           word = parts[0]
           vec = [float(x) for x in parts[1:]]
           f_out.write(f"{word} ".encode('utf-8'))
           f_out.write(struct.pack(f'{len(vec)}f', *vec))
   ```

### 2. Initialize Engine
```java
// Word2Vec Mode
long enginePtr = W2VNative.initEngine(modelPath);

// BERT Mode
long enginePtr = W2VNative.initBertEngine(onnxPath, vocabPath);
```

### 3. Load Data and Search
```java
// Load QA knowledge base (CSV format)
W2VNative.loadQAFromFile(enginePtr, "qa_list.csv");

// Execute semantic search
W2VNative.SearchResult result = W2VNative.search(enginePtr, "How to restart the system");

if (result != null) {
    System.out.println("Matched Question: " + result.question);
    System.out.println("Confidence (Cos): " + result.score);
}
```

## 🐧 Linux / RK3588 Deployment

### 1. Build Linux Version
```bash
# Use build.sh script for local compilation
chmod +x build.sh
./build.sh linux
```

### 2. Deployment Steps
1. Copy the generated `libw2v_jni.so`, model files, and `qa_list.csv` to the target device.
2. Ensure the Java runtime can load the `.so` library (set `java.library.path`).
3. Verify the library: `file libw2v_jni.so` should show `ELF 64-bit LSB shared object, ARM aarch64`.

## 📱 Android Integration Guide

### 1. Copy Resources and Libraries
- **JNI Library**: Copy `libw2v_jni.so` to `app/src/main/jniLibs/arm64-v8a/`.
- **Java Interface**: Copy `W2VNative.java` to the corresponding package path.
- **Model**: Place the model and vocab files into the `assets` directory.

### 2. Project Configuration (build.gradle)
```gradle
android {
    defaultConfig {
        ndk { abiFilters 'arm64-v8a' }
    }
}
dependencies {
    // Required only for BERT mode
    implementation 'com.microsoft.onnxruntime:onnxruntime-android:latest.release'
}
```

### 3. Code Example
Since Android cannot directly access assets via file path, you must copy the model to a private directory first:
```java
String modelPath = context.getFilesDir().getPath() + "/model.onnx";
// ... Copy assets to modelPath ...
long enginePtr = W2VNative.initBertEngine(modelPath, vocabPath);
```

## 💡 Optimization and Troubleshooting

- **Memory Management**: Models remain in memory after loading (W2V ~120MB, BERT ~30MB). It's recommended to call `releaseEngine` explicitly in `onDestroy`.
- **Thread Safety**: JNI calls should not be executed on the UI thread; use a background thread instead.
- **Common Issues**:
    - `UnsatisfiedLinkError`: Check if `abiFilters` includes `arm64-v8a` and the library path is correct.
    - Model Loading Failure: Ensure the file path is an absolute path and has read permissions.

## 📊 Performance Metrics (Android ARM64)

| Engine | Model Size | Memory Usage | Search Latency |
| :--- | :--- | :--- | :--- |
| **Word2Vec** | ~50MB | ~120MB | < 0.2 ms |
| **BERT (Tiny)** | ~33MB | ~30MB | ~100 ms |

> **Note**: BERT engine depends on `onnxruntime-android`, while the Word2Vec engine has no external dependencies.

## 📄 License

This project is licensed under the MIT License. For educational and technical research purposes only.
