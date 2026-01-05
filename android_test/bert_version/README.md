# W2V Android Semantic Search SDK Integration Guide (BERT Version)

This SDK provides deep learning-based semantic search functionality powered by **Chinese BERT (CoROM-Tiny)**. Compared to traditional Word2Vec, BERT offers better contextual understanding and higher-quality text matching results.

## 🚀 Key Features

- **Deep Semantic Understanding**: Based on the Transformer architecture, supporting complex contextual matching.
- **Industry-grade Alignment**: Uses the CLS Pooling strategy, perfectly aligned with ModelScope pre-trained models.
- **Raw Similarity Score**: Returns raw cosine similarity (-1.0 to 1.0), accurately reflecting the match degree.

## 1. Project Structure & Core Files

| File Type | File Path (Android Project) | Description |
| :--- | :--- | :--- |
| **JNI Library** | `app/src/main/jniLibs/arm64-v8a/libw2v_jni.so` | Core engine |
| **Java Interface** | `app/src/main/java/com/example/w2v/W2VNative.java` | JNI method definitions |
| **BERT Model** | `app/src/main/assets/model.onnx` | ONNX format model |
| **Vocab File** | `app/src/main/assets/vocab.txt` | Tokenizer vocabulary |

## 2. Quick Integration Steps

### Step 1: Add ONNX Runtime Dependency
The BERT engine depends on Microsoft's ONNX Runtime. Add the following to your `app/build.gradle`:
```gradle
dependencies {
    implementation 'com.microsoft.onnxruntime:onnxruntime-android:latest.release'
}
```

### Step 2: Configure NDK Library Loading
```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### Step 3: Initialization and Search
```java
// 1. Initialize BERT engine (Recommended to run on a background thread)
long enginePtr = W2VNative.initBertEngine(onnxPath, vocabPath);

if (enginePtr != 0) {
    // 2. Load QA data
    W2VNative.loadQAFromFile(enginePtr, qaPath);
    
    // 3. Execute search
    W2VNative.SearchResult result = W2VNative.search(enginePtr, "How to restart the system");
    System.out.println("Similarity: " + result.score); // Range: -1.0 ~ 1.0
}
```

## 3. Technical Specifications

- **Model Architecture**: CoROM-Tiny (256 dimensions).
- **Inference Backend**: ONNX Runtime (CPU).
- **Performance**: Approximately 100ms per inference on ARM64 devices.

## 4. Simulation Testing
Run `./run_android_sim_test.sh` to start automated testing.
