import com.example.w2v.W2VNative;

public class TestW2V {
    public static void main(String[] args) {
        System.out.println("=== W2V智能问答系统Java测试 ===");
        System.out.println("测试JNI接口功能");
        System.out.println();
        
        long enginePtr = 0;
        
        try {
            // 测试1: 初始化引擎
            System.out.println("1. 初始化引擎...");
            String modelPath = "models/light_Tencent_AILab_ChineseEmbedding.bin";
            enginePtr = W2VNative.initEngine(modelPath);
            
            if (enginePtr == 0) {
                System.err.println("初始化引擎失败！");
                return;
            }
            System.out.println("引擎初始化成功，指针: " + enginePtr);
            
            // 获取引擎信息
            int embeddingDim = W2VNative.getEmbeddingDim(enginePtr);
            long memoryUsage = W2VNative.getMemoryUsage(enginePtr);
            System.out.println("向量维度: " + embeddingDim);
            System.out.println("内存使用: " + memoryUsage + " bytes");
            
            // 测试2: 从内存加载QA
            System.out.println("\n2. 从内存加载QA...");
            String[] questions = {
                "你好",
                "今天天气怎么样",
                "如何学习编程",
                "什么是人工智能",
                "推荐一本好书"
            };
            
            String[] answers = {
                "你好！我是智能问答助手，有什么可以帮助你的吗？",
                "今天天气晴朗，温度适宜。",
                "学习编程需要多实践，从基础语法开始。",
                "人工智能是让机器模拟人类智能的技术。",
                "《深入理解计算机系统》是一本很好的书。"
            };
            
            boolean memoryLoadSuccess = W2VNative.loadQAFromMemory(enginePtr, questions, answers);
            if (memoryLoadSuccess) {
                int qaCount = W2VNative.getQACount(enginePtr);
                System.out.println("内存加载成功，QA数量: " + qaCount);
            } else {
                System.err.println("内存加载失败！");
            }
            
            // 测试3: 从文件加载QA（CSV格式）
            System.out.println("\n3. 从CSV文件加载QA...");
            String csvFilePath = "data/qa_list.csv";
            boolean fileLoadSuccess = W2VNative.loadQAFromFile(enginePtr, csvFilePath);
            
            if (fileLoadSuccess) {
                int qaCount = W2VNative.getQACount(enginePtr);
                System.out.println("CSV文件加载成功，QA数量: " + qaCount);
            } else {
                System.err.println("CSV文件加载失败！");
            }
            
            // 测试4: 单次搜索
            System.out.println("\n4. 单次搜索测试...");
            String[] testQueries = {
                "你好啊",
                "天气如何",
                "怎么学编程",
                "AI是什么",
                "有什么好书推荐"
            };
            
            for (String query : testQueries) {
                System.out.println("\n查询: \"" + query + "\"");
                W2VNative.SearchResult result = W2VNative.search(enginePtr, query);
                
                if (result != null) {
                    System.out.println("匹配问题: \"" + result.question + "\"");
                    System.out.println("答案: \"" + result.answer + "\"");
                    System.out.println("相似度: " + String.format("%.4f", result.score));
                } else {
                    System.out.println("未找到匹配结果");
                }
            }
            
            // 测试5: 批量搜索
            System.out.println("\n5. 批量搜索测试...");
            String[] batchQueries = {
                "系统状态",
                "学习英语",
                "RK3588",
                "你的功能"
            };
            
            W2VNative.SearchResult[] batchResults = W2VNative.searchBatch(enginePtr, batchQueries);
            
            if (batchResults != null) {
                for (int i = 0; i < batchQueries.length; i++) {
                    System.out.println("\n查询: \"" + batchQueries[i] + "\"");
                    if (batchResults[i] != null) {
                        System.out.println("匹配问题: \"" + batchResults[i].question + "\"");
                        System.out.println("答案: \"" + batchResults[i].answer + "\"");
                        System.out.println("相似度: " + String.format("%.4f", batchResults[i].score));
                    } else {
                        System.out.println("未找到匹配结果");
                    }
                }
            } else {
                System.out.println("批量搜索返回null");
            }
            
            // 测试6: 性能测试
            System.out.println("\n6. 性能测试...");
            int performanceTestCount = 100;
            long startTime = System.currentTimeMillis();
            
            for (int i = 0; i < performanceTestCount; i++) {
                W2VNative.search(enginePtr, "测试查询" + i);
            }
            
            long endTime = System.currentTimeMillis();
            long totalTime = endTime - startTime;
            double avgTime = (double) totalTime / performanceTestCount;
            
            System.out.println("执行 " + performanceTestCount + " 次搜索");
            System.out.println("总耗时: " + totalTime + " ms");
            System.out.println("平均每次: " + String.format("%.2f", avgTime) + " ms");
            
        } catch (Exception e) {
            System.err.println("测试过程中发生异常:");
            e.printStackTrace();
        } finally {
            // 清理资源
            if (enginePtr != 0) {
                System.out.println("\n7. 释放引擎资源...");
                W2VNative.releaseEngine(enginePtr);
                System.out.println("引擎资源已释放");
            }
        }
        
        System.out.println("\n=== 测试完成 ===");
    }
}