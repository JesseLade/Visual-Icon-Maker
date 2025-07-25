// main.cpp - Raylib Icon Maker by Demi 💖 PRO MAX (Mobile Edition)
extern "C" {
    #include "raylib.h"
}
// Include necessary headers
#include <TargetConditionals.h>

#include <vector>
#include <stack>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <cmath>
#if defined(__APPLE__) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
  #define PLATFORM "iOS"
#elif defined(__APPLE__) && (!defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE)
  #define PLATFORM "MACOS"
#elif defined(__ANDROID__)
  #define PLATFORM "ANDROID"
#elif defined(_WIN32)
  #define PLATFORM "WINDOWS"
#else
  #define PLATFORM "LINUX"
#endif



const int WIDTH = 600, HEIGHT = 600;
int brushSize = 8;
bool brushIsCircle = true;
int currentFrame = 0;
const int NUM_FRAMES = 4;
Vector2 cameraOffset = { 0, 0 };
bool panning = false;
Vector2 panStart = { 0, 0 };

Color drawColor = BLACK;
std::vector<Color> colorHistory;
std::stack<unsigned char*> undoStack, redoStack;
bool showAI = false;
bool showInfo = false;

enum InputMode { NONE, RGB_MODE, HSV_MODE, HEX_MODE, SAVE_PATH, LOAD_PATH };
InputMode inputMode = NONE;
char inputBuffer[256] = "";
int inputLength = 0;
bool inputActive = false;
bool rubberActive = false;
char savedPath[256] = "";

bool commandKeyboard = true;
Rectangle toggleButton = { 10, 10, 140, 30 };

// === Color Helpers ===
Color HSVtoRGB(float h, float s, float v) {
    float r, g, b;
    int i = int(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return { (unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), 255 };
}

void RGBtoHSV(Color c, float &h, float &s, float &v) {
    float r = c.r / 255.0f, g = c.g / 255.0f, b = c.b / 255.0f;
    float max = std::max(r, std::max(g, b));
    float min = std::min(r, std::min(g, b));
    v = max;
    float d = max - min;
    s = max == 0 ? 0 : d / max;
    if (max == min) h = 0;
    else if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
    else if (max == g) h = (b - r) / d + 2;
    else h = (r - g) / d + 4;
    h /= 6;
}

void AddColor(Color c) {
    colorHistory.insert(colorHistory.begin(), c);
    if (colorHistory.size() > 5) colorHistory.pop_back();
}

void PushUndo(Image frames[]) {
    unsigned char* snap = (unsigned char*)malloc(WIDTH * HEIGHT * 4);
    memcpy(snap, frames[currentFrame].data, WIDTH * HEIGHT * 4);
    undoStack.push(snap);
    while (!redoStack.empty()) {
        free(redoStack.top());
        redoStack.pop();
    }
}

void ApplyBrush(int x, int y, Image frames[]) {
    for (int dy = -brushSize; dy <= brushSize; dy++) {
        for (int dx = -brushSize; dx <= brushSize; dx++) {
            if (brushIsCircle && dx * dx + dy * dy > brushSize * brushSize) continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                ImageDrawPixel(&frames[currentFrame], nx, ny, drawColor);
            }
        }
    }
}

void ProcessInput(Image frames[]) {
    std::istringstream iss(inputBuffer);
    if (inputMode == RGB_MODE) {
        int r, g, b;
        if (iss >> r >> g >> b) {
            drawColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
            AddColor(drawColor);
        }
    } else if (inputMode == HSV_MODE) {
        float h, s, v;
        if (iss >> h >> s >> v) {
            drawColor = HSVtoRGB(h, s, v);
            AddColor(drawColor);
        }
    } else if (inputMode == HEX_MODE) {
        std::string hex;
        iss >> hex;
        if (hex[0] == '#') hex = hex.substr(1);
        if (hex.length() == 3) {
            hex = {hex[0], hex[0], hex[1], hex[1], hex[2], hex[2]};
        }
        if (hex.length() >= 6) {
            try {
                drawColor = {
                    (unsigned char)std::stoi(hex.substr(0, 2), nullptr, 16),
                    (unsigned char)std::stoi(hex.substr(2, 2), nullptr, 16),
                    (unsigned char)std::stoi(hex.substr(4, 2), nullptr, 16),
                    255
                };
                AddColor(drawColor);
            } catch (...) {}
        }
    } else if (inputMode == SAVE_PATH) {
        strcpy(savedPath, inputBuffer);
        ExportImage(frames[currentFrame], savedPath);
    } else if (inputMode == LOAD_PATH) {
        Image loaded = LoadImage(inputBuffer);
        if (loaded.data && loaded.width == WIDTH && loaded.height == HEIGHT) {
            PushUndo(frames);
            UnloadImage(frames[currentFrame]);
            frames[currentFrame] = loaded;
        } else {
            UnloadImage(loaded);
        }
    }
    inputActive = false;
    inputMode = NONE;
    inputBuffer[0] = '\0';
    inputLength = 0;
}

std::vector<Color> GeneratePalette(Color base) {
    float h, s, v;
    RGBtoHSV(base, h, s, v);
    std::vector<Color> palette;
    for (int i = 0; i < 5; i++) {
        float newHue = fmod(h + i * 0.1f, 1.0f);
        palette.push_back(HSVtoRGB(newHue, s, v));
    }
    return palette;
}

void DrawColorWheel(int cx, int cy, int radius) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float dist = sqrtf(x * x + y * y);
            if (dist <= radius) {
                float angle = atan2f(y, x);
                if (angle < 0) angle += 2 * PI;
                float hue = angle / (2 * PI);
                Color c = HSVtoRGB(hue, 1, 1);
                DrawPixel(cx + x, cy + y, c);
            }
        }
    }
}

bool DrawVirtualKey(Rectangle rect, const char* label) {
    DrawRectangleRec(rect, LIGHTGRAY);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);
    DrawText(label, rect.x + 6, rect.y + 4, 18, BLACK);
    Vector2 mouse = GetMousePosition();
    return IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rect);
}

void DrawCommandKeyboard(Image frames[]) {
    const char* keys[] = { "R", "C", "H", "S", "L", "X", "Z", "Y", "B", "[", "]", "N", "P", "TAB", "I",  };
    int numKeys = sizeof(keys)/sizeof(keys[0]);
    int x = 10, y = HEIGHT - 100;
    for (int i = 0; i < numKeys; i++) {
        Rectangle key = { x + i * 34.0f, (float)y, 30, 30 };
        if (DrawVirtualKey(key, keys[i])) {
            const char* k = keys[i];
            if (strcmp(k, "R") == 0) { inputActive = true; inputMode = RGB_MODE; }
            else if (strcmp(k, "C") == 0) { inputActive = true; inputMode = HSV_MODE; }
            else if (strcmp(k, "H") == 0) { inputActive = true; inputMode = HEX_MODE; }
            else if (strcmp(k, "S") == 0) { inputActive = true; inputMode = SAVE_PATH; }
            else if (strcmp(k, "L") == 0) { inputActive = true; inputMode = LOAD_PATH; }
            else if (strcmp(k, "X") == 0) { PushUndo(frames); frames[currentFrame] = GenImageColor(WIDTH, HEIGHT, WHITE); }
            else if (strcmp(k, "Z") == 0 && !undoStack.empty()) {
                unsigned char* last = undoStack.top(); undoStack.pop();
                unsigned char* redoSnap = (unsigned char*)malloc(WIDTH * HEIGHT * 4);
                memcpy(redoSnap, frames[currentFrame].data, WIDTH * HEIGHT * 4);
                redoStack.push(redoSnap);
                memcpy(frames[currentFrame].data, last, WIDTH * HEIGHT * 4);
                free(last);
            }
            else if (strcmp(k, "Y") == 0 && !redoStack.empty()) {
                unsigned char* redoSnap = redoStack.top(); redoStack.pop();
                PushUndo(frames);
                memcpy(frames[currentFrame].data, redoSnap, WIDTH * HEIGHT * 4);
                free(redoSnap);
            }
            else if (strcmp(k, "B") == 0) brushIsCircle = !brushIsCircle;
            else if (strcmp(k, "[") == 0) brushSize = std::max(1, brushSize - 1);
            else if (strcmp(k, "]") == 0) brushSize = std::min(100, brushSize + 1);
            else if (strcmp(k, "N") == 0) currentFrame = (currentFrame + 1) % NUM_FRAMES;
            else if (strcmp(k, "P") == 0) currentFrame = (currentFrame - 1 + NUM_FRAMES) % NUM_FRAMES;
            else if (strcmp(k, "TAB") == 0) {
                rubberActive = !rubberActive;
                SetMouseCursor(rubberActive ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
            }
            else if (strcmp(k, "I") == 0) showInfo = !showInfo;
        }
    }
}

void DrawInputKeyboard(Image frames[]) {
    const char* keys = "1234567890QWERTYUIOPASDFGHJKLZXCVBNM#./-;_+=!@;";
    int len = strlen(keys);
    int x = 10, y = HEIGHT - 130;
    for (int i = 0; i < len; i++) {
        Rectangle key = { x + (i % 10) * 32.0f, y + (i / 10) * 36.0f, 30, 30 };
        

        char label[2] = { keys[i], '\0' };
        if (DrawVirtualKey(key, label)) {
            if (inputLength < 255) {
                inputBuffer[inputLength++] = keys[i];
                inputBuffer[inputLength] = '\0';
            }
        }
    }
    // HANDLE BACKSPACE, ENTER, DOT, SLASH, DASH, SEMICOLON, UNDERSCORE, PLUS, EQUALS, EXCLAMATION MARK AND AT SIGN
    Rectangle dash = { 360, static_cast<float>(y + 80), 60, 30 };
    if (DrawVirtualKey(dash, "-")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '-';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle semicolon = { 420, static_cast<float>(y + 80), 30, 30 };
    if (DrawVirtualKey(semicolon, ";")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = ';';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle underscore = { 360, static_cast<float>(y + 120), 60, 30 };
    if (DrawVirtualKey(underscore, "_")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '_';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle plus = { 420, static_cast<float>(y + 120), 30, 30 };
    if (DrawVirtualKey(plus, "+")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '+';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle exclamation = { 360, static_cast<float>(y + 160), 60, 30 };
    if (DrawVirtualKey(exclamation, "!")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '!';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle atSign = { 420, static_cast<float>(y + 160), 30, 30 };
    if (DrawVirtualKey(atSign, "@")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '@';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle bksp = { 360, static_cast<float>(y), 60, 30 };
    if (DrawVirtualKey(bksp, "<--") && inputLength > 0) inputBuffer[--inputLength] = '\0';

    Rectangle enter = { 360, static_cast<float>(y + 40), 60, 30 };
    if (DrawVirtualKey(enter, "OK")) ProcessInput(frames);
    Rectangle dot = { 420, static_cast<float>(y), 30, 30 };
    if (DrawVirtualKey(dot, ".")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '.';
            inputBuffer[inputLength] = '\0';
        }
    }
    Rectangle slash = { 420, static_cast<float>(y + 40), 30, 30 };
    if (DrawVirtualKey(slash, "/")) {
        if (inputLength < 255) {
            inputBuffer[inputLength++] = '/';
            inputBuffer[inputLength] = '\0';
        }
    }
}

int main() {
    // Initialize raylib

    InitWindow(WIDTH, HEIGHT, "Visual Icon Maker");
    SetTargetFPS(60);
    // let them scroll on the screen
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    // scrolling
    
    

    Image frames[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) frames[i] = GenImageColor(WIDTH, HEIGHT, WHITE);
    Texture2D canvas = LoadTextureFromImage(frames[currentFrame]);

    bool drawing = false;
    while (!WindowShouldClose()) {
        int mx = GetMouseX(), my = GetMouseY();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec({(float)mx, (float)my}, toggleButton)) {
            commandKeyboard = !commandKeyboard;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !inputActive && !CheckCollisionPointRec({(float)mx, (float)my}, toggleButton)) {
            PushUndo(frames);
            drawing = true;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) drawing = false;

        if (drawing && !inputActive) {
            Color prev = drawColor;
            if (rubberActive) drawColor = WHITE;
            ApplyBrush(mx, my, frames);
            drawColor = prev;
        }

        UpdateTexture(canvas, frames[currentFrame].data);
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTexture(canvas, 0, 0, WHITE);

        DrawRectangleRec(toggleButton, commandKeyboard ? DARKGRAY : GRAY);
        DrawText(commandKeyboard ? "[Cmd Mode]" : "[Input Mode]", 20, 15, 14, WHITE);

        for (int i = 0; i < colorHistory.size(); i++) {
            DrawRectangle(10 + i * 30, HEIGHT - 40, 28, 28, colorHistory[i]);
        }

        for (int dy = -brushSize; dy <= brushSize; dy++)
            for (int dx = -brushSize; dx <= brushSize; dx++)
                if (!brushIsCircle || dx * dx + dy * dy <= brushSize * brushSize)
                    DrawPixel(mx + dx, my + dy, drawColor);

        DrawColorWheel(WIDTH - 100, HEIGHT - 120, 50);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int dx = mx - (WIDTH - 100), dy = my - (HEIGHT - 120);
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= 50) {
                float angle = atan2f(dy, dx);
                if (angle < 0) angle += 2 * PI;
                float hue = angle / (2 * PI);
                drawColor = HSVtoRGB(hue, 1, 1);
                AddColor(drawColor);
            }
        }

        std::vector<Color> palette = GeneratePalette(drawColor);
        for (int i = 0; i < palette.size(); i++) {
            int x = WIDTH - 160 + i * 30, y = HEIGHT - 60;
            DrawRectangle(x, y, 28, 28, palette[i]);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
                mx >= x && mx <= x + 28 && my >= y && my <= y + 28) {
                drawColor = palette[i];
                AddColor(drawColor);
            }
        }

        if (inputActive) {
            DrawRectangle(50, HEIGHT / 2 - 20, 400, 40, LIGHTGRAY);
            DrawRectangleLines(50, HEIGHT / 2 - 20, 400, 40, DARKGRAY);
            DrawText(inputBuffer, 60, HEIGHT / 2 - 10, 20, BLACK);
        }
              // Then at the bottom:
              std::string credits = std::string("main.cpp - Raylib Icon Maker by Demi 💖 PRO MAX") +
              " (" + PLATFORM + ")";
                      DrawText(credits.c_str(), 10, HEIGHT - 30, 10, GRAY);

        if (commandKeyboard && !inputActive) {
            DrawCommandKeyboard(frames);
        } else if (inputActive && !commandKeyboard) {
            DrawInputKeyboard(frames);
        }

        if (showInfo) {
            DrawRectangle(60, 60, WIDTH - 120, HEIGHT - 120, Fade(LIGHTGRAY, 0.95f));
            DrawRectangleLines(60, 60, WIDTH - 120, HEIGHT - 120, GRAY);
            int x = 80, y = 80;
            DrawText("🎨 Mobile IconMaker Controls:", x, y, 24, BLACK); y += 40;
            DrawText("LMB: Draw | TAB: Erase", x, y, 20, DARKGRAY); y += 30;
            DrawText("R: RGB | C: HSV | H: HEX", x, y, 20, DARKGRAY); y += 30;
            DrawText("S: Save Path | L: Load Path", x, y, 20, DARKGRAY); y += 30;
            DrawText("[ / ]: Brush Size | B: Shape", x, y, 20, DARKGRAY); y += 30;
            DrawText("Z/Y: Undo/Redo | X: Clear", x, y, 20, DARKGRAY); y += 30;
            DrawText("N/P: Frames | I: Info", x, y, 20, DARKGRAY); y += 30;
        }

        EndDrawing();
    }

    UnloadTexture(canvas);
    for (int i = 0; i < NUM_FRAMES; i++) UnloadImage(frames[i]);
    while (!undoStack.empty()) { free(undoStack.top()); undoStack.pop(); }
    while (!redoStack.empty()) { free(redoStack.top()); redoStack.pop(); }

    CloseWindow();
}
// main.cpp - Raylib Icon Maker by Demi 💖 PRO MAX - ANDROID