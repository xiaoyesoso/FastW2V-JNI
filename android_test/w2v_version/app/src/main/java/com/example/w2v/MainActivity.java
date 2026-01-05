package com.example.w2v;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.TextView;
import android.widget.Button;
import android.view.View;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;

public class MainActivity extends Activity {
    private TextView statusText;
    private TextView resultText;
    private Button initButton;
    private Button searchButton;
    private long enginePtr = 0;
    private Handler mainHandler;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        mainHandler = new Handler(Looper.getMainLooper());
        
        // 初始化UI
        statusText = findViewById(R.id.status_text);
        resultText = findViewById(R.id.result_text);
        initButton = findViewById(R.id.init_button);
        searchButton = findViewById(R.id.search_button);
        
        // 设置按钮点击监听
        initButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                initializeEngine();
            }
        });
        
        searchButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                performSearch();
            }
        });
        
        // 初始状态
        updateStatus("应用已启动，点击初始化引擎");
        searchButton.setEnabled(false);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (enginePtr != 0) {
            W2VNative.releaseEngine(enginePtr);
            enginePtr = 0;
        }
    }
    
    private void initializeEngine() {
        updateStatus("正在初始化引擎...");
        initButton.setEnabled(false);
        
        // 在后台线程执行初始化
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // 1. 准备模型和 QA 路径 (从 Assets 复制到内部存储)
                    String modelPath = copyAssetToInternalStorage("light_Tencent_AILab_ChineseEmbedding.bin");
                    String qaPath = copyAssetToInternalStorage("qa_list.csv");

                    // 2. 初始化引擎
                    enginePtr = W2VNative.initEngine(modelPath);
                    if (enginePtr == 0) {
                        updateStatusOnMainThread("引擎初始化失败");
                        enableButtonOnMainThread(initButton, true);
                        return;
                    }

                    // 3. 加载 QA 数据
                    boolean success = W2VNative.loadQAFromFile(enginePtr, qaPath);
                    
                    if (success) {
                        int qaCount = W2VNative.getQACount(enginePtr);
                        updateStatusOnMainThread("引擎初始化成功\n加载了 " + qaCount + " 个QA对");
                        enableButtonOnMainThread(searchButton, true);
                        
                        // 调用测试代码
                        AndroidJavaTest.runTest(enginePtr);
                    } else {
                        updateStatusOnMainThread("QA 加载失败");
                        W2VNative.releaseEngine(enginePtr);
                        enginePtr = 0;
                        enableButtonOnMainThread(initButton, true);
                    }
                } catch (IOException e) {
                    updateStatusOnMainThread("资源加载异常: " + e.getMessage());
                    enableButtonOnMainThread(initButton, true);
                }
            }
        }).start();
    }
    
    private void performSearch() {
        updateStatus("正在搜索...");
        searchButton.setEnabled(false);
        
        // 在后台线程执行搜索
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (enginePtr == 0) return;

                    // 使用模仿 LinuxJavaTest 的逻辑进行完整测试
                    AndroidJavaTest.runTest(enginePtr);
                    
                    // 为 UI 显示准备结果
                    StringBuilder sb = new StringBuilder();
                    sb.append("=== 模仿 LinuxJavaTest 运行成功 ===\n");
                    sb.append("详细日志请查看 Logcat (Tag: AndroidJavaTest)\n\n");
                    
                    String[] uiQueries = {"你好", "系统状态"};
                    for (String query : uiQueries) {
                        W2VNative.SearchResult result = W2VNative.search(enginePtr, query);
                        if (result != null) {
                            sb.append("问: ").append(query).append("\n");
                            sb.append("答: ").append(result.answer).append("\n");
                            sb.append("分: ").append(result.score).append("\n\n");
                        }
                    }

                    final String finalResult = sb.toString();
                    mainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            resultText.setText(finalResult);
                            updateStatus("搜索完成");
                            searchButton.setEnabled(true);
                        }
                    });
                } catch (Exception e) {
                    updateStatusOnMainThread("搜索异常: " + e.getMessage());
                    enableButtonOnMainThread(searchButton, true);
                }
            }
        }).start();
    }

    private String copyAssetToInternalStorage(String filename) throws IOException {
        File destFile = new File(getFilesDir(), filename);
        if (!destFile.exists() || destFile.length() == 0) {
            try (InputStream in = getAssets().open(filename);
                 FileOutputStream out = new FileOutputStream(destFile)) {
                byte[] buffer = new byte[8192];
                int length;
                while ((length = in.read(buffer)) > 0) {
                    out.write(buffer, 0, length);
                }
            }
        }
        return destFile.getAbsolutePath();
    }
    
    private void updateStatus(final String message) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                statusText.setText(message);
            }
        });
    }
    
    private void updateStatusOnMainThread(String message) {
        updateStatus(message);
    }
    
    private void enableButtonOnMainThread(final Button button, final boolean enabled) {
        mainHandler.post(new Runnable() {
            @Override
            public void run() {
                button.setEnabled(enabled);
            }
        });
    }
}