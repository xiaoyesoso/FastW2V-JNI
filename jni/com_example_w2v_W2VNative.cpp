#include "../include/main.h"
#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#ifdef ANDROID
#include <android/log.h>
#define TAG "W2VNative_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#else
#include <iostream>
#define LOGI(...) printf(__VA_ARGS__); printf("\n")
#define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#endif

// 全局引擎映射
std::unordered_map<jlong, std::shared_ptr<W2VEngine> > engine_map;
jlong next_engine_id = 1;

// JNI辅助函数
std::string jstring_to_string(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";
    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string str(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return str;
}

jstring string_to_jstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

std::vector<std::string> jobjectarray_to_stringvector(JNIEnv* env, jobjectArray jarray) {
    std::vector<std::string> result;
    if (!jarray) return result;
    jsize length = env->GetArrayLength(jarray);
    result.reserve(length);
    for (jsize i = 0; i < length; i++) {
        jstring jstr = (jstring)env->GetObjectArrayElement(jarray, i);
        result.push_back(jstring_to_string(env, jstr));
        env->DeleteLocalRef(jstr);
    }
    return result;
}

// 原生方法实现
jlong native_initEngine(JNIEnv *env, jclass clazz, jstring modelPath) {
    std::string model_path = jstring_to_string(env, modelPath);
    std::shared_ptr<W2VEngine> engine = std::make_shared<W2VEngine>();
    if (!engine->initialize(model_path)) {
        return 0;
    }
    jlong engine_id = next_engine_id++;
    engine_map[engine_id] = engine;
    return engine_id;
}

jboolean native_loadQAFromFile(JNIEnv *env, jclass clazz, jlong enginePtr, jstring filePath) {
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) return JNI_FALSE;
    return it->second->load_qa_from_file(jstring_to_string(env, filePath)) ? JNI_TRUE : JNI_FALSE;
}

jboolean native_loadQAFromMemory(JNIEnv *env, jclass clazz, jlong enginePtr, jobjectArray questions, jobjectArray answers) {
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) return JNI_FALSE;
    std::vector<std::string> q_vec = jobjectarray_to_stringvector(env, questions);
    std::vector<std::string> a_vec = jobjectarray_to_stringvector(env, answers);
    if (q_vec.size() != a_vec.size()) return JNI_FALSE;
    std::vector<std::pair<std::string, std::string> > qa_pairs;
    for (size_t i = 0; i < q_vec.size(); i++) qa_pairs.emplace_back(q_vec[i], a_vec[i]);
    return it->second->load_qa_from_memory(qa_pairs) ? JNI_TRUE : JNI_FALSE;
}

// 缓存 SearchResult 类信息
static jclass gResultClass = nullptr;
static jmethodID gResultInit = nullptr;

jobject native_search(JNIEnv *env, jclass clazz, jlong enginePtr, jstring query) {
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) return nullptr;
    float similarity = 0.0f;
    auto result = it->second->search(jstring_to_string(env, query), &similarity);
    
    if (!gResultClass || !gResultInit) return nullptr;
    
    jstring jq = string_to_jstring(env, result.first);
    jstring ja = string_to_jstring(env, result.second);
    jobject jobj = env->NewObject(gResultClass, gResultInit, jq, ja, similarity);
    env->DeleteLocalRef(jq);
    env->DeleteLocalRef(ja);
    return jobj;
}

jobjectArray native_searchBatch(JNIEnv *env, jclass clazz, jlong enginePtr, jobjectArray queries) {
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) return nullptr;
    std::vector<std::string> q_vec = jobjectarray_to_stringvector(env, queries);
    std::vector<float> sims;
    auto results = it->second->search_batch(q_vec, &sims);
    
    if (!gResultClass || !gResultInit) return nullptr;
    jobjectArray jarray = env->NewObjectArray(results.size(), gResultClass, nullptr);
    
    for (size_t i = 0; i < results.size(); i++) {
        jstring jq = string_to_jstring(env, results[i].first);
        jstring ja = string_to_jstring(env, results[i].second);
        jobject jobj = env->NewObject(gResultClass, gResultInit, jq, ja, sims[i]);
        env->SetObjectArrayElement(jarray, i, jobj);
        env->DeleteLocalRef(jq);
        env->DeleteLocalRef(ja);
        env->DeleteLocalRef(jobj);
    }
    return jarray;
}

jint native_getQACount(JNIEnv *env, jclass clazz, jlong enginePtr) {
    auto it = engine_map.find(enginePtr);
    return (it != engine_map.end()) ? (jint)it->second->get_qa_count() : 0;
}

jint native_getEmbeddingDim(JNIEnv *env, jclass clazz, jlong enginePtr) {
    auto it = engine_map.find(enginePtr);
    return (it != engine_map.end()) ? (jint)it->second->get_embedding_dim() : 0;
}

jlong native_getMemoryUsage(JNIEnv *env, jclass clazz, jlong enginePtr) {
    auto it = engine_map.find(enginePtr);
    return (it != engine_map.end()) ? (jlong)it->second->get_memory_usage() : 0;
}

void native_releaseEngine(JNIEnv *env, jclass clazz, jlong enginePtr) {
    auto it = engine_map.find(enginePtr);
    if (it != engine_map.end()) {
        it->second->release();
        engine_map.erase(it);
    }
}

// 动态注册方法表
static JNINativeMethod gMethods[] = {
    {"initEngine", "(Ljava/lang/String;)J", (void*)native_initEngine},
    {"loadQAFromFile", "(JLjava/lang/String;)Z", (void*)native_loadQAFromFile},
    {"loadQAFromMemory", "(J[Ljava/lang/String;[Ljava/lang/String;)Z", (void*)native_loadQAFromMemory},
    {"search", nullptr, (void*)native_search},
    {"searchBatch", nullptr, (void*)native_searchBatch},
    {"getQACount", "(J)I", (void*)native_getQACount},
    {"getEmbeddingDim", "(J)I", (void*)native_getEmbeddingDim},
    {"getMemoryUsage", "(J)J", (void*)native_getMemoryUsage},
    {"releaseEngine", "(J)V", (void*)native_releaseEngine}
};

// 存储动态生成的签名，防止被释放
static std::string gSearchSig;
static std::string gSearchBatchSig;

// 为了支持不同的包名，可以在编译时通过 -DJNI_CLASS_NAME="path/to/Class" 来指定
// 如果未指定，则使用默认值
#ifndef JNI_CLASS_NAME
#define JNI_CLASS_NAME "com/example/w2v/W2VNative"
#endif

// JNI_OnLoad 动态注册
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;

    // 使用宏定义的类名
    const char* className = JNI_CLASS_NAME;
    jclass clazz = env->FindClass(className);
    
    if (!clazz) {
        env->ExceptionClear();
        LOGE("无法找到类: %s", className);
        return JNI_ERR;
    }

    // 动态构建签名以支持自定义包名
    gSearchSig = "(JLjava/lang/String;)L" + std::string(className) + "$SearchResult;";
    gSearchBatchSig = "(J[Ljava/lang/String;)[L" + std::string(className) + "$SearchResult;";
    gMethods[3].signature = (char*)gSearchSig.c_str();
    gMethods[4].signature = (char*)gSearchBatchSig.c_str();

    // 缓存内部类 SearchResult 信息
    std::string resultClassName = std::string(className) + "$SearchResult";
    jclass resClass = env->FindClass(resultClassName.c_str());
    if (resClass) {
        gResultClass = (jclass)env->NewGlobalRef(resClass);
        gResultInit = env->GetMethodID(gResultClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;F)V");
    }

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        LOGE("注册原生方法失败");
        return JNI_ERR;
    }

    LOGI("W2V JNI 库加载成功");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        if (gResultClass) env->DeleteGlobalRef(gResultClass);
    }
}
