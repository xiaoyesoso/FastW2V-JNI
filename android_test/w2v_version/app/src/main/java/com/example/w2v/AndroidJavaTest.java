package com.example.w2v;

import android.util.Log;
import java.io.File;

/**
 * 模仿 LinuxJavaTest.java 的逻辑
 * 专门用于在 Android 环境下进行 JNI 调用测试
 */
public class AndroidJavaTest {
    private static final String TAG = "AndroidJavaTest";

    public static void runTest(long enginePtr) {
        Log.i(TAG, "=== Android Java JNI 调用测试 (模仿 LinuxJavaTest) ===");
        
        if (enginePtr == 0) {
            Log.e(TAG, "错误: 引擎未初始化！");
            return;
        }
        
        try {
            // 1. 检查引擎状态
            int count = W2VNative.getQACount(enginePtr);
            if (count <= 0) {
                Log.e(TAG, "错误: QA 数据未加载或加载失败！");
                return;
            }
            Log.i(TAG, "QA 数据就绪, 共 " + count + " 条记录");
            
            // 2. 测试单次搜索
            String query = "你好";
            Log.i(TAG, "正在搜索: \"" + query + "\"");
            long startTime = System.nanoTime();
            W2VNative.SearchResult result = W2VNative.search(enginePtr, query);
            long endTime = System.nanoTime();
            Log.i(TAG, "单次搜索耗时: " + (endTime - startTime) / 1000000.0 + " ms");
            
            if (result != null) {
                Log.i(TAG, "匹配问题: " + result.question);
                Log.i(TAG, "匹配答案: " + result.answer);
                Log.i(TAG, "相似度得分: " + result.score);
            } else {
                Log.i(TAG, "未找到匹配结果");
            }
            
            // 3. 测试批量搜索
            String[] queries = {
                "系统状态",          // 高相关
                "RK3588处理器",     // 高相关
                "如何重启系统服务",   // 语义相关
                "学习写代码",        // 语义相关
                "今天天气不错",      // 不相关
                "我想吃苹果",        // 不相关
                "珠穆朗玛峰有多高",    // 不相关
                "AbC dEf GhI 123"   // 极低相关/随机
            };
            Log.i(TAG, "\n正在进行批量搜索 (共 " + queries.length + " 个查询)...");
            startTime = System.nanoTime();
            W2VNative.SearchResult[] batchResults = W2VNative.searchBatch(enginePtr, queries);
            endTime = System.nanoTime();
            double totalMs = (endTime - startTime) / 1000000.0;
            Log.i(TAG, String.format("批量搜索总耗时: %.3f ms, 平均每条: %.3f ms", totalMs, totalMs / queries.length));
            
            if (batchResults != null) {
                for (int i = 0; i < batchResults.length; i++) {
                    if (batchResults[i] != null) {
                        Log.i(TAG, "查询 [" + queries[i] + "] -> 匹配问题: [" + batchResults[i].question + "] -> 结果: " + batchResults[i].answer + " (得分: " + batchResults[i].score + ")");
                    } else {
                        Log.w(TAG, "查询 [" + queries[i] + "] -> 无结果");
                    }
                }
            }
            
            // 4. 测试引擎元数据获取
            Log.i(TAG, "\n引擎元数据:");
            Log.i(TAG, "- 向量维度: " + W2VNative.getEmbeddingDim(enginePtr));
            long bytes = W2VNative.getMemoryUsage(enginePtr);
            String memoryStr;
            if (bytes < 1024) {
                memoryStr = bytes + " B";
            } else if (bytes < 1024 * 1024) {
                memoryStr = String.format("%.2f KB", bytes / 1024.0);
            } else {
                memoryStr = String.format("%.2f MB", bytes / (1024.0 * 1024.0));
            }
            Log.i(TAG, "- 内存占用: " + memoryStr);
            
        } catch (Exception e) {
            Log.e(TAG, "测试过程中发生异常", e);
        }
        
        Log.i(TAG, "=== 测试完成 ===");
    }
}
