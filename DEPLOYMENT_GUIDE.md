# W2V智能问答系统部署指南

## 部署包内容

### 核心文件
1. **light_Tencent_AILab_ChineseEmbedding.bin** - 腾讯AI Lab预训练中文词向量模型
   - 大小: 111MB
   - 词汇量: 143,613个词
   - 向量维度: 200维

2. **qa_list.csv** - QA知识库（CSV格式）
   - 格式: `question,answer`
   - 示例:
     ```csv
     question,answer
     你好,你好！我是智能问答助手，有什么可以帮助你的吗？
     系统状态如何,系统运行正常，所有服务都处于良好状态。
     如何学习编程,学习编程需要耐心和实践，建议从Python或JavaScript开始，多做项目练习。
     ```

3. **W2VNative.java** - Java JNI接口类
   - 包名: `com.example.w2v`
   - 提供三个核心方法:
     - `initEngine()` - 初始化引擎
     - `loadQAFromFile()` - 加载QA文件
     - `search()` - 搜索最相似问题

4. **libw2v_jni.so** - JNI动态链接库
   - 架构: ARM64 (RK3588)
   - 依赖: 标准C++库

5. **build.sh** - 构建脚本
   - 支持Linux和Android交叉编译
   - 包含测试功能

## 部署步骤

### 步骤1: 准备部署包
```bash
# 在开发机上创建部署包
mkdir -p deploy
cp models/light_Tencent_AILab_ChineseEmbedding.bin deploy/
cp data/qa_list.csv deploy/
cp jni/W2VNative.java deploy/
cp build.sh deploy/

# 编译Linux版本
cd deploy
chmod +x build.sh
./build.sh linux

# 打包
tar -czf w2v_deploy.tar.gz light_Tencent_AILab_ChineseEmbedding.bin qa_list.csv W2VNative.java libw2v_jni.so build.sh
```

### 步骤2: 传输到RK3588
```bash
# 拷贝到U盘
cp w2v_deploy.tar.gz /path/to/usb/

# 在RK3588上插入U盘并复制
cp /media/usb/w2v_deploy.tar.gz ~/
cd ~
tar -xzf w2v_deploy.tar.gz
```

### 步骤3: 验证部署
```bash
# 查看文件
ls -la
# 应该看到以下文件:
# -rw-r--r-- 1 user user 111M light_Tencent_AILab_ChineseEmbedding.bin
# -rw-r--r-- 1 user user  2.5K qa_list.csv
# -rw-r--r-- 1 user user  1.5K W2VNative.java
# -rwxr-xr-x 1 user user  1.2M libw2v_jni.so
# -rwxr-xr-x 1 user user  6.5K build.sh

# 验证库文件
file libw2v_jni.so
# 输出应该显示: ELF 64-bit LSB shared object, ARM aarch64

# 运行简单测试
./build.sh test
```

## Android集成指南

### 1. 将文件复制到Android项目
```bash
# 复制JNI库到Android项目
cp libw2v_jni.so /path/to/android-project/app/src/main/jniLibs/arm64-v8a/

# 复制Java接口类
cp W2VNative.java /path/to/android-project/app/src/main/java/com/example/w2v/

# 复制模型和QA文件到assets目录
cp light_Tencent_AILab_ChineseEmbedding.bin /path/to/android-project/app/src/main/assets/
cp qa_list.csv /path/to/android-project/app/src/main/assets/
```

### 2. Android项目配置
```gradle
// app/build.gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a'
        }
    }
    
    sourceSets {
        main {
            jniLibs.srcDirs = ['src/main/jniLibs']
        }
    }
}
```

### 3. Android代码示例
```java
package com.example.w2v;

import android.content.Context;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class W2VManager {
    private long enginePtr = 0;
    private Context context;
    
    public W2VManager(Context context) {
        this.context = context;
    }
    
    public boolean initialize() {
        // 复制模型文件到内部存储
        copyAssetToInternalStorage("light_Tencent_AILab_ChineseEmbedding.bin");
        copyAssetToInternalStorage("qa_list.csv");
        
        // 初始化引擎
        String modelPath = context.getFilesDir().getPath() + "/light_Tencent_AILab_ChineseEmbedding.bin";
        enginePtr = W2VNative.initEngine(modelPath);
        
        if (enginePtr != 0) {
            // 加载QA文件
            String qaPath = context.getFilesDir().getPath() + "/qa_list.csv";
            return W2VNative.loadQAFromFile(enginePtr, qaPath);
        }
        
        return false;
    }
    
    public W2VNative.SearchResult search(String query) {
        if (enginePtr != 0) {
            return W2VNative.search(enginePtr, query);
        }
        return null;
    }
    
    public void release() {
        if (enginePtr != 0) {
            W2VNative.releaseEngine(enginePtr);
            enginePtr = 0;
        }
    }
    
    private void copyAssetToInternalStorage(String filename) {
        File destFile = new File(context.getFilesDir(), filename);
        if (!destFile.exists()) {
            try (InputStream in = context.getAssets().open(filename);
                 FileOutputStream out = new FileOutputStream(destFile)) {
                byte[] buffer = new byte[1024];
                int length;
                while ((length = in.read(buffer)) > 0) {
                    out.write(buffer, 0, length);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}
```

## 性能优化建议

### 1. 内存管理
- 模型加载后占用约300MB内存
- 建议在应用启动时初始化，避免频繁创建/销毁
- 在`onDestroy()`或应用退出时调用`release()`释放资源

### 2. 线程安全
- JNI调用应在后台线程执行
- 避免在主线程进行模型加载和搜索操作
- 使用`AsyncTask`或`Thread`包装JNI调用

### 3. QA知识库管理
- CSV格式便于编辑和维护
- 支持动态更新QA列表
- 建议定期更新知识库以提高准确性

## 故障排除

### 常见问题1: 库加载失败
```
java.lang.UnsatisfiedLinkError: dlopen failed: library "libw2v_jni.so" not found
```
**解决方案**:
- 确认`libw2v_jni.so`在`jniLibs/arm64-v8a/`目录中
- 检查Android项目的`abiFilters`配置
- 验证库文件架构是否为ARM64

### 常见问题2: 模型加载失败
```
模型加载失败: 无法打开文件
```
**解决方案**:
- 确认模型文件已复制到assets目录
- 检查文件复制代码是否正确
- 验证文件路径权限

### 常见问题3: 搜索返回空结果
```
搜索返回null或相似度为0
```
**解决方案**:
- 检查QA文件格式是否正确
- 确认QA文件已成功加载
- 验证查询文本不为空

## 扩展功能

### 1. 批量搜索
```java
// 支持批量查询
String[] queries = {"问题1", "问题2", "问题3"};
for (String query : queries) {
    W2VNative.SearchResult result = W2VNative.search(enginePtr, query);
    // 处理结果
}
```

### 2. 动态更新QA
```java
// 动态添加QA对
String[] newQuestions = {"新问题1", "新问题2"};
String[] newAnswers = {"新答案1", "新答案2"};
W2VNative.loadQAFromMemory(enginePtr, newQuestions, newAnswers);
```

### 3. 性能监控
```java
// 获取系统状态
int qaCount = W2VNative.getQACount(enginePtr);
float memoryUsage = W2VNative.getMemoryUsage(enginePtr);
```

## 技术支持
遇到问题请提供:
1. 错误日志
2. Android设备信息
3. 复现步骤
4. 相关代码片段