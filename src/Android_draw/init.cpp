//
// Created by JiangNight on 2023/6/13.
//
#include "init.h"

EGLDisplay display = EGL_NO_DISPLAY;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;
MDisplayInfo displayInfo;
int32_t init_screen_width = 0;
int32_t init_screen_height = 0;
//TextureInfo icon = {};

bool g_Initialized = false;
ANativeWindow *native_window;
bool had_windows = false;


bool init_android_draw(){
    // EGL初始化过程
    ANativeWindow *(*createNativeWindow)(const char *surface_name ,uint32_t screen_width ,uint32_t screen_height, bool is_by_pass, void* getFunc); // 构造指针
    createNativeWindow = (ANativeWindow *(*)(const char *, uint32_t, uint32_t, bool, void*))(funcPointer.func_createNativeWindow); // 构造函数
    get_native_create_transaction(); // 获取nativeCreateTransaction
    native_window = createNativeWindow("SsageParuders",displayInfo.override_width, displayInfo.override_height, true, get_func); // 创建Native Surface
    ANativeWindow_acquire(native_window); // 请求一个windows
    had_windows = true; // 有了Windows
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY); //
    if (display == EGL_NO_DISPLAY) {
        printf("eglGetDisplay error=%u\n", glGetError());
        return false;
    }
    if (!eglInitialize(display, 0, 0)) {
        printf("eglInitialize error=%u\n", glGetError());
        return false;
    }
    EGLint num_config = 0; // 支持多少配置数量
    const EGLint attrib_lists[] = { // attrib_lists
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_BLUE_SIZE, 5,   //-->delete
            EGL_GREEN_SIZE, 6,  //-->delete
            EGL_RED_SIZE, 5,    //-->delete
            EGL_BUFFER_SIZE, 32,  //-->new field
            EGL_DEPTH_SIZE, 16,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE
    };
    if (!eglChooseConfig(display, attrib_lists, nullptr, 0, &num_config)) {
        printf("eglChooseConfig  error=%u\n", glGetError());
        return false;
    }
    EGLConfig config = nullptr;
    if (!eglChooseConfig(display, attrib_lists, &config, 1, &num_config)) {
        printf("eglChooseConfig  error=%u\n", glGetError());
        return false;
    }
    EGLint egl_format; // egl格式
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &egl_format); // 获取格式
    ANativeWindow_setBuffersGeometry(native_window, 0, 0, egl_format); // 清空buff
    const EGLint attrib_list[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE}; // 配置列表
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrib_list); // 创建内容
    if (context == EGL_NO_CONTEXT) {
        printf("eglCreateContext  error = %u\n", glGetError());
        return false;
    }
    surface = eglCreateWindowSurface(display, config, native_window, nullptr); // 创建surface
    if (surface == EGL_NO_SURFACE) {
        printf("eglCreateWindowSurface  error = %u\n", glGetError());
        return false;
    }
    if (!eglMakeCurrent(display, surface, surface, context)) {
        printf("eglMakeCurrent  error = %u\n", glGetError());
        return false;
    }
    return true;
}

void release_android_draw(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    if (display != EGL_NO_DISPLAY){
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT){
            eglDestroyContext(display, context);
            return;
        }
        if (surface != EGL_NO_SURFACE){
            eglDestroySurface(display, surface);
            return;
        }
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;
    ANativeWindow_release(native_window);
    return;
}

void init_screen_config(){
// 屏幕信息获取线程
    std::thread *orithread = new std::thread([&] {
        MDisplayInfo (*getDisplayInfo)(); // 构造指针
        getDisplayInfo = (MDisplayInfo (*)())(funcPointer.func_getDisplayInfo);
        void (*setSize)(uint32_t screen_width, uint32_t screen_height); // 构造指针
        setSize = (void (*)(uint32_t, uint32_t)) (funcPointer.func_setSize);
        bool init_screen = false;
        while(true){
            displayInfo = getDisplayInfo(); // 不断刷新获取屏幕信息
            if (!init_screen) {
                init_screen = true;
                if (displayInfo.orientation == 0 || displayInfo.orientation == 2){ // 竖屏 && 倒屏
                    init_screen_height = displayInfo.physical_height; // 2400
                    init_screen_width = displayInfo.override_width; // 1080
                }
                if (displayInfo.orientation == 1 || displayInfo.orientation == 3){ // 横屏1 && 横屏2
                    init_screen_height = displayInfo.override_width; // 2400
                    init_screen_width = displayInfo.override_height; // 1080
                }
            }
            if (!had_windows) { // 尚未创建native_window
                continue; // 等待下次循环
            }
            if(displayInfo.orientation == 0 || displayInfo.orientation == 2){ // 竖屏 && 倒屏
                setSize(displayInfo.override_width, displayInfo.override_height); // 交换屏幕
            }
            if(displayInfo.orientation == 1 || displayInfo.orientation == 3){ // 横屏1 && 横屏2
                setSize(displayInfo.override_width, displayInfo.override_height); // 交换屏幕
            }
            // printf("orientation is %d width is %d height is %d\n", displayInfo.orientation, displayInfo.override_width, displayInfo.override_height);
            std::this_thread::sleep_for(0.1s);
        }
    });
    orithread->detach();
}

ImTextureID createTexture(unsigned char *buffer,int len) {
    int w, h, n;
    stbi_uc *data = stbi_png_load_from_memory(reinterpret_cast<const stbi_uc *>(buffer), len, &w, &h, &n, 0);
    GLuint texture;
    glGenTextures(1, &texture);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (n == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }
    stbi_image_free(data);
    return reinterpret_cast<ImTextureID>((GLuint *) texture);
}

//TextureInfo ImAgeHeadFile(const char* ImagePath)
//{
//    int w,h,n;
//    stbi_uc * data = stbi_load(ImagePath,&w,&h,&n,0);
//    GLuint texture;
//    glGenTextures(1, &texture);
//    glEnable(GL_TEXTURE_2D);
//    glBindTexture(GL_TEXTURE_2D, texture);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    if(n==3)
//    {
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w , h , 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//    } else{
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w , h , 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
//    }
//    stbi_image_free(data);
//    TextureInfo textureInfo;
//    textureInfo.textureId=(GLuint*)texture;
//    textureInfo.w=w;
//    textureInfo.h=h;
//    return textureInfo;
//}


void drawBegin(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
}

void drawEnd(){
    ImGuiIO &io = ImGui::GetIO();
    glViewport(0.0f, 0.0f, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT); // GL_DEPTH_BUFFER_BIT
    glFlush();
    if (display == EGL_NO_DISPLAY) {
        return;
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(display, surface);
}

void init_imgui(){
    // IMGUI初始化过程
    if (g_Initialized){
        return;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui::StyleColorsDark();
    ImGui_ImplAndroid_Init(native_window);
    ImGui_ImplOpenGL3_Init("#version 300 es");
    io.Fonts->AddFontFromMemoryTTF((void*)font_v, font_v_size, 30.f,NULL,io.Fonts->GetGlyphRangesChineseFull());
    g_Initialized = true;
}



