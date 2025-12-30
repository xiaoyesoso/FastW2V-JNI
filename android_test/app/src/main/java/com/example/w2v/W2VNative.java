package com.example.w2v;

public class W2VNative {
    static {
        System.loadLibrary("w2v_jni");
    }
    
    public static class SearchResult {
        public String question;
        public String answer;
        public float score;

        public SearchResult(String question, String answer, float score) {
            this.question = question;
            this.answer = answer;
            this.score = score;
        }
    }

    public static native long initEngine(String modelPath);
    
    public static native boolean loadQAFromFile(long enginePtr, String filePath);
    
    public static native boolean loadQAFromMemory(long enginePtr, String[] questions, String[] answers);
    
    public static native SearchResult search(long enginePtr, String query);
    
    public static native SearchResult[] searchBatch(long enginePtr, String[] queries);
    
    public static native int getQACount(long enginePtr);
    
    public static native int getEmbeddingDim(long enginePtr);
    
    public static native long getMemoryUsage(long enginePtr);
    
    public static native void releaseEngine(long enginePtr);
}