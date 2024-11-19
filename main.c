#include <windows.h>
#include <gdiplus.h>
#include <stdbool.h>

#include <stdio.h>

// Linked list for button management (Not creating windows for buttons)
typedef struct buttonNode {
    RECT position;
    void (*callback)(HWND);
    struct buttonNode *next;
} buttonNode;

buttonNode *firstnode;
int buttonLength = 0;

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
void pushNewNode(RECT rect, void(* cb)()){
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
    HDC hdc_x = GetDC(hwnd);
    BITMAP bm;
    // get size of original image
    GetObject(bmp, sizeof(BITMAP), &bm);
    StretchBlt(hdc_x, x, y, width, height, copy, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Draw the image with custom width and height (Stretches image)
    DeleteObject(copy);
    ReleaseDC(hwnd, hdc_x);
    DeleteObject(bmp);
}

// Function to set the background of window to be an image (Bitmap)
void setBackgroundImage(HWND hwnd, HDC hdc, char* name){
    RECT windowBounds;
    GetWindowRect(hwnd, &windowBounds);
    int width = windowBounds.right-windowBounds.left;
    int height = windowBounds.bottom - windowBounds.top;
    drawImage(hwnd, hdc, 0, 0, width, height, "./assets/mainbg.bmp");
}

// Function to set background to be static color
void setBackground(HWND hwnd, COLORREF Color, HDC hdc){
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    HBRUSH hbrush;
    hbrush = CreateSolidBrush(Color);
    CreateSolidBrush(Color);
    RECT rect = {0, 0, clientRect.right, clientRect.bottom};
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

// Creates a button and adds it to the buttons linked list
void AddButton(HWND hwnd, HDC hdc, int x, int y, char *text, void (*callback)()){
    int width = 300;
    int height = 80;
    RECT rect = {x, y, x+width, y+height};
    POINT mousePos = getMousePos(hwnd);
    if (pointCollideRect(mousePos, rect)){
        drawImage(hwnd, hdc, x, y, width, height, "./assets/buttonhover.bmp");
    }else{
        drawImage(hwnd, hdc, x, y, width, height, "./assets/button.bmp");
    }
    if (callback == NULL){
        pushNewNode(rect, &printHello);
    }else{
        pushNewNode(rect, callback);
    }
    AddText(hwnd, text, (int)(x+width/2), (int)(y+(height)/2)+13, 26, hdc);
}

// Creates custom circle at the cursor position
void DrawCursor(HWND hwnd, HDC hdc){
    POINT cursorPos = getMousePos(hwnd); 
    drawEllipse(hwnd, hdc, RGB(12, 12, 12), cursorPos.x, cursorPos.y, 5);
    drawEllipse(hwnd, hdc, RGB(35, 35, 35), cursorPos.x, cursorPos.y, 3);
}

// Simple quit window for the quit button
void quitWindow(){
    PostQuitMessage(0);
}

// Create Menu Counter
int currentMenu = 0;

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
LRESULT OnPaint (HWND hwnd){
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    PAINTSTRUCT ps;
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);
    setBackgroundImage(hwnd, hdc, "./assets/mainbg.bmp");
    AddText(hwnd, "POOLS", (int) (windowRect.right/2), (int) (windowRect.bottom*0.12), 84, hdc);
    AddButton(hwnd, hdc, 50, 625, "QUIT", &quitWindow);
    AddButton(hwnd, hdc, 50, 525, "OPTIONS", &options);
    AddButton(hwnd, hdc, 50, (windowRect.bottom*0.15)+100, "SHOP", &shop);
    AddButton(hwnd, hdc, 50, (windowRect.bottom*0.15), "START SESSION", &confirm);
    DrawCursor(hwnd, hdc);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
    EndPaint(hwnd, &ps);
}

// Shop Menu
LRESULT shopMenu(HWND hwnd){
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    PAINTSTRUCT ps;
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);
    setBackground(hwnd, RGB(189, 128, 75), hdc);
    AddText(hwnd, "SHOP", (int)(windowRect.right/2), 100, 64, hdc);
    AddButton(hwnd, hdc, 50, 150, "BACK", &resetCounter);
    DrawCursor(hwnd, hdc);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
    EndPaint(hwnd, &ps);
}

// Options Menu
LRESULT optionsMenu(HWND hwnd){
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    PAINTSTRUCT ps;
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);
    setBackground(hwnd, RGB(189, 128, 75), hdc);
    AddText(hwnd, "OPTIONS", (int)(windowRect.right/2), 100, 64, hdc);
    AddButton(hwnd, hdc, 50, 150, "BACK", &resetCounter);
    DrawCursor(hwnd, hdc);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
    EndPaint(hwnd, &ps);
}

// Starts a session
void start(HWND hwnd){
    ShowWindow(hwnd, SW_HIDE);
}

// Start Menu
LRESULT startMenu(HWND hwnd){
    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    PAINTSTRUCT ps;
    HDC hdc;
    hdc = BeginPaint(hwnd, &ps);
    setBackground(hwnd, RGB(189, 128, 75), hdc);
    AddText(hwnd, "CONFIRM", (int)(windowRect.right/2), 100, 64, hdc);
    AddButton(hwnd, hdc, 50, 150, "BACK", &resetCounter);
    AddButton(hwnd, hdc, 50, windowRect.bottom-100, "START", &start);
    DrawCursor(hwnd, hdc);
    AddText(hwnd, "CREATED BY REMEEDEV", windowRect.right-180, windowRect.bottom-10, 24, hdc);
    EndPaint(hwnd, &ps);
}

// Create array of functions of paint
LRESULT (*menus[])(HWND) = {&OnPaint, &optionsMenu, &shopMenu, &startMenu};

// Handles messages of the main window
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CREATE:
            SetTimer(hwnd, 1, 25, NULL);
            ShowCursor(FALSE);
            break;
        case WM_LBUTTONDOWN:
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
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            free(firstnode);
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            deleteButtonNodes();
            menus[currentMenu](hwnd);
            break;
        case WM_TIMER:
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Creates the main window
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    firstnode = createButtonNode((RECT){-15, -15, -69, -69}, NULL);
    const char class_name[] = "windowClass";
    WNDCLASSEX wc;
    MSG msg;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_HAND);
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
