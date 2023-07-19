//
// Created by 泓清 on 2022/8/26.
//
// Self lib
#include <Android_touch/Android_touch.h>
// System libs
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <unistd.h>
#include <vector>
// User libs
#include <Android_draw/Android_draw.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <linux/uinput.h>

#define maxE 5 // 最大允许获得的event数量 设置为5 过多会导致多线程资源浪费
#define maxF 10 // 最大手指 10个
#define UNGRAB 0
#define GRAB 1

struct touchObj {
    bool isTmpDown = false;
    bool isDown = false;
    int x = 0;
    int y = 0;
    int id = 0;
};

struct targ {
    int fdNum;
    float S2TX;
    float S2TY;
};

// Var
int fdNum = 0, origfd[maxE], nowfd;
static struct input_event event[128];
static struct input_event upEvent[3];
int inputFd;
bool bPad = false; // 判断是否为特殊平板
float origScreenx, origScreeny;
float touchScreenx, touchScreeny;
float offsetx = 0.0f, offsety = 0.0f;
float halfOffsetx = 0.0f, halfOffsety = 0.0f;
float t2sx, t2sy;
static pthread_t touch_loop; // 触摸线程
struct touchObj Finger[maxE][maxF];
int screenX, screenY; // uinput注册设备 也是minCnt设备的触摸屏的xy
float imgui_x, imgui_y = 0;
bool isUpdate = false;
bool isOnIMGUI = false;
bool touchInit = false;

int get_touch_event_num() {
    DIR *dir = opendir("/dev/input/");
    dirent *ptr = NULL;
    int eventCount = 0;
    while ((ptr = readdir(dir))) {
        if (strstr(ptr->d_name, "event")) {
            eventCount++;
        }
    }
    closedir(dir);
    int *fdArray = (int *)malloc(eventCount * sizeof(int));
    for (int i = 0; i < eventCount; i++) {
        char temp[128];
        sprintf(temp, "/dev/input/event%d", i);
        fdArray[i] = open(temp, O_RDWR | O_NONBLOCK);
    }
    int ret = -1;
    input_event ie;
    for (;;) {
        for (int i = 0; i < eventCount; i++) {
            memset(&ie, 0, sizeof(ie));
            read(fdArray[i], &ie, sizeof(ie));
            if (ie.type == EV_ABS && ie.code == ABS_MT_TRACKING_ID &&
                ie.value == -1) { // 屏幕触摸
                ret = i;
                break;
            }
        }
        if (ret >= 0) {
            break;
        }
        usleep(100);
    }
    for (int i = 0; i < eventCount; i++) {
        close(fdArray[i]);
    }
    free(fdArray);
    return ret;
}
extern bool showMenu;
inline float move_x,move_y,down_x,down_y;
// 纯只读处理UI
void handleIMGUITouchEvent() {
    char temp[19];
    sprintf(temp, "/dev/input/event%d", get_touch_event_num());
    inputFd = open(temp, O_RDWR);
    struct input_absinfo absX, absY;
    ioctl(inputFd, EVIOCGABS(ABS_MT_POSITION_X), &absX);
    ioctl(inputFd, EVIOCGABS(ABS_MT_POSITION_Y), &absY);
    bPad = false;
    if (absX.maximum > absY.maximum) {
        bPad = true; // 特殊平板手机判断
    }
    if (bPad) {
        touchScreenx = init_screen_height;
        touchScreeny = init_screen_width;
        origScreenx = displayInfo.physical_height;
        origScreeny = displayInfo.physical_width;
    } else {
        touchScreenx = init_screen_width;
        touchScreeny = init_screen_height;
        origScreenx = displayInfo.physical_width;
        origScreeny = displayInfo.physical_height;
    }
    if (touchScreenx / origScreenx != touchScreeny / origScreeny) {
        float origScale = origScreeny / origScreenx;
        float newScale = touchScreeny / touchScreenx;
        if (newScale > origScale) {
            offsetx = (origScreenx - (origScreeny / touchScreeny) * touchScreenx) *
                      (absX.maximum / origScreenx);
        } else {
            offsety = (origScreeny - (origScreenx / touchScreenx) * touchScreeny) *
                      (absY.maximum / origScreeny);
        }
        halfOffsetx = offsetx * 0.5f;
        halfOffsety = offsety * 0.5f;
    }
    t2sx = float(touchScreenx) / (absX.maximum - offsetx);
    t2sy = float(touchScreeny) / (absY.maximum - offsety);
    int latest = 0;
    bool isUpdate = false;
    ImGuiIO &io = ImGui::GetIO();
    for (;;) {
        struct input_event iel[32];
        int32_t readSize = read(inputFd, &iel, sizeof(struct input_event));
        if (readSize <= 0 || (readSize % sizeof(struct input_event)) != 0) {
            continue;
        }
        size_t count = size_t(readSize) / sizeof(struct input_event);
        for (size_t j = 0; j < count; j++) {

            struct input_event ie = iel[j];
            if (ie.type != EV_ABS) {
                continue;
            }
            if (ie.code == ABS_MT_SLOT) {
                latest = ie.value;
                continue;
            }
            if (latest != 0) {
                continue;
            }
            if (ie.code == ABS_MT_TRACKING_ID) {
                if (ie.value == -1) {
                    down_x = down_y = 0;
                    io.MouseDown[0] = false;
                } else {
                    io.MouseDown[0] = true;
                }
                continue;
            }
            isUpdate = false;
            float x, y;
            if (ie.code == ABS_MT_POSITION_X) {
                x = (ie.value - halfOffsetx) * t2sx;
                isUpdate = true;
            }
            if (ie.code == ABS_MT_POSITION_Y) {
                y = (ie.value - halfOffsety) * t2sy;
                isUpdate = true;
            }
            if (!isUpdate) {
                continue;
            }
            if (bPad) {
                switch (displayInfo.orientation) {
                    case 0:
                        io.MousePos = ImVec2(touchScreeny - y, x);
                        break;
                    case 1:
                        io.MousePos = ImVec2(x, y);
                        break;
                    case 2:
                        io.MousePos = ImVec2(y, touchScreenx - x);
                        break;
                    case 3:
                        io.MousePos = ImVec2(touchScreenx - x, touchScreeny - y);
                        break;
                }
            } else {
                switch (displayInfo.orientation) {
                    case 0:
                        io.MousePos = ImVec2(x, y);
                        break;
                    case 1:
                        io.MousePos = ImVec2(y, touchScreenx - x);
                        break;
                    case 2:
                        io.MousePos = ImVec2(touchScreenx - x, touchScreeny - y);
                        break;
                    case 3:
                        io.MousePos = ImVec2(touchScreeny - y, x);
                        break;
                }
            }
        }
    }
}

void dispatchIMGUITouchEvent(input_event ie) {
    int latest = 0;
    ImGuiIO &io = ImGui::GetIO();
    if (ie.type != EV_ABS) {
        return;
    }
    if (ie.code == ABS_MT_SLOT) {
        latest = ie.value;
        return;
    }
    if (latest != 0) {
        return;
    }
    if (ie.code == ABS_MT_TRACKING_ID) {
        if (ie.value == -1) {
            io.MouseDown[0] = false;
        } else {
            io.MouseDown[0] = true;
        }
        return;
    }
    isUpdate = false;
    if (ie.code == ABS_MT_POSITION_X) {
        imgui_x = (ie.value - halfOffsetx) * t2sx;
        isUpdate = true;
    }
    if (ie.code == ABS_MT_POSITION_Y) {
        imgui_y = (ie.value - halfOffsety) * t2sy;
        isUpdate = true;
    }
    if (!isUpdate) {
        return;
    }
    if (bPad) {
        switch (displayInfo.orientation) {
            case 0:
                io.MousePos = ImVec2(touchScreeny - imgui_y, imgui_x);
                break;
            case 1:
                io.MousePos = ImVec2(imgui_x, imgui_y);
                break;
            case 2:
                io.MousePos = ImVec2(imgui_y, touchScreenx - imgui_x);
                break;
            case 3:
                io.MousePos = ImVec2(touchScreenx - imgui_x, touchScreeny - imgui_y);
                break;
        }
    } else {
        switch (displayInfo.orientation) {
            case 0:
                io.MousePos = ImVec2(imgui_x, imgui_y);
                // printf("0 -- > press:%d pos: %f | %f
                // \n",io.MouseDown[0],io.MousePos.x,io.MousePos.y);
                break;
            case 1:
                io.MousePos = ImVec2(imgui_y, touchScreenx - imgui_x);
                // printf("1 -- > press:%d pos: %f | %f
                // \n",io.MouseDown[0],io.MousePos.x,io.MousePos.y);
                break;
            case 2:
                io.MousePos = ImVec2(touchScreenx - imgui_x, touchScreeny - imgui_y);
                // printf("2 -- > press:%d pos: %f | %f
                // \n",io.MouseDown[0],io.MousePos.x,io.MousePos.y);
                break;
            case 3:
                io.MousePos = ImVec2(touchScreeny - imgui_y, imgui_x);
                // printf("3 -- > press:%d pos: %f | %f
                // \n",io.MouseDown[0],io.MousePos.x,io.MousePos.y);
                break;
        }
    }
}

// 上传报文
void Upload() {
    int tmpCnt = 0, tmpCnt2 = 0, i, j;
    int size = rand() % 30;
    for (i = 0; i < fdNum; i++) {
        for (j = 0; j < maxF; j++) {
            if (Finger[i][j].isDown) {
                tmpCnt2++;          // 有手指按下了 计数
                if (tmpCnt2 > 10) { // 如果手指大于10了 那不正常 break
                    break;
                }
                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_X;
                event[tmpCnt].value = Finger[i][j].x;
                tmpCnt++;

                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_Y;
                event[tmpCnt].value = Finger[i][j].y;
                tmpCnt++;

                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_POSITION_X;
                event[tmpCnt].value = Finger[i][j].x;
                tmpCnt++;

                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_POSITION_Y;
                event[tmpCnt].value = Finger[i][j].y;
                tmpCnt++;

                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_TOUCH_MAJOR;
                event[tmpCnt].value = size;
                tmpCnt++;

                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_WIDTH_MAJOR;
                event[tmpCnt].value = size;
                tmpCnt++;

                event[tmpCnt].type = EV_ABS;
                event[tmpCnt].code = ABS_MT_TRACKING_ID;
                event[tmpCnt].value = Finger[i][j].id;
                tmpCnt++;

                event[tmpCnt].type = EV_SYN;
                event[tmpCnt].code = SYN_MT_REPORT;
                event[tmpCnt].value = 0;
                tmpCnt++;
            }
        }
    }
    if (tmpCnt == 0) { //
        event[tmpCnt].type = EV_SYN;
        event[tmpCnt].code = SYN_MT_REPORT;
        event[tmpCnt].value = 0;
        tmpCnt++;
    }
    event[tmpCnt].type = EV_SYN;
    event[tmpCnt].code = SYN_REPORT;
    event[tmpCnt].value = 0;
    tmpCnt++;
    // 写报文
    write(nowfd, event, sizeof(struct input_event) * tmpCnt);
}

void Touch_Down(int id, int x, int y) {
    int num = fdNum - 1;
    Finger[num][id].id = (num * 2 + 1) * maxF + id;
    if (bPad) {
        switch (displayInfo.orientation) {
            case 0:
                // y = (Finger[num][id].x - halfOffsetx) / t2sx;
                Finger[num][id].x = (y / t2sx + halfOffsetx);
                // x = touchScreeny - (Finger[num][id].y - halfOffsety) * t2sy;
                Finger[num][id].y = (touchScreeny - x) / t2sy + halfOffsety;
                break;
            case 1:
                Finger[num][id].x = ((x / t2sx) + halfOffsetx);
                Finger[num][id].y = ((y / t2sy) + halfOffsety);
                break;
            case 2:
                // y = touchScreenx - (Finger[num][id].x - halfOffsetx) * t2sx;
                Finger[num][id].x = (touchScreenx - y) / t2sx + halfOffsetx;
                // x = (Finger[num][id].y - halfOffsety) * t2sy;
                Finger[num][id].y = (x / t2sy + halfOffsety);
                break;
            case 3:
                // x = touchScreenx - (Finger[num][id].x - halfOffsetx) * t2sx;
                Finger[num][id].x = ((touchScreenx - x) / t2sx) + halfOffsetx;
                // y = touchScreeny - (Finger[num][id].y - halfOffsety) * t2sy;
                Finger[num][id].y = ((touchScreeny - y) / t2sy) + halfOffsety;
                break;
        }
    } else {
        switch (displayInfo.orientation) {
            case 0:
                Finger[num][id].x = ((x / t2sx) + halfOffsetx);
                Finger[num][id].y = ((y / t2sy) + halfOffsety);
                break;
            case 1:
                // y = touchScreenx - (Finger[num][id].x - halfOffsetx) * t2sx;
                Finger[num][id].x = (touchScreenx - y) / t2sx + halfOffsetx;
                // x = (Finger[num][id].y - halfOffsety) * t2sy;
                Finger[num][id].y = (x / t2sy + halfOffsety);
                break;
            case 2:
                // x = touchScreenx - (Finger[num][id].x - halfOffsetx) * t2sx;
                Finger[num][id].x = ((touchScreenx - x) / t2sx) + halfOffsetx;
                // y = touchScreeny - (Finger[num][id].y - halfOffsety) * t2sy;
                Finger[num][id].y = ((touchScreeny - y) / t2sy) + halfOffsety;
                break;
            case 3:
                // y = (Finger[num][id].x - halfOffsetx) / t2sx;
                Finger[num][id].x = (y / t2sx + halfOffsetx);
                // x = touchScreeny - (Finger[num][id].y - halfOffsety) * t2sy;
                Finger[num][id].y = (touchScreeny - x) / t2sy + halfOffsety;
                break;
        }
    }
    Finger[num][id].isDown = true;
    Upload();
}

void Touch_Move(int id, int x, int y) {
    printf("Touch_Move  x->%d\ty->%d",x,y);
    int num = fdNum - 1;
    Finger[num][id].id = (num * 2 + 1) * maxF + id;
    if (bPad) {
        switch (displayInfo.orientation) {
            case 0:
                Finger[num][id].x = (y / t2sx + halfOffsetx);
                Finger[num][id].y = (touchScreeny - x) / t2sy + halfOffsety;
                break;
            case 1:
                Finger[num][id].x = ((x / t2sx) + halfOffsetx);
                Finger[num][id].y = ((y / t2sy) + halfOffsety);
                break;
            case 2:
                Finger[num][id].x = (touchScreenx - y) / t2sx + halfOffsetx;
                Finger[num][id].y = (x / t2sy + halfOffsety);
                break;
            case 3:
                Finger[num][id].x = ((touchScreenx - x) / t2sx) + halfOffsetx;
                Finger[num][id].y = ((touchScreeny - y) / t2sy) + halfOffsety;
                break;
        }
    } else {
        switch (displayInfo.orientation) {
            case 0:
                Finger[num][id].x = ((x / t2sx) + halfOffsetx);
                Finger[num][id].y = ((y / t2sy) + halfOffsety);
                break;
            case 1:
                Finger[num][id].x = (touchScreenx - y) / t2sx + halfOffsetx;
                Finger[num][id].y = (x / t2sy + halfOffsety);
                break;
            case 2:
                Finger[num][id].x = ((touchScreenx - x) / t2sx) + halfOffsetx;
                Finger[num][id].y = ((touchScreeny - y) / t2sy) + halfOffsety;
                break;
            case 3:
                Finger[num][id].x = (y / t2sx + halfOffsetx);
                Finger[num][id].y = (touchScreeny - x) / t2sy + halfOffsety;
                break;
        }
    }
    Finger[num][id].isDown = true;
    Upload();
}

void Touch_Up(int id) {
    int num = fdNum - 1;
    Finger[num][id].isDown = false;
    Upload();
}

// 生成随机字符串
char *_genRandomString(int length) {
    int flag, i;
    srand((unsigned)time(NULL));
    char *tmpString = (char *)malloc(length * sizeof(char));
    for (i = 0; i < length - 1; i++) {
        flag = rand() % 3;
        switch (flag) {
            case 0:
                tmpString[i] = 'A' + rand() % 26;
                break;
            case 1:
                tmpString[i] = 'a' + rand() % 26;
                break;
            case 2:
                tmpString[i] = '0' + rand() % 10;
                break;
            default:
                tmpString[i] = 'x';
                break;
        }
    }
    tmpString[length - 1] = '\0';
    return tmpString;
}

void *TypeA(void *arg) {
    targ tmp = *(targ *)arg;
    int i = tmp.fdNum;
    float S2TX = tmp.S2TX;
    float S2TY = tmp.S2TY;
    struct input_event ie;
    int latest = 0;
    for (;;) {
        struct input_event iel[32];
        int32_t readSize = read(origfd[i], &iel, sizeof(iel));
        if (readSize <= 0 || (readSize % sizeof(struct input_event)) != 0) {
            continue;
        }
        size_t count = size_t(readSize) / sizeof(struct input_event);
        for (size_t j = 0; j < count; j++) {
            struct input_event ie = iel[j];
            dispatchIMGUITouchEvent(ie);
            if (isOnIMGUI) {
                continue;
            }
            if (ie.code == ABS_MT_SLOT) {
                latest = ie.value;
                continue;
            }
            if (ie.code == ABS_MT_TRACKING_ID) {
                if (ie.value == -1) { // 手指放开了
                    Finger[i][latest].isDown = false;
                } else {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].isDown = true;
                }
                continue;
            }
            if (ie.code == ABS_MT_POSITION_X) {
                Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                Finger[i][latest].x = (int)(ie.value * S2TX);
                Finger[i][latest].isTmpDown = true;
                continue;
            }
            if (ie.code == ABS_MT_POSITION_Y) {
                Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                Finger[i][latest].y = (int)(ie.value * S2TY);
                Finger[i][latest].isTmpDown = true;
                continue;
            }
        }
        if (ie.code == SYN_REPORT) {
            Upload();
            continue;
        }
//        if (ImGui::IsWindowFocused(
//                ImGuiFocusedFlags_AnyWindow)) { // 对IMGUI窗口做判断
//            // 如果在窗口内
//            isOnIMGUI = true;
//            // 手指按下后 如果在IMGUI窗口内 则不会立刻抬起
//            Finger[i][latest].isDown = false;
//            Upload();
//        } else {
//            isOnIMGUI = false;
//        }
    }
    return 0;
}

// 初始化触摸和imgui触摸处理
void handleAndroidTouchAndIMGUI() { // 初始化触摸设置
    char temp[128];
    DIR *dir = opendir("/dev/input/");
    dirent *ptr = NULL;
    int eventCount = 0;
    int touch_num = get_touch_event_num(); // 获取触摸屏的event
    // 遍历/dev/input/event 获取全部的event事件数量
    while ((ptr = readdir(dir)) != NULL) {
        if (strstr(ptr->d_name, "event")) {
            eventCount++;
        }
    }
    struct input_absinfo abs, absX[maxE], absY[maxE];
    int fd, i, tmp1, tmp2;
    int minCnt =
            eventCount + 1; // 因为要判断出minCnt 所以minCnt必须至少比eventCount大1
    fdNum = 0;
    // 遍历全部的event数量 获取到全部的可用触摸设备
    for (i = 0; i <= eventCount; i++) {
        sprintf(temp, "/dev/input/event%d", i);
        fd = open(temp, O_RDWR);
        if (fd) { // 如果fd被打开成功
            uint8_t *bits = NULL;
            ssize_t bits_size = 0;
            int res, j, k;
            bool itmp1 = false, itmp2 = false, itmp3 = false;
            // 获取每个event事件的配置 匹配bit是否有我们的目标 如果都存在
            // 则说明为触摸设备
            while (1) {
                res = ioctl(fd, EVIOCGBIT(EV_ABS, bits_size), bits);
                if (res < bits_size) {
                    break;
                }
                bits_size = res + 16;
                bits = (uint8_t *)realloc(bits, bits_size * 2);
                if (bits == NULL) {
                    printf("获取事件失败\n");
                    exit(0);
                }
            }
            // 获取每个event事件的配置 匹配bit是否有我们的目标 如果都存在
            // 则说明为触摸设备
            for (j = 0; j < res; j++) {
                for (k = 0; k < 8; k++) {
                    if (bits[j] & 1 << k && ioctl(fd, EVIOCGABS(j * 8 + k), &abs) == 0) {
                        if (j * 8 + k == ABS_MT_SLOT) {
                            itmp1 = true;
                            continue;
                        }
                        if (j * 8 + k == ABS_MT_POSITION_X) {
                            itmp2 = true;
                            continue;
                        }
                        if (j * 8 + k == ABS_MT_POSITION_Y) {
                            itmp3 = true;
                            continue;
                        }
                    }
                }
            }
            //
            if (itmp1 && itmp2 && itmp3) {
                tmp1 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX[fdNum]);
                tmp2 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY[fdNum]);
                if (tmp1 == 0 && tmp2 == 0) {
                    // 如果tmp1和tmp2 都是0 则说明本次遍历的event是触摸设备
//                    printf("找到触摸设备: %d\n", i);
                    origfd[fdNum] = fd; // 做一个存储
                    ioctl(fd, EVIOCGRAB,
                          GRAB); // 对原event做屏蔽 以防止和我们创造的event抢接管权
                    // if (i < minCnt) { // 后续会选取最小的minCnt作为伪造设备的根据
                    if (i == touch_num) {
                        // if(i == 10){
                        // 原触摸程序会选取最小的触摸设备文件作为模拟创建的对象
                        // 但是这里为了方便部分多设备文件的分辨率和触摸屏分辨率不一样的情况
                        // 统一模拟创建为触摸屏设备文件
                        // 也就是固定minCnt为touch event
                        screenX =
                                absX[fdNum].maximum; // 获取最小event数的设备文件的screenX和Y
                        // 也就是触摸屏大小
                        screenY = absY[fdNum].maximum;
                        minCnt = i;
                    }
                    fdNum++;
                    if (fdNum >= maxE) { // 如果fd数量大于我们规定的最大数 则不再遍历
                        // 太多线程浪费资源
                        break;
                    }
                }
            } else {
                // 否则关闭fd
                close(fd);
            }
        }
    }
    // 判断minCnt是否有获取成功 后续会选取最小的minCnt作为伪造设备的根据
    if (minCnt > eventCount) {
        printf("获取屏幕驱动失败\n");
        exit(0);
    }
    // 创建uinput设备
    struct uinput_user_dev ui_dev;
    nowfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (nowfd <= 0) {
        printf("打开驱动失败\n");
        exit(0);
    }
    memset(&ui_dev, 0, sizeof(ui_dev));
    strncpy(ui_dev.name, _genRandomString(rand() % 10 + 5), UINPUT_MAX_NAME_SIZE);
    ioctl(nowfd, UI_SET_PHYS, _genRandomString(rand() % 10 + 5));
    ui_dev.id.bustype = 0;
    ui_dev.id.vendor = rand() % 10 + 5;
    ui_dev.id.product = rand() % 10 + 5;
    ui_dev.id.version = rand() % 10 + 5;

    ui_dev.absmin[ABS_MT_POSITION_X] = 0;
    ui_dev.absmax[ABS_MT_POSITION_X] = screenX;
    ui_dev.absmin[ABS_MT_POSITION_Y] = 0;
    ui_dev.absmax[ABS_MT_POSITION_Y] = screenY;
    ioctl(nowfd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    ioctl(nowfd, UI_SET_EVBIT, EV_ABS);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);

    ioctl(nowfd, UI_SET_EVBIT, EV_SYN);

    ioctl(nowfd, UI_SET_ABSBIT, ABS_X);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_Y);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR);

    ui_dev.absmin[ABS_MT_TOUCH_MAJOR] = 0;
    ui_dev.absmax[ABS_MT_TOUCH_MAJOR] = 255;
    ui_dev.absmin[ABS_MT_WIDTH_MAJOR] = 0;
    ui_dev.absmax[ABS_MT_WIDTH_MAJOR] = 255;

    ui_dev.absmin[ABS_X] = 0;
    ui_dev.absmax[ABS_X] = screenX;
    ui_dev.absmin[ABS_Y] = 0;
    ui_dev.absmax[ABS_Y] = screenY;

    ioctl(nowfd, UI_SET_EVBIT, EV_KEY);
    //随机注册
    for (int ii = 0; ii < 41; ii++) {
        int i = rand() % 41;
        if (i == 0) { ioctl(nowfd, UI_SET_ABSBIT, ABS_X); }
        if (i == 1) { ioctl(nowfd, UI_SET_ABSBIT, ABS_Y); }
        if (i == 2) { ioctl(nowfd, UI_SET_ABSBIT, ABS_Z); }

        if (i == 3) { ioctl(nowfd, UI_SET_ABSBIT, ABS_RX); }
        if (i == 4) { ioctl(nowfd, UI_SET_ABSBIT, ABS_RY); }
        if (i == 5) { ioctl(nowfd, UI_SET_ABSBIT, ABS_RZ); }

        if (i == 6) { ioctl(nowfd, UI_SET_ABSBIT, ABS_THROTTLE); }
        if (i == 7) { ioctl(nowfd, UI_SET_ABSBIT, ABS_RUDDER); }
        if (i == 8) { ioctl(nowfd, UI_SET_ABSBIT, ABS_WHEEL); }
        if (i == 9) { ioctl(nowfd, UI_SET_ABSBIT, ABS_GAS); }


        if (i == 10) { ioctl(nowfd, UI_SET_ABSBIT, ABS_BRAKE); }
        if (i == 11) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT0Y); }
        if (i == 12) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT1X); }
        if (i == 13) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT1Y); }
        if (i == 14) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT2X); }
        if (i == 15) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT2Y); }
        if (i == 16) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT3X); }
        if (i == 17) { ioctl(nowfd, UI_SET_ABSBIT, ABS_HAT3Y); }
        if (i == 18) { ioctl(nowfd, UI_SET_ABSBIT, ABS_PRESSURE); }
        if (i == 19) { ioctl(nowfd, UI_SET_ABSBIT, ABS_DISTANCE); }
        if (i == 20) { ioctl(nowfd, UI_SET_ABSBIT, ABS_TILT_X); }
        if (i == 21) { ioctl(nowfd, UI_SET_ABSBIT, ABS_TILT_Y); }
        if (i == 22) { ioctl(nowfd, UI_SET_ABSBIT, ABS_TOOL_WIDTH); }
        if (i == 23) { ioctl(nowfd, UI_SET_ABSBIT, ABS_VOLUME); }
        if (i == 24) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MISC); }

        if (i == 25) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR); }
        if (i == 26) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOUCH_MINOR); }
        if (i == 27) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR); }
        if (i == 28) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_WIDTH_MINOR); }
        if (i == 29) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_ORIENTATION); }
        if (i == 30) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_X); }
        if (i == 31) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_POSITION_Y); }
        if (i == 32) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOOL_TYPE); }
        if (i == 33) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_BLOB_ID); }
        if (i == 34) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID); }
        if (i == 35) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_PRESSURE); }
        if (i == 36) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_DISTANCE); }
        if (i == 37) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOOL_X); }
        if (i == 38) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MT_TOOL_Y); }
        if (i == 39) { ioctl(nowfd, UI_SET_ABSBIT, ABS_MAX); }
        if (i == 40) { ioctl(nowfd, UI_SET_ABSBIT, ABS_CNT); }

    }
    //随机注册
    //ioctl(uinp_fd, UI_SET_KEYBIT, KEY_BACK);
    if (2 == 2) {
        uint8_t *bits = NULL;
        ssize_t bits_size = 0;
        int res, j, k;
        while (1) {
            res = ioctl(origfd[fdNum], EVIOCGBIT(EV_KEY, bits_size), bits);
            if (res < bits_size)
                break;
            bits_size = res + 16;
            bits = (uint8_t *) realloc(bits, bits_size * 2);
            if (bits == NULL)
                exit(1);
        }

        for (j = 0; j < res; j++) {
            for (k = 0; k < 8; k++)
                //printf("%ld",ABS_MT_POSITION_X);
                if (bits[j] & 1 << k) {

                    //struct input_absinfo ABS;
                    //ioctl(FD, EVIOCGABS(j*8+k), &ABS);
                    //ui_dev.absmin[j*8+k] = ABS.minimum;
                    //ui_dev.absmax[j*8+k] = ABS.maximum;
                    if (j * 8 + k < 249 & j * 8 + k > -1) {
                        ioctl(nowfd, UI_SET_KEYBIT, j * 8 + k);
                    }

                }
        }
    }
    write(nowfd, &ui_dev, sizeof(ui_dev));
    if (ioctl(nowfd, UI_DEV_CREATE)) { // 创建触摸设备文件 --> 存在被检测点
        printf("创建驱动失败\n");
        exit(0);
    }

    // 手机有修改分辨率情况的适配
    if (screenX > screenY) {
        bPad = true; // 特殊平板手机判断
    }
    if (bPad) { // 对特殊平板的转化判断
        touchScreenx = init_screen_height;
        touchScreeny = init_screen_width;
        origScreenx = displayInfo.physical_height;
        origScreeny = displayInfo.physical_width;
    } else {
        touchScreenx = init_screen_width;
        touchScreeny = init_screen_height;
        origScreenx = displayInfo.physical_width;
        origScreeny = displayInfo.physical_height;
    }
    // 计算比例 确认是否屏幕分辨率修改
    if (touchScreenx / origScreenx != touchScreeny / origScreeny) {
        float origScale = origScreeny / origScreenx;
        float newScale = touchScreeny / touchScreenx;
        if (newScale > origScale) { // 屏幕分辨率的修改判断 然后计算偏移了多少
            offsetx = (origScreenx - (origScreeny / touchScreeny) * touchScreenx) *
                      (screenX / origScreenx);
        } else {
            offsety = (origScreeny - (origScreenx / touchScreenx) * touchScreeny) *
                      (screenY / origScreeny);
        }
        // 需要减半 因为黑边在上下两边
        halfOffsetx = offsetx * 0.5f;
        halfOffsety = offsety * 0.5f;
    }
    // 计算比率
    t2sx = float(touchScreenx) / (screenX - offsetx);
    t2sy = float(touchScreeny) / (screenY - offsety);

    targ tmp[fdNum];
    for (i = 0; i < fdNum; i++) { // 给每一个触摸设备创建一个触摸处理线程
        tmp[i].fdNum = i;
        // 其余触摸设备的触摸屏大小不一定和minCnt的触摸屏大小完全一致 做比例转化
        tmp[i].S2TX =
                (float)(screenX - offsetx) / (float)(absX[i].maximum - offsetx);
        tmp[i].S2TY =
                (float)(screenY - offsety) / (float)(absY[i].maximum - offsety);
        pthread_create(&touch_loop, NULL, TypeA, &tmp[i]);
    }
    fdNum++;
    // rec
    system("chmod 000 -R /proc/bus/input/*");
    system("chmod 311 -R /sys/devices/virtual/input/*/");
    touchInit = true;
}

void init_android_touch(bool isReadOnly) {
    if (isReadOnly) {
        std::thread *touch_thread = new std::thread(handleIMGUITouchEvent);
        touch_thread->detach();
    } else {
        std::thread *touch_thread = new std::thread(handleAndroidTouchAndIMGUI);
        touch_thread->detach();
    }
}