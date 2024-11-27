#include <windows.h>
#include <gdiplus.h>
#include <stdbool.h>
#include <string.h>

#include <stdio.h>

// Linked list for button management (Not creating windows for buttons)
typedef struct InputNode {
    RECT position;
    char **text;
    bool numeric;
    struct InputNode *next;
} InputNode;

// Stores the position of currently being written text
InputNode *currentTyping;

// Text that stores the amount of time to focus in minutes
char *timeFilling = "";
int timeLeft;
int timerInterval = 50;
int timerCounter = 0;

bool drawing = false;

// Default text when not entering input
char *nun = "";

// Deletes all inputs in the linked list, freeing the space
void deleteInputNodes(){
    InputNode *elem = currentTyping;
    elem = elem->next;
    while (elem){
        InputNode *prev = elem;
        elem = elem->next;
        free(prev);
    }
    currentTyping->next = NULL;
}
// Function to create a basic node
InputNode *createInputNode(RECT rect, char **content, bool numeric){
    InputNode *node = (InputNode *)malloc(sizeof(InputNode));
    node->position = rect;
    node->text = content;
    node->numeric = numeric;
    node->next = NULL;
    return node;
}

// Creates basic node and pushes it to the end of the linked list
void pushNewInput(RECT rect, char *content[100], bool numeric){
    InputNode *node = createInputNode(rect, content, numeric);
    InputNode *elem = currentTyping;
    while (elem->next){
        elem = elem->next;
    }
    elem->next = node;
}

// Linked list for button management (Not creating windows for buttons)
typedef struct buttonNode {
    RECT position;
    void (*callback)(HWND);
    struct buttonNode *next;
} buttonNode;

buttonNode *firstnode;
int buttonLength = 0;

// Deletes all buttons in the linked list, freeing the space
void deleteButtonNodes(){
    buttonNode *elem = firstnode;
    elem = elem->next;
    while (elem){
        buttonNode *prev = elem;
        elem = elem->next;
        free(prev);
        buttonLength--;
    }
    firstnode->next = NULL;
}
// Function to create a basic node
buttonNode *createButtonNode(RECT rect, void (*cb)()){
    buttonNode *node = (buttonNode *)malloc(sizeof(buttonNode));
    node->position = rect;
    node->callback = cb;
    node->next = NULL;
    buttonLength++;
    return node;
}

// Creates basic node and pushes it to the end of the linked list
void pushNewButton(RECT rect, void(* cb)()){
    buttonNode *node = createButtonNode(rect, cb);
    buttonNode *elem = firstnode;
    while (elem->next){
        elem = elem->next;
    }
    elem->next = node;
}

// Function to get position of mouse relative to the window
POINT getMousePos(HWND hwnd){
    POINT mousePos;
    GetCursorPos(&mousePos);
    RECT windowBounds;
    GetWindowRect(hwnd, &windowBounds);
    RECT clientBounds;
    GetClientRect(hwnd, &clientBounds);
    int barSize = windowBounds.bottom - windowBounds.top - clientBounds.bottom + clientBounds.top;
    POINT relativeToWindow = {mousePos.x-windowBounds.left, mousePos.y-windowBounds.top-barSize};
    if (relativeToWindow.x < 0 || relativeToWindow.x > windowBounds.right || relativeToWindow.y < 0 || relativeToWindow.y > clientBounds.bottom){
        POINT out = {-10, -10};
        return out;
    }
    return relativeToWindow;
}

// Function to draw image at position with set height and width
void drawImage(HWND hwnd, HDC hdc, int x, int y, int width, int height, char* name){
    HBITMAP bmp = LoadImage(NULL, (LPCSTR)name, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    HDC copy = CreateCompatibleDC(NULL);
    SelectObject(copy, bmp);
    BITMAP bm;
    // get size of original image
    GetObject(bmp, sizeof(BITMAP), &bm);
    StretchBlt(hdc, x, y, width, height, copy, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Draw the image with custom width and height (Stretches image)
    DeleteObject(copy);
    DeleteObject(bmp);
}

// Create Menu Counter
int currentMenu = 0;

// Add a menu buffer to save current menu
int menuBuffer = -1;

// Function to set the background of window to be an image (Bitmap)
void setBackgroundImage(HWND hwnd, HDC hdc, char* name){
    if (currentMenu == menuBuffer && timeLeft == 0){
        return;
    }else{
        menuBuffer = currentMenu;
    }
    RECT windowBounds;
    GetWindowRect(hwnd, &windowBounds);
    int width = windowBounds.right-windowBounds.left;
    int height = windowBounds.bottom - windowBounds.top;
    drawImage(hwnd, hdc, 0, 0, width, height, "./assets/mainbg.bmp");
}

// Function to set background to be static color
void setBackground(HWND hwnd, COLORREF Color, HDC hdc){
    if (currentMenu == menuBuffer && timeLeft == 0){
        return;
    }else{
        menuBuffer = currentMenu;
    }
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    HBRUSH hbrush;
    hbrush = CreateSolidBrush(Color);
    CreateSolidBrush(Color);
    RECT rect = {clientRect.left, clientRect.top, clientRect.right, clientRect.bottom};
    FillRect(hdc, &rect, hbrush);
    DeleteObject(hbrush);
}

// Function to draw a circle with center point and radius
void drawEllipse(HWND hwnd, HDC hdc, COLORREF Color, int x, int y, int radius){
    RECT pos = {x - radius, y-radius, x+radius, y+radius};
    HBRUSH hbrush;
    hbrush = CreateSolidBrush(Color);
    SelectObject(hdc, hbrush);
    Ellipse(hdc, pos.left, pos.top, pos.right, pos.bottom);
    DeleteObject(hbrush);
}

// Add text in the custom font in assets
void AddText(HWND hwnd, char* text, int x, int y,int fontSize, HDC hdc){
    RECT rect;
    int nResults = AddFontResource("./assets/pixel.ttf");
    HFONT hfont = CreateFont(fontSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "pixel");
    SetRect(&rect, x, y, x+1, y+1);
    SetTextColor(hdc, RGB(255, 255, 255));
    SetTextAlign(hdc, TA_BASELINE | TA_CENTER);
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hfont);
    DrawText(hdc, text, -1, &rect, DT_NOCLIP);
    RemoveFontResource("./assets/pixel.ttf");
    DeleteObject(hfont);
}

// Checks if a point is inside of a rect
bool pointCollideRect(POINT point, RECT rect){
    bool properX = point.x >= rect.left && point.x <= rect.right;
    bool properY = point.y >= rect.top && point.y <= rect.bottom;
    return properX && properY;
}

// Default button action, will delete later
void printHello(){
    printf("A button was clicked\n");
}

// Creates a button and adds it to the buttons linked list
void AddButton(HWND hwnd, HDC hdc, int x, int y, char *text, void (*callback)()){
    int widthPerCharacter = 25;
    int width;
    width = widthPerCharacter*(strlen(text)+1);
    int height = 80;
    RECT rect = {x, y, x+width, y+height};
    POINT mousePos = getMousePos(hwnd);
    if (pointCollideRect(mousePos, rect)){
        drawImage(hwnd, hdc, x, y, width, height, "./assets/buttonhover.bmp");
    }else{
        drawImage(hwnd, hdc, x, y, width, height, "./assets/button.bmp");
    }
    if (callback == NULL){
        pushNewButton(rect, &printHello);
    }else{
        pushNewButton(rect, callback);
    }
    AddText(hwnd, text, (int)(x+width/2), (int)(y+(height)/2)+13, 26, hdc);
}

// Creates a button and adds it to the buttons linked list
void AddInput(HWND hwnd, HDC hdc, int x, int y, char *placeholder, bool numeric, char **content){
    int widthPerCharacter = 25;
    int width;
    //if (content != currentTyping->text){
    width = widthPerCharacter*strlen(placeholder);
    //}
    //else{
    //    width = widthPerCharacter*(strlen(*content)+1);
    //}
    int height = 80;
    RECT rect = {x, y, x+width, y+height};
    POINT mousePos = getMousePos(hwnd);
    pushNewInput(rect, content, numeric);
    if (pointCollideRect(mousePos, rect) || currentTyping->text == content){
        drawImage(hwnd, hdc, x, y, width, height, "./assets/text-selected.bmp");
    }else{
        drawImage(hwnd, hdc, x, y, width, height, "./assets/text.bmp");
    }
    if (strlen(*content) == 0 && currentTyping->text != content){
        AddText(hwnd, placeholder, (int)(x+width/2), (int)(y+(height)/2)+13, 26, hdc);
    }else{
        AddText(hwnd, *content, (int)(x+width/2), (int)(y+(height)/2)+13, 26, hdc);
    }
}

// Simple quit window for the quit button
void quitWindow(){
    PostQuitMessage(0);
}

// Reset counter
void resetCounter(HWND _){
    currentMenu = 0;
}

// Functions to change menu to set menu
void options(HWND _){
    currentMenu = 1;
}

void shop(HWND _){
    currentMenu = 2;
}

void confirm(HWND _){
    currentMenu = 3;
}

// Draws all of the elements of the main menu
LRESULT OnPaint (HWND hwnd, PAINTSTRUCT ps, HDC hdc, RECT windowRect){
    setBackgroundImage(hwnd, hdc, "./assets/mainbg.bmp");
    AddText(hwnd, "POOLS", (int) (windowRect.right/2), (int) (windowRect.bottom*0.12), 84, hdc);
    AddButton(hwnd, hdc, 50, 625, "QUIT", &quitWindow);
    AddButton(hwnd, hdc, 50, 525, "OPTIONS", &options);
    AddButton(hwnd, hdc, 50, (windowRect.bottom*0.15)+100, "SHOP", &shop);
    AddButton(hwnd, hdc, 50, (windowRect.bottom*0.15), "START SESSION", &confirm);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
}

// Shop Menu
LRESULT shopMenu(HWND hwnd, PAINTSTRUCT ps, HDC hdc, RECT windowRect){
    setBackground(hwnd, RGB(189, 128, 75), hdc);
    AddText(hwnd, "SHOP", (int)(windowRect.right/2), 100, 64, hdc);
    AddButton(hwnd, hdc, 50, 150, "BACK", &resetCounter);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
}

// Options Menu
LRESULT optionsMenu(HWND hwnd, PAINTSTRUCT ps, HDC hdc, RECT windowRect){
    setBackground(hwnd, RGB(189, 128, 75), hdc);
    AddText(hwnd, "OPTIONS", (int)(windowRect.right/2), 100, 64, hdc);
    AddButton(hwnd, hdc, 50, 150, "BACK", &resetCounter);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
}

HWND *mainWin;

// exits filling menu and goes back to main menu
void quitPool(HWND hwnd){
    DestroyWindow(hwnd);
    ShowWindow(*mainWin, SW_NORMAL);
}

// Draw of secondary window
LRESULT invisMenu(HWND hwnd){
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    PAINTSTRUCT ps;
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);
    setBackground(hwnd, RGB(0, 0, 0), hdc);
    POINT mousePos = getMousePos(hwnd);
    if (pointCollideRect(mousePos, (RECT){windowRect.right-400, windowRect.bottom-225, windowRect.right, windowRect.bottom})){
        AddButton(hwnd, hdc, windowRect.right-130, windowRect.bottom-125, "QUIT", &quitPool);
        AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-100, windowRect.bottom-10, 12, hdc);
    }
    int minutes = (int)(timeLeft/60);
    int seconds = (int)(timeLeft%60);
    char minutesString[5];
    char secondsString[5];
    if (minutes < 10){
        sprintf(minutesString, "0%d", minutes);
    }else{
        sprintf(minutesString, "%d", minutes);
    }
    if (seconds < 10){
        sprintf(secondsString, "0%d", seconds);
    }else{
        sprintf(secondsString, "%d", seconds);
    }
    char timeString[16];
    sprintf(timeString, "%s:%s", minutesString, secondsString);
    AddText(hwnd, timeString, windowRect.right-125, windowRect.top+50, 56, hdc);
    EndPaint(hwnd, &ps);
}

// Handle a button click
void buttonClick(HWND hwnd){
    buttonNode *elem = firstnode;
    elem = elem->next;
    POINT mousePos = getMousePos(hwnd);
    int counter = 0;
    while (elem){
        if (pointCollideRect(mousePos, elem->position)){
            elem->callback(hwnd);
        }
        elem = elem->next;
        counter++;
    }
}

// Handles messages of the secondary window
LRESULT CALLBACK XtrProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CREATE:
            SetTimer(hwnd, 1, timerInterval, NULL);
            SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            ShowWindow(*mainWin, SW_NORMAL);
            break;
        case WM_LBUTTONDOWN:
            buttonClick(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            invisMenu(hwnd);
            break;
        case WM_TIMER:
            timerCounter++;
            if (timerCounter >= 1000/timerInterval){
                timeLeft--;
                timerCounter = 0;
            }
            if (timeLeft == 0){
                quitPool(hwnd);
                break;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Starts a session
void start(HWND hwnd){
    if (strlen(timeFilling) == 0 || atoi(timeFilling) == 0){
        timeFilling = "";
        return;
    }
    ShowWindow(hwnd, SW_HIDE);
    timeLeft = 60*atoi(timeFilling);
    mainWin = &hwnd;
    WNDCLASSEX wc;
    HWND newWnd;
    MSG msg;
    const char *className = "invis";
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = XtrProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hInstance = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)){
    }

    RECT size;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &size, 0);

    newWnd = CreateWindowEx(
        WS_EX_COMPOSITED | WS_EX_TOPMOST | WS_EX_LAYERED,
        className,
        "Pools",
        WS_BORDER,
        0, 0,size.right-size.left, size.bottom-size.top,
        NULL,
        NULL,
        NULL,
        NULL
    );
    
    if (newWnd == NULL){
        MessageBox(NULL, "No window for u", "sad", MB_ICONEXCLAMATION | MB_OK);
    }

    SetWindowLong(newWnd, GWL_STYLE, 0);

    ShowWindow(newWnd, SW_NORMAL);
    UpdateWindow(newWnd);
    while(GetMessage(&msg, NULL, 0, 0) > 0){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Convert int to char *
char * convertInt(int num){
    int length = 0;
    int lengthNum = num;
    while (lengthNum != 0){
        length++;
        lengthNum = (int)(lengthNum - (lengthNum%10))/10;
    }
    char assignation[10] = "0123456789";
    char *tempString = malloc(sizeof(char) * (length+1));
    int count = 0;
    while(count != length){
        *((tempString+length-1)-(count++)) = (char)(assignation[num%10]);
        num = (int)(num - (num%10))/10;
    }
    *(tempString+length) = '\0';
    return tempString;
}

// Increase the timerFilling by ten 
void increaseTime(HWND _){
    if (strlen(timeFilling) == 0){
        timeFilling = "10";
        return;
    }
    int temp = atoi(timeFilling) + 10;
    timeFilling = convertInt(temp);
}

// Decrease the timerFilling by ten 
void decreaseTime(HWND _){
    if (strlen(timeFilling) == 0){
        timeFilling = "10";
        return;
    }
    if (atoi(timeFilling) < 10){
        timeFilling = "";
        return;
    }
    int temp = atoi(timeFilling) - 10;
    timeFilling = convertInt(temp);
}

// Start Menu
LRESULT startMenu(HWND hwnd, PAINTSTRUCT ps, HDC hdc, RECT windowRect){
    drawing = true;
    setBackground(hwnd, RGB(189, 128, 75), hdc);
    AddButton(hwnd, hdc, 50, 50, "BACK", &resetCounter);
    AddInput(hwnd, hdc, 50, 150, "TIME (MINUTES)", true, &timeFilling);
    AddButton(hwnd, hdc, 50, 250, "+", &increaseTime);
    AddButton(hwnd, hdc, 125, 250, "-", &decreaseTime);
    AddButton(hwnd, hdc, 50, windowRect.bottom-100, "START", &start);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
    drawing = false;
}

// Handle an input click
void inputClick(HWND hwnd){
    InputNode *elem = currentTyping;
    elem = elem->next;
    POINT mousePos = getMousePos(hwnd);
    while (elem){
        if (pointCollideRect(mousePos, elem->position)){
            currentTyping = elem;
            return;
        }
        elem = elem->next;
    }
    currentTyping->text = &nun;
    return;
}

// Create array of functions of paint
LRESULT (*menus[])(HWND, PAINTSTRUCT, HDC, RECT) = {&OnPaint, &optionsMenu, &shopMenu, &startMenu};

char * appendChar(char * source, char end){
    char *copy = malloc(sizeof(char)*(strlen(source)+2));
    char *t;
    int count = 0;
    for (t = source; *t != '\0'; t++){
        *(copy+count++) = *t;
    }
    *(copy+count++) = end;
    *(copy+count) = '\0';
    return copy;
}

char * removeLastChar(char * source, char end){
    if (strlen(source) == 0){
        return source;
    }
    char *copy = malloc(sizeof(char)*(strlen(source)));
    char *t;
    int count = 0;
    for (t = source; *t != '\0'; t++){
        *(copy+count++) = *t;
    }
    *(copy+(--count)) = '\0';
    return copy;
}


// Handles messages of the main window
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CREATE:
            SetTimer(hwnd, 1, 25, NULL);
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_LBUTTONDOWN:
            buttonClick(hwnd);
            inputClick(hwnd);
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            free(firstnode);
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            RECT windowRect;
            GetClientRect(hwnd, &windowRect);
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(hwnd, &ps);
            deleteButtonNodes();
            deleteInputNodes();
            menus[currentMenu](hwnd, ps, hdc, windowRect);
            ReleaseDC(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        case WM_TIMER:
            if (!drawing){
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case WM_CHAR:
            if (strlen(*(currentTyping->text)) > 16){
                break;
            }
            if (((int) wParam >= 97 && (int) wParam <= 122 && currentTyping->numeric == false) || ((int) wParam >= 48 && (int) wParam <= 57)){
                char * newText = appendChar(*(currentTyping->text), (char)wParam);
                *(currentTyping->text) = newText;
            }
            if ((int) wParam == 8){
                char * newText = removeLastChar(*(currentTyping->text), (char)wParam);
                *(currentTyping->text) = newText;
            }
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Creates the main window
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    firstnode = createButtonNode((RECT){-15, -15, -69, -69}, NULL);
    currentTyping = createInputNode((RECT){-15, -15, -69, -69}, &nun, false);
    const char class_name[] = "windowClass";
    WNDCLASSEX wc;
    MSG msg;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpszClassName = class_name;
    wc.hbrBackground = (HBRUSH)(0);
    wc.lpszMenuName = NULL;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)){
        MessageBox(NULL, "Window Registration Failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd;
    hwnd = CreateWindowEx(
        WS_EX_COMPOSITED | WS_EX_TOPMOST | WS_EX_WINDOWEDGE,
        class_name,
        "Pools",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT,800, 800,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    
    if (hwnd == NULL){
        MessageBox(NULL, "Window not loaded", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0)>0){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
