#include "../include/com_example_w2v_W2VNative.h"
#include "../include/main.h"
#include <jni.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

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

jobjectArray stringvector_to_jobjectarray(JNIEnv* env, const std::vector<std::string>& vec) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray jarray = env->NewObjectArray(vec.size(), stringClass, nullptr);
    
    for (size_t i = 0; i < vec.size(); i++) {
        jstring jstr = string_to_jstring(env, vec[i]);
        env->SetObjectArrayElement(jarray, i, jstr);
        env->DeleteLocalRef(jstr);
    }
    
    return jarray;
}

jobjectArray stringpair_to_jobjectarray(JNIEnv* env, const std::pair<std::string, std::string>& pair) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray jarray = env->NewObjectArray(2, stringClass, nullptr);
    
    jstring jstr1 = string_to_jstring(env, pair.first);
    jstring jstr2 = string_to_jstring(env, pair.second);
    
    env->SetObjectArrayElement(jarray, 0, jstr1);
    env->SetObjectArrayElement(jarray, 1, jstr2);
    
    env->DeleteLocalRef(jstr1);
    env->DeleteLocalRef(jstr2);
    
    return jarray;
}

JNIEXPORT jlong JNICALL Java_com_example_w2v_W2VNative_initEngine
  (JNIEnv *env, jclass clazz, jstring modelPath) {
    
    std::string model_path = jstring_to_string(env, modelPath);
    
    std::shared_ptr<W2VEngine> engine = std::make_shared<W2VEngine>();
    if (!engine->initialize(model_path)) {
        return 0;
    }
    
    jlong engine_id = next_engine_id++;
    engine_map[engine_id] = engine;
    
    return engine_id;
}

JNIEXPORT jboolean JNICALL Java_com_example_w2v_W2VNative_loadQAFromFile
  (JNIEnv *env, jclass clazz, jlong enginePtr, jstring filePath) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return JNI_FALSE;
    }
    
    std::string file_path = jstring_to_string(env, filePath);
    bool success = it->second->load_qa_from_file(file_path);
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_com_example_w2v_W2VNative_loadQAFromMemory
  (JNIEnv *env, jclass clazz, jlong enginePtr, jobjectArray questions, jobjectArray answers) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return JNI_FALSE;
    }
    
    std::vector<std::string> question_vec = jobjectarray_to_stringvector(env, questions);
    std::vector<std::string> answer_vec = jobjectarray_to_stringvector(env, answers);
    
    if (question_vec.size() != answer_vec.size()) {
        return JNI_FALSE;
    }
    
    std::vector<std::pair<std::string, std::string> > qa_pairs;
    qa_pairs.reserve(question_vec.size());
    
    for (size_t i = 0; i < question_vec.size(); i++) {
        qa_pairs.emplace_back(question_vec[i], answer_vec[i]);
    }
    
    bool success = it->second->load_qa_from_memory(qa_pairs);
    
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jobject JNICALL Java_com_example_w2v_W2VNative_search
  (JNIEnv *env, jclass clazz, jlong enginePtr, jstring query) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return nullptr;
    }
    
    std::string query_str = jstring_to_string(env, query);
    float similarity = 0.0f;
    auto result = it->second->search(query_str, &similarity);
    
    // 创建SearchResult对象
    jclass resultClass = env->FindClass("com/example/w2v/W2VNative$SearchResult");
    if (!resultClass) {
        return nullptr;
    }
    
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;F)V");
    if (!constructor) {
        return nullptr;
    }
    
    jstring jquestion = string_to_jstring(env, result.first);
    jstring janswer = string_to_jstring(env, result.second);
    
    jobject jobj = env->NewObject(resultClass, constructor, jquestion, janswer, similarity);
    
    env->DeleteLocalRef(jquestion);
    env->DeleteLocalRef(janswer);
    
    return jobj;
}

JNIEXPORT jobjectArray JNICALL Java_com_example_w2v_W2VNative_searchBatch
  (JNIEnv *env, jclass clazz, jlong enginePtr, jobjectArray queries) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return nullptr;
    }
    
    std::vector<std::string> query_vec = jobjectarray_to_stringvector(env, queries);
    std::vector<float> similarities;
    auto results = it->second->search_batch(query_vec, &similarities);
    
    if (results.size() != similarities.size()) {
        return nullptr;
    }
    
    // 创建SearchResult对象数组
    jclass resultClass = env->FindClass("com/example/w2v/W2VNative$SearchResult");
    if (!resultClass) {
        return nullptr;
    }
    
    jmethodID constructor = env->GetMethodID(resultClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;F)V");
    if (!constructor) {
        return nullptr;
    }
    
    jobjectArray jresult = env->NewObjectArray(results.size(), resultClass, nullptr);
    
    for (size_t i = 0; i < results.size(); i++) {
        jstring jquestion = string_to_jstring(env, results[i].first);
        jstring janswer = string_to_jstring(env, results[i].second);
        
        jobject jobj = env->NewObject(resultClass, constructor, jquestion, janswer, similarities[i]);
        env->SetObjectArrayElement(jresult, i, jobj);
        
        env->DeleteLocalRef(jquestion);
        env->DeleteLocalRef(janswer);
        env->DeleteLocalRef(jobj);
    }
    
    return jresult;
}

JNIEXPORT jint JNICALL Java_com_example_w2v_W2VNative_getQACount
  (JNIEnv *env, jclass clazz, jlong enginePtr) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return 0;
    }
    
    return static_cast<jint>(it->second->get_qa_count());
}

JNIEXPORT jint JNICALL Java_com_example_w2v_W2VNative_getEmbeddingDim
  (JNIEnv *env, jclass clazz, jlong enginePtr) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return 0;
    }
    
    return static_cast<jint>(it->second->get_embedding_dim());
}

JNIEXPORT jlong JNICALL Java_com_example_w2v_W2VNative_getMemoryUsage
  (JNIEnv *env, jclass clazz, jlong enginePtr) {
    
    auto it = engine_map.find(enginePtr);
    if (it == engine_map.end()) {
        return 0;
    }
    
    return static_cast<jlong>(it->second->get_memory_usage());
}

JNIEXPORT void JNICALL Java_com_example_w2v_W2VNative_releaseEngine
  (JNIEnv *env, jclass clazz, jlong enginePtr) {
    
    auto it = engine_map.find(enginePtr);
    if (it != engine_map.end()) {
        it->second->release();
        engine_map.erase(it);
    }
}