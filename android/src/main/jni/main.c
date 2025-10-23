//
// Created by Andreas on 22.10.2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <dlfcn.h>
#include <string.h>
#include <android/log.h>
#include "environ/environ.h"
#include "dlfake/fake_dlfcn.h"

#define INIT_SUCCESS 0x00
#define INIT_GENERIC_ERROR 0xFFFFFFFF
#define INIT_DEX_NOT_INITIALIZED 0xFFFFFFFE
#define INIT_DVM_NOT_FOUND 0xFFFFFFFD
#define INIT_METHOD_NOT_INITIALIZED 0xFFFFFFFC

#define DIE(msg) do { printf(msg); exit(EXIT_FAILURE); } while(0)

typedef  jint(*jni_getvms_t)(JavaVM**, jsize, jsize*);

JavaVM *game_vm;
JavaVM *dalvik_vm;
char *dex_data;
size_t dex_size = 0;

jobject object_classLoader;
jmethodID method_loadClass;

jclass class_mainClass;
jmethodID method_openKeyboard;

// get vm functions stolen from https://github.com/artdeell/AutoWax4C
jni_getvms_t getVMFunction_fake() {
    void* library = fake_dlopen("libart.so", 0);
    if(library == NULL) {
        printf("[AutoKeyboard/VMFinder] Google trolling failed. Giving up.\n");
        return NULL;
    }
    void* sym = fake_dlsym(library, "JNI_GetCreatedJavaVMs");
    fake_dlclose(library);
    return (jni_getvms_t)sym;
}
jni_getvms_t getVMFunction() {
    void* library = dlopen("libnativehelper.so", RTLD_LAZY);
    if(library == NULL) {
        printf("[AutoKeyboard/VMFinder] Time to troll Google!\n");
        return getVMFunction_fake();
    }
    void* sym = dlsym(library, "JNI_GetCreatedJavaVMs");
    dlclose(library);
    return sym == NULL ? getVMFunction_fake() : (jni_getvms_t) sym;
}

jclass LoadClass(JNIEnv* env, const char* name) {
    jclass clazz = (jclass) (*env)->CallObjectMethod(env, object_classLoader, method_loadClass,
                                                     (*env)->NewStringUTF(env, name));
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        printf("[AutoKeyboard/LoadClass] Failed to load %s\n", name);
        return NULL;
    }
    return clazz;
}

jobject getAppClassLoader(JNIEnv* env) {
    jclass activityThreadClass = (*env)->FindClass(env, "android/app/ActivityThread");
    if (!activityThreadClass) return NULL;

    jmethodID currentActivityThreadMethod = (*env)->GetStaticMethodID(env, activityThreadClass,
                                                                      "currentActivityThread",
                                                                      "()Landroid/app/ActivityThread;");
    if (!currentActivityThreadMethod) return NULL;

    jobject activityThread = (*env)->CallStaticObjectMethod(env, activityThreadClass,
                                                            currentActivityThreadMethod);
    if (!activityThread) return NULL;

    jmethodID getApplicationMethod = (*env)->GetMethodID(env, activityThreadClass, "getApplication", "()Landroid/app/Application;");
    if(!getApplicationMethod) return NULL;

    jobject application = (*env)->CallObjectMethod(env, activityThread, getApplicationMethod);
    if(!application) return NULL;

    jclass contextClass = (*env)->FindClass(env, "android/content/Context");
    if(!contextClass) return NULL;

    jmethodID getClassLoaderMethod = (*env)->GetMethodID(env, contextClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    if(!getClassLoaderMethod) return NULL;

    return (*env)->CallObjectMethod(env, application, getClassLoaderMethod);
}

JNIEXPORT jint JNICALL
Java_me_andreasmelone_autokeyboard_pojav_AutoKeyboardAndroidNative_init(JNIEnv *env,
                                                                        jclass clazz) {
    (*env)->GetJavaVM(env, &game_vm);
    if(!dex_data || dex_size == 0) {
        return INIT_DEX_NOT_INITIALIZED;
    }

    JNIEnv *dalvik_env;

    jsize cnt;
    jni_getvms_t JNI_GetCreatedJavaVMs_p = getVMFunction();
    if(!JNI_GetCreatedJavaVMs_p || JNI_GetCreatedJavaVMs_p(&dalvik_vm, 1, &cnt) != JNI_OK || cnt == 0) {
        printf("[AutoKeyboard/Init] Failed to find a JVM\n");
        if(pojav_environ == NULL || pojav_environ->dalvikJavaVMPtr == NULL) return INIT_DVM_NOT_FOUND;
        dalvik_vm = pojav_environ->dalvikJavaVMPtr;
        printf("[AutoKeyboard/Init] If the game crashes after this message, the pojav_environ is incompatible with the mod. "
               "This means that you should remove the AutoKeyboard mod, switch to a different launcher or try using a different device/Android version.\n");
    }

    jint result = (*dalvik_vm)->GetEnv(dalvik_vm, (void**)&dalvik_env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        result = (*dalvik_vm)->AttachCurrentThread(dalvik_vm, &dalvik_env, NULL);
    }
    if (result != JNI_OK) {
        printf("[AutoKeyboard/Init] Can't get a JNIEnv\n");
        return INIT_DVM_NOT_FOUND;
    }

    jclass class_InMemoryClassLoader = (*dalvik_env)->FindClass(dalvik_env, "dalvik/system/InMemoryDexClassLoader");
    if(!class_InMemoryClassLoader || (*dalvik_env)->ExceptionCheck(dalvik_env)) {
        (*dalvik_env)->ExceptionDescribe(dalvik_env);
        printf("[AutoKeyboard/Init] Can't find the class loader\n");
        return INIT_GENERIC_ERROR;
    }

    jmethodID constructor_InMemoryClassLoader = (*dalvik_env)->GetMethodID(dalvik_env, class_InMemoryClassLoader, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    method_loadClass = (*dalvik_env)->GetMethodID(dalvik_env, class_InMemoryClassLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if(!constructor_InMemoryClassLoader || !method_loadClass){
        printf("[AutoKeyboard/Init] Can't find the class methods\n");
        return INIT_GENERIC_ERROR;
    }
    jobject byteBuffer = (*dalvik_env)->NewDirectByteBuffer(dalvik_env, dex_data, dex_size);
    if(!byteBuffer) {
        printf("[AutoKeyboard/Init] Can't create the byte buffer\n");
        return INIT_GENERIC_ERROR;
    }
    jobject appClassLoader = getAppClassLoader(dalvik_env);
    if(appClassLoader == NULL) {
        printf("[AutoKeyboard/Init] Failed to retrieve app classloader!\n");
        return INIT_GENERIC_ERROR;
    }
    object_classLoader = (*dalvik_env)->NewObject(dalvik_env, class_InMemoryClassLoader, constructor_InMemoryClassLoader, byteBuffer, appClassLoader);
    if((*dalvik_env)->ExceptionCheck(dalvik_env)) {
        (*dalvik_env)->ExceptionDescribe(dalvik_env);
        printf("[AutoKeyboard/Init] Failed to create class loader\n");
        return INIT_GENERIC_ERROR;
    }
    object_classLoader = (*dalvik_env)->NewGlobalRef(dalvik_env, object_classLoader);
    jclass mainClass = LoadClass(dalvik_env, "me.andreasmelone.autokeyboard.pojav.AutoKeyboardAndroid");
    if(!mainClass) {
        printf("[AutoKeyboard/Init] Failed to load main class\n");
        return INIT_GENERIC_ERROR;
    }
    printf("[AutoKeyboard/Init] ClassLoader class = %p\n", mainClass);
    class_mainClass = (jclass) (*dalvik_env)->NewGlobalRef(dalvik_env, mainClass);
    method_openKeyboard = (*dalvik_env)->GetStaticMethodID(dalvik_env, class_mainClass, "setKeyboardOpen", "(Z)V");
    if(!method_openKeyboard) {
        printf("[AutoKeyboard/Init] Failed to get setKeyboardOpen(Z)V method\n");
        return INIT_METHOD_NOT_INITIALIZED;
    }

    return INIT_SUCCESS;
}

JNIEXPORT void JNICALL
Java_me_andreasmelone_autokeyboard_pojav_AutoKeyboardAndroidNative_setDexData(JNIEnv *env, jclass clazz, jbyteArray data) {
    if(!data) {
        jclass exceptionClass = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        (*env)->ThrowNew(env, exceptionClass, "Null data passed!");
        return;
    }

    if(dex_data) {
        free(dex_data);
        dex_data = NULL;
    }

    jboolean isCopy;
    jbyte* b = (*env)->GetByteArrayElements(env, data, &isCopy);
    if(!b) {
        DIE("[AutoKeyboard/SetDexData] Failed to get byte array, exiting\n");
    }
    jint size = (*env)->GetArrayLength(env, data);

    dex_size = size;
    printf("[AutoKeyboard/SetDexData] New byte array size: %zu\n", dex_size);
    dex_data = malloc(size);
    if(!dex_data) {
        DIE("[AutoKeyboard/SetDexData] Failed to allocate memory, exiting\n");
    }
    memcpy(dex_data, b, size);

    (*env)->ReleaseByteArrayElements(env, data, b, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_me_andreasmelone_autokeyboard_pojav_AutoKeyboardAndroidNative_setKeyboardState(JNIEnv *env, jclass clazz, jboolean state) {
    if(!class_mainClass || !method_openKeyboard) {
        DIE("[AutoKeyboard/SetKeyboardState] Main class or open keyboard method not initialized!\n");
    }

    JNIEnv *dvm;
    jint result = (*dalvik_vm)->GetEnv(dalvik_vm, (void**)&dvm, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        result = (*dalvik_vm)->AttachCurrentThread(dalvik_vm, &dvm, NULL);
    }
    if (result != JNI_OK) {
        printf("[AutoKeyboard/SetKeyboardState] Can't get a JNIEnv\n");
        return;
    }

    (*dvm)->CallStaticVoidMethod(dvm, class_mainClass, method_openKeyboard, state);
}