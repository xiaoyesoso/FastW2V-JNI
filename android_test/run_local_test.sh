#!/bin/bash

# 本地 Java 测试脚本 - 用于模拟运行 AndroidJavaTest 并打印其测试逻辑输出
# 修复了 Java 文件名匹配问题

set -e

PROJECT_ROOT="/Users/souljoy/Documents/trae_projects/w2v/android_test"
JAVA_SRC="$PROJECT_ROOT/app/src/main/java"
BUILD_DIR="$PROJECT_ROOT/build_local_test"

echo "=== 1. 准备测试环境 ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/android/util"
mkdir -p "$BUILD_DIR/android/content"
mkdir -p "$BUILD_DIR/android/content/res"
mkdir -p "$BUILD_DIR/com/example/w2v"

echo "=== 2. 创建 Android 模拟类 ==="

# 模拟 Log 类
cat > "$BUILD_DIR/android/util/Log.java" << 'EOF'
package android.util;
public class Log {
    public static int i(String t, String m) { System.out.println("[Log.i] " + t + ": " + m); return 0; }
    public static int e(String t, String m) { System.err.println("[Log.e] " + t + ": " + m); return 0; }
    public static int e(String t, String m, Throwable e) { System.err.println("[Log.e] " + t + ": " + m); e.printStackTrace(); return 0; }
    public static int w(String t, String m) { System.out.println("[Log.w] " + t + ": " + m); return 0; }
    public static int d(String t, String m) { System.out.println("[Log.d] " + t + ": " + m); return 0; }
}
EOF

# 模拟 AssetManager 类
cat > "$BUILD_DIR/android/content/res/AssetManager.java" << 'EOF'
package android.content.res;
import java.io.IOException;
import java.io.InputStream;
public class AssetManager {
    public InputStream open(String filename) throws IOException { return null; }
}
EOF

# 模拟 Context 类
cat > "$BUILD_DIR/android/content/Context.java" << 'EOF'
package android.content;
import android.content.res.AssetManager;
import java.io.File;
public abstract class Context {
    public abstract Context getApplicationContext();
    public abstract AssetManager getAssets();
    public abstract File getFilesDir();
}
EOF

echo "=== 3. 编译源码与准备 JNI 库 ==="
# 编译模拟的 Android 类
javac -d "$BUILD_DIR" \
    "$BUILD_DIR/android/util/Log.java" \
    "$BUILD_DIR/android/content/res/AssetManager.java" \
    "$BUILD_DIR/android/content/Context.java"

# 拷贝真实的 W2VNative.java
cp "$JAVA_SRC/com/example/w2v/W2VNative.java" "$BUILD_DIR/com/example/w2v/W2VNative.java"

# 注释掉 static 块中的 loadLibrary (我们将手动指定路径加载)
sed -i '' 's/System.loadLibrary("w2v_jni");//' "$BUILD_DIR/com/example/w2v/W2VNative.java"

# 编译所有类
javac -cp "$BUILD_DIR" -d "$BUILD_DIR" \
    "$BUILD_DIR/com/example/w2v/W2VNative.java" \
    "$JAVA_SRC/com/example/w2v/AndroidJavaTest.java"

echo "=== 4. 创建并运行测试运行器 (连接真实 JNI 引擎) ==="
cat > "$BUILD_DIR/TestRunner.java" << 'EOF'
import com.example.w2v.W2VNative;
import com.example.w2v.AndroidJavaTest;
import java.io.File;

public class TestRunner {
    public static void main(String[] args) {
        System.out.println(">>> 正在本地连接真实 JNI 引擎运行 AndroidJavaTest <<<\n");
        
        // 指定 dylib 路径
        String libPath = "/Users/souljoy/Documents/trae_projects/w2v/linux_java_test/libs/libw2v_jni.dylib";
        System.load(libPath);
        System.out.println("成功加载本地库: " + libPath);

        long enginePtr = 0;
        try {
            // 直接指向本地文件路径
            String modelPath = "/Users/souljoy/Documents/trae_projects/w2v/android_test/app/src/main/assets/light_Tencent_AILab_ChineseEmbedding.bin";
            String qaPath = "/Users/souljoy/Documents/trae_projects/w2v/android_test/app/src/main/assets/qa_list.csv";
            
            System.out.println("正在初始化模型: " + modelPath);
            enginePtr = W2VNative.initEngine(modelPath);
            
            if (enginePtr == 0) {
                System.err.println("引擎初始化失败");
                return;
            }

            if (W2VNative.loadQAFromFile(enginePtr, qaPath)) {
                // 运行测试
                AndroidJavaTest.runTest(enginePtr);
            } else {
                System.err.println("QA 加载失败");
            }
            
            System.out.println("\n>>> 运行结束 <<<");
            
        } catch (Throwable e) {
            System.err.println("!!! 运行过程发生错误 !!!");
            e.printStackTrace();
            System.exit(1);
        } finally {
            if (enginePtr != 0) {
                W2VNative.releaseEngine(enginePtr);
            }
        }
    }
}
EOF

javac -cp "$BUILD_DIR" -d "$BUILD_DIR" "$BUILD_DIR/TestRunner.java"
# 运行测试，指定库加载路径
java -cp "$BUILD_DIR" TestRunner

echo -e "\n=== 5. 清理测试目录 ==="
rm -rf "$BUILD_DIR"
