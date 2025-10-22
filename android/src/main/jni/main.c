//
// Created by Andreas on 22.10.2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <dlfcn.h>
#include <string.h>
#include "environ/environ.h"

#define INIT_SUCCESS 0x00
#define INIT_GENERIC_ERROR 0xFFFFFFFF
#define INIT_DEX_NOT_INITIALIZED 0xFFFFFFFE
#define INIT_DVM_NOT_FOUND 0xFFFFFFFD
#define INIT_METHOD_NOT_INITIALIZED 0xFFFFFFFC

#define DIE(msg) do { printf(msg); exit(EXIT_FAILURE); } while(0)

typedef  jint(*jni_getvms_t)(JavaVM**, jsize, jsize*);

JavaVM *vm;
char *dex_data;
size_t dex_size;

jobject object_classLoader;
jmethodID method_loadClass;

jclass class_mainClass;
jmethodID method_openKeyboard;

jni_getvms_t getVMFunction_libnativehelper() {
    void* library = dlopen("libnativehelper.so", RTLD_LAZY);
    if(library == NULL) {
        printf("Failed to load libnativehelper\n");
        return NULL;
    }
    void* sym = dlsym(library, "JNI_GetCreatedJavaVMs");
    dlclose(library);
    return (jni_getvms_t) sym;
}

jclass LoadClass(JNIEnv* env, const char* name) {
    jclass clazz = (jclass) (*env)->CallObjectMethod(env, object_classLoader, method_loadClass,
                                                     (*env)->NewStringUTF(env, name));
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        printf("Failed to load %s\n", name);
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

    jfieldID mBoundApplicationField = (*env)->GetFieldID(env, activityThreadClass,
                                                         "mBoundApplication",
                                                         "Landroid/app/ActivityThread$AppBindData;");
    if (!mBoundApplicationField) return NULL;

    jobject appBindData = (*env)->GetObjectField(env, activityThread, mBoundApplicationField);
    if (!appBindData) return NULL;

    jclass appBindDataClass = (*env)->GetObjectClass(env, appBindData);
    jfieldID infoField = (*env)->GetFieldID(env, appBindDataClass,
                                            "info",
                                            "Landroid/app/LoadedApk;");
    if (!infoField) return NULL;

    jobject appInfo = (*env)->GetObjectField(env, appBindData, infoField);
    if (!appInfo) return NULL;

    jclass appInfoClass = (*env)->GetObjectClass(env, appInfo);
    jfieldID classLoaderField = (*env)->GetFieldID(env, appInfoClass,
                                                   "mClassLoader",
                                                   "Ljava/lang/ClassLoader;");
    if (!classLoaderField) return NULL;

    jobject classLoader = (*env)->GetObjectField(env, appInfo, classLoaderField);
    return classLoader;
}

int init() {
    if(!dex_data) {
        return INIT_DEX_NOT_INITIALIZED;
    }

    JNIEnv *env;
    if(pojav_environ != NULL && pojav_environ->dalvikJavaVMPtr != NULL) {
        vm = pojav_environ->dalvikJavaVMPtr;
    } else {
        jsize cnt;
        jni_getvms_t JNI_GetCreatedJavaVMs_p = getVMFunction_libnativehelper();
        if(!JNI_GetCreatedJavaVMs_p || JNI_GetCreatedJavaVMs_p(&vm, 1, &cnt) != JNI_OK || cnt == 0) {
            printf("Failed to find a JVM\n");
            return INIT_DVM_NOT_FOUND;
        }
    }

    jint result = (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        result = (*vm)->AttachCurrentThread(vm, &env, NULL);
    }
    if (result != JNI_OK) {
        printf("Can't get a JNIEnv\n");
        return INIT_DVM_NOT_FOUND;
    }

    jclass class_InMemoryClassLoader = (*env)->FindClass(env, "dalvik/system/InMemoryDexClassLoader");
    if(!class_InMemoryClassLoader || (*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        printf("Can't find the class loader\n");
        return INIT_GENERIC_ERROR;
    }
    jclass class_Class = (*env)->FindClass(env, "java/lang/Class");
    if(!class_Class || (*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        printf("Can't find the Class class\n");
        return INIT_GENERIC_ERROR;
    }

    jmethodID constructor_InMemoryClassLoader = (*env)->GetMethodID(env, class_InMemoryClassLoader, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    method_loadClass = (*env)->GetMethodID(env, class_InMemoryClassLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if(!constructor_InMemoryClassLoader || !method_loadClass){
        printf("Can't find the class methods\n");
        return INIT_GENERIC_ERROR;
    }
    jobject byteBuffer = (*env)->NewDirectByteBuffer(env, dex_data, dex_size);
    if(!byteBuffer) {
        printf("Can't create the byte buffer\n");
        return INIT_GENERIC_ERROR;
    }
    jobject appClassLoader = getAppClassLoader(env);
    if(appClassLoader == NULL) {
        printf("Failed to retrieve app classloader!\n");
        return INIT_GENERIC_ERROR;
    }
    object_classLoader = (*env)->NewObject(env, class_InMemoryClassLoader, constructor_InMemoryClassLoader, byteBuffer, appClassLoader);
    if((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionDescribe(env);
        printf("Failed to create class loader\n");
        return INIT_GENERIC_ERROR;
    }
    object_classLoader = (*env)->NewGlobalRef(env, object_classLoader);
    jclass mainClass = LoadClass(env,"me.andreasmelone.pojavintegrate.pojav.PojavIntegrateAndroid");
    if(!mainClass) {
        printf("Failed to load main class\n");
        return INIT_GENERIC_ERROR;
    }
    printf("ClassLoader class = %p\n", mainClass);
    class_mainClass = (jclass) (*env)->NewGlobalRef(env, mainClass);
    method_openKeyboard = (*env)->GetStaticMethodID(env, class_mainClass, "setKeyboardOpen", "(Z)V");
    if(!method_openKeyboard) {
        printf("Failed to get setKeyboardOpen(Z)V method\n");
        return INIT_METHOD_NOT_INITIALIZED;
    }
    return INIT_SUCCESS;
}

JNIEXPORT jint JNICALL
Java_me_andreasmelone_autokeyboard_pojav_PojavIntegrateAndroidNative_init(JNIEnv *env,
                                                                          jclass clazz) {
    return init();
}


JNIEXPORT void JNICALL
Java_me_andreasmelone_autokeyboard_pojav_PojavIntegrateAndroidNative_setDexData(JNIEnv *env,
                                                                                jclass clazz,
                                                                                jbyteArray data) {
    if(dex_data) {
        free(dex_data);
        dex_data = NULL;
    }

    jboolean isCopy;
    jbyte* b = (*env)->GetByteArrayElements(env, data, &isCopy);
    if(!b) {
        DIE("Failed to get byte array, exiting\n");
    }
    jint size = (*env)->GetArrayLength(env, data);

    dex_size = size;
    printf("New byte array size: %zu\n", dex_size);
    dex_data = malloc(size);
    if(!dex_data) {
        DIE("Failed to allocate memory, exiting\n");
    }
    memcpy(dex_data, b, size);

    (*env)->ReleaseByteArrayElements(env, data, b, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_me_andreasmelone_autokeyboard_pojav_PojavIntegrateAndroidNative_setKeyboardState(JNIEnv *env, jclass clazz, jboolean state) {
    if(!class_mainClass || !method_openKeyboard) {
        DIE("Main class or open keyboard method not initialized!\n");
    }

    JNIEnv *dvm;
    jint result = (*vm)->GetEnv(vm, (void**)&dvm, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        result = (*vm)->AttachCurrentThread(vm, &dvm, NULL);
    }
    if (result != JNI_OK) {
        DIE("Can't get a JNIEnv\n");
    }

    (*dvm)->CallStaticVoidMethod(dvm, class_mainClass, method_openKeyboard, state);
}