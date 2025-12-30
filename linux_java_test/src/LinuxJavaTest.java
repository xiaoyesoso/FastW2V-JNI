import com.example.w2v.W2VNative;
import java.io.File;

public class LinuxJavaTest {
    public static void main(String[] args) {
        System.out.println("=== Linux Java JNI 调用测试 ===");
        
        // 1. 初始化引擎
        String modelPath = "../models/light_Tencent_AILab_ChineseEmbedding.bin";
        File modelFile = new File(modelPath);
        if (!modelFile.exists()) {
            System.err.println("错误: 模型文件不存在: " + modelFile.getAbsolutePath());
            return;
        }
        
        System.out.println("正在初始化引擎, 模型: " + modelPath);
        long enginePtr = W2VNative.initEngine(modelPath);
        
        if (enginePtr == 0) {
            System.err.println("错误: 引擎初始化失败！");
            return;
        }
        System.out.println("引擎初始化成功, 指针: " + String.format("0x%x", enginePtr));
        
        try {
            // 2. 加载 QA 数据
            String qaPath = "../data/qa_list.csv";
            System.out.println("正在加载 QA 数据: " + qaPath);
            boolean loadSuccess = W2VNative.loadQAFromFile(enginePtr, qaPath);
            
            if (!loadSuccess) {
                System.err.println("错误: QA 数据加载失败！");
                return;
            }
            
            int count = W2VNative.getQACount(enginePtr);
            System.out.println("QA 数据加载成功, 共 " + count + " 条记录");
            
            // 3. 测试搜索
            String query = "你好";
            System.out.println("\n正在搜索: \"" + query + "\"");
            W2VNative.SearchResult result = W2VNative.search(enginePtr, query);
            
            if (result != null) {
                System.out.println("匹配问题: " + result.question);
                System.out.println("匹配答案: " + result.answer);
                System.out.println("相似度得分: " + result.score);
            } else {
                System.out.println("未找到匹配结果");
            }
            
            // 4. 测试批量搜索
            String[] queries = {"系统状态", "你是谁","你叫什么名字？"};
            System.out.println("\n正在进行批量搜索...");
            W2VNative.SearchResult[] batchResults = W2VNative.searchBatch(enginePtr, queries);
            
            for (int i = 0; i < batchResults.length; i++) {
                System.out.println("查询 [" + queries[i] + "] -> 结果: " + batchResults[i].answer + " (得分: " + batchResults[i].score + ")");
            }
            
        } finally {
            // 5. 释放引擎
            System.out.println("\n正在释放引擎...");
            W2VNative.releaseEngine(enginePtr);
            System.out.println("引擎已释放");
        }
        
        System.out.println("\n=== 测试完成 ===");
    }
}
