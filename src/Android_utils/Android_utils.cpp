//
// Created by 泓清 on 2022/9/3.
//
// Self lib
#include <Android_utils.h>
// System libs
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <jni.h>
#include <dlfcn.h>
#include <asm-generic/unistd.h>
#include <sys/system_properties.h>
// User libs
#include <And64InlineHook/And64InlineHook.hpp>

static int (*register_android_view_SurfaceControl)(JNIEnv* env) = NULL;
static void *nativeCreateTransaction = NULL;
void *get_func = NULL;
static int (*orig_registerNativeMethods)(JNIEnv* env, const char* className, const JNINativeMethod* gMethods, int numMethods) = NULL;

void *dlblob(const void *blob, size_t len){
    int fd = syscall(__NR_memfd_create, "Ssgae_shm_anon", (unsigned int)(MFD_CLOEXEC));
    ftruncate(fd, len);
    void *mem = mmap(NULL, len, PROT_WRITE, MAP_SHARED, fd, 0);
    memcpy(mem, blob, len);
    munmap(mem, len);
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/self/fd/%d", fd);
    void *so = dlopen(path, RTLD_NOW);
    close(fd);
    return so;
}

string exec(string command){
    char buffer[128];
    string result = "";
    // Open pipe to file
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "popen failed!";
    }
    // read till end of process:
    while (!feof(pipe)) {
        // use buffer to read and add to result
        if (fgets(buffer, 128, pipe) != nullptr){
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

int get_android_api_level(){
    char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.sdk", prop_value);
    return atoi(prop_value);
}

static inline int fake_env_func(){
    return 1;
}

static inline JNIEnv *get_fake_env(){
    JNIEnv *env = (JNIEnv *)malloc(sizeof(JNIEnv));
    env->functions = (JNINativeInterface *)malloc(sizeof(JNINativeInterface));
    void *func = (void *)fake_env_func;
    void **funcTbl = (void **)env->functions;
    for (int i = 0; i < sizeof(JNINativeInterface) / sizeof(void *); i++){
        funcTbl[i] = func;
    }
    return env;
}

static inline int new_registerNativeMethods(JNIEnv* env,char const* className, const JNINativeMethod* gMethods, int numMethods){
    if (!strcmp(className,"android/view/SurfaceControl")) {
        for (int i = 0; i < numMethods; i++) {
            if (!strcmp(gMethods[i].name, "nativeCreateTransaction")) {
                nativeCreateTransaction = gMethods[i].fnPtr;
            }
        }
        return 1; // 这里解释一下为什么返回-1
        /**
         * 首先 我们调用这个函数的目的只是为了获取nativeCreateTransaction的偏移
         * 那么到这里为止 我们已经获取到了偏移 剩下的已经不再重要了
         * 然后我们返回1 让他正常运行 因为调用orig原函数会出非法访问
         * 为什么呢 因为我们传入进去的env是虚构的fake_env
         * 而orig函数内会深度使用env 造成程序异常
         */
    }
    return orig_registerNativeMethods(env, className, gMethods, numMethods);
}

void* get_native_create_transaction(){
    const char *libandroid_runtime = "/system/lib64/libandroid_runtime.so";
    void *so = dlopen(libandroid_runtime, RTLD_NOW);
    if (!so){
        exit(0);
    }
    *(void **)&register_android_view_SurfaceControl = dlsym(so, "_ZN7android36register_android_view_SurfaceControlEP7_JNIEnv");
    if (!register_android_view_SurfaceControl){
        dlclose(so);
        exit(0);
    }
    void *registerNativeMethods = dlsym(so, "_ZN7android14AndroidRuntime21registerNativeMethodsEP7_JNIEnvPKcPK15JNINativeMethodi");
    if (!registerNativeMethods){
        dlclose(so);
        exit(0);
    }
    A64HookFunction(registerNativeMethods, (void *)new_registerNativeMethods, (void **)&orig_registerNativeMethods);
    JNIEnv *env = get_fake_env();
    register_android_view_SurfaceControl(env);
    free((void *)env->functions);
    free((void *)env);
    get_func = nativeCreateTransaction;
    if (!get_func){
        dlclose(so);
        exit(0);
    }
    return 0;
}

const char *get_android_mac(){
    char *prop_value = (char *) malloc(PROP_VALUE_MAX);
    __system_property_get("ro.serialno", prop_value);
    return prop_value;
}
