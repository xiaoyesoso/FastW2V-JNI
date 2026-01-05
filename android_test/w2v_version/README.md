# W2V Android Semantic Search SDK Integration Guide (Word2Vec Version)

This SDK provides Word2Vec-based semantic search functionality, supporting local offline operation. It is primarily used for question-and-answer matching (QA Matching) and text similarity calculation on Android.

## 🚀 Key Features

- **Extreme Speed**: Pure C++ implementation, with search times < 0.2ms.
- **Raw Similarity Score**: Returns raw cosine similarity (-1.0 to 1.0), accurately reflecting the match degree.
- **Zero External Dependencies**: The JNI library runs independently without requiring additional runtimes.

## 1. Project Structure & Core Files

| File Type | File Path (Android Project) | Description |
| :--- | :--- | :--- |
| **JNI Library** | `app/src/main/jniLibs/arm64-v8a/libw2v_jni.so` | Core calculation engine |
| **Java Interface** | `app/src/main/java/com/example/w2v/W2VNative.java` | JNI method definitions |
| **Model File** | `app/src/main/assets/light_Tencent_AILab_ChineseEmbedding.bin` | Pre-trained model |
| **QA Data** | `app/src/main/assets/qa_list.csv` | Q&A knowledge base |

## 2. Quick Integration Steps

### Step 1: Configure NDK Library Loading
Ensure the `jniLibs` path is configured in your `app/build.gradle`:
```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
}
```

### Step 2: Initialization and Search
```java
// 1. Initialize engine (Recommended to run on a background thread)
long enginePtr = W2VNative.initEngine(modelPath);

if (enginePtr != 0) {
    // 2. Load QA data
    W2VNative.loadQAFromFile(enginePtr, qaPath);
    
    // 3. Execute search
    W2VNative.SearchResult result = W2VNative.search(enginePtr, "Hello");
    System.out.println("Similarity: " + result.score); // Range: -1.0 ~ 1.0
}
```

## 3. Technical Specifications

- **Architecture Support**: `arm64-v8a`, `armeabi-v7a`.
- **Similarity Algorithm**: Cosine Similarity.
- **Memory Usage**: Approximately 120MB (depending on model size).

## 4. Simulation Testing
Run `./run_android_sim_test.sh` to start automated testing.
