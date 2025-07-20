// main.cpp - Raylib Icon Maker by Demi 💖 PRO MAX (with GUI Text Input + Color Wheel + Palette)

#include "raylib.h"
#include <vector>
#include <stack>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <cmath>
#include "tinyfiledialogs.h"
#if defined(__APPLE__)
#define PLATFORM "MACOS"
#elif defined(_WIN32)
#define PLATFORM "WINDOWS"
#else
#define PLATFORM "LINUX"
#endif


const int WIDTH = 512, HEIGHT = 512;
int brushSize = 8;
bool brushIsCircle = true;
int currentFrame = 0;
const int NUM_FRAMES = 4;

Color drawColor = BLACK;
std::vector<Color> colorHistory;
std::stack<unsigned char*> undoStack, redoStack;
bool showAI = false;
bool showInfo = false;


enum InputMode { NONE, RGB_MODE, HSV_MODE, HEX_MODE};
InputMode inputMode = NONE;
char inputBuffer[64] = "";
int inputLength = 0;
bool inputActive = false;
//NEW RUBBER
bool rubberActive = false;  

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
            if (dx * dx + dy * dy > brushSize * brushSize && brushIsCircle) continue;
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
            drawColor = {(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};
            AddColor(drawColor);
        }
    } else if (inputMode == HSV_MODE) {
        float h, s, v;
        if (iss >> h >> s >> v) {
            drawColor = HSVtoRGB(h, s, v);
            AddColor(drawColor);
        }
    }  else if (inputMode == HEX_MODE) {
        std::string hex;
        iss >> hex;
        if (hex[0] == '#') hex = hex.substr(1);
        if (hex.length() == 3) {
            std::string expanded;
            for (char c : hex) {
                expanded += c;
                expanded += c;
            }
            hex = expanded;
        }
        if (hex.length() == 6 || hex.length() == 8) {
            try {
                unsigned char r = std::stoi(hex.substr(0, 2), nullptr, 16);
                unsigned char g = std::stoi(hex.substr(2, 2), nullptr, 16);
                unsigned char b = std::stoi(hex.substr(4, 2), nullptr, 16);
                unsigned char a = 255;
                if (hex.length() == 8) {
                    a = std::stoi(hex.substr(6, 2), nullptr, 16);
                }
                drawColor = {r, g, b, a};
                AddColor(drawColor);
            } catch (...) {}
        }
    }
    inputActive = false;
    inputBuffer[0] = '\0';
    inputLength = 0;
    inputMode = NONE;
}

// NEW: Generates a palette of 5 harmonious colors
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

// NEW: Draws a basic color wheel
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

int main() {
    InitWindow(WIDTH, HEIGHT, "Visual Icon Maker");
    SetTargetFPS(60);

    Image frames[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) frames[i] = GenImageColor(WIDTH, HEIGHT, WHITE);
    Texture2D canvas = LoadTextureFromImage(frames[currentFrame]);

    bool drawing = false;

    while (!WindowShouldClose()) {
        int mx = GetMouseX(), my = GetMouseY();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !inputActive) {
            PushUndo(frames);
            drawing = true;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) drawing = false;
        if (drawing && !inputActive) {
            Color originalColor = drawColor;
            if (rubberActive) drawColor = WHITE;  // Erase mode
            ApplyBrush(mx, my, frames);
            drawColor = originalColor;            // Restore original
        }
        

        if (!inputActive) {
            if (IsKeyPressed(KEY_R)) { inputActive = true; inputMode = RGB_MODE; }
            else if (IsKeyPressed(KEY_C)) { inputActive = true; inputMode = HSV_MODE; }
            else if (IsKeyPressed(KEY_H)) { inputActive = true; inputMode = HEX_MODE; }


        }

        if (inputActive) {
            int key = GetCharPressed();
            while (key > 0) {
                if (inputLength < 63 && (isprint(key) || key == '#')) {
                    inputBuffer[inputLength++] = (char)key;
                    inputBuffer[inputLength] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && inputLength > 0) inputBuffer[--inputLength] = '\0';
            if (IsKeyPressed(KEY_ENTER)) ProcessInput(frames);
        }

        if (!inputActive) {
            if (IsKeyPressed(KEY_X)) { PushUndo(frames); frames[currentFrame] = GenImageColor(WIDTH, HEIGHT, WHITE); }
            if (IsKeyPressed(KEY_Z) && !undoStack.empty()) {
                unsigned char* last = undoStack.top(); undoStack.pop();
                unsigned char* redoSnap = (unsigned char*)malloc(WIDTH * HEIGHT * 4);
                memcpy(redoSnap, frames[currentFrame].data, WIDTH * HEIGHT * 4);
                redoStack.push(redoSnap);
                memcpy(frames[currentFrame].data, last, WIDTH * HEIGHT * 4);
                free(last);
            }
            if (IsKeyPressed(KEY_Y) && !redoStack.empty()) {
                unsigned char* redoSnap = redoStack.top(); redoStack.pop();
                PushUndo(frames);
                memcpy(frames[currentFrame].data, redoSnap, WIDTH * HEIGHT * 4);
                free(redoSnap);
            }
            if (IsKeyPressed(KEY_B)) brushIsCircle = !brushIsCircle;
            if (IsKeyPressed(KEY_LEFT_BRACKET)) brushSize = std::max(1, brushSize - 1);
            if (IsKeyPressed(KEY_RIGHT_BRACKET)) brushSize = std::min(100, brushSize + 1);
            if (IsKeyPressed(KEY_N)) currentFrame = (currentFrame + 1) % NUM_FRAMES;
            if (IsKeyPressed(KEY_P)) currentFrame = (currentFrame - 1 + NUM_FRAMES) % NUM_FRAMES;
            if (IsKeyPressed(KEY_A)) showAI = !showAI;
                        else if (IsKeyPressed(KEY_S)) {
    const char *savePath = tinyfd_saveFileDialog("Save Image", "icon.png", 0, NULL, NULL);
    if (savePath) {
        PushUndo(frames);
        ExportImage(frames[currentFrame], savePath);
    }
}
else if (IsKeyPressed(KEY_L)) {
    const char *loadPath = tinyfd_openFileDialog("Load Image", "", 1, (const char *[]){"*.png"}, "PNG Images", 0);
    if (loadPath) {
        Image loadedImage = LoadImage(loadPath);
        if (loadedImage.data && loadedImage.width == WIDTH && loadedImage.height == HEIGHT) {
            PushUndo(frames);
            UnloadImage(frames[currentFrame]);
            frames[currentFrame] = loadedImage;
        } else {
            UnloadImage(loadedImage);
        }
    }
}              // NEW: Show info toggle (key 'I')
            if (IsKeyPressed(KEY_I)) {
                showInfo = !showInfo;
            }
            // NEW rubber if key tab is pressed
            if (IsKeyPressed(KEY_TAB)) {
                rubberActive = !rubberActive;
                if (rubberActive) {
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                } else {
                    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
                }
            }
            
        }

        UpdateTexture(canvas, frames[currentFrame].data);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTexture(canvas, 0, 0, WHITE);

        for (int i = 0; i < colorHistory.size(); i++) {
            DrawRectangle(10 + i * 30, HEIGHT - 40, 28, 28, colorHistory[i]);
        }

        if (showAI) DrawRectangle(WIDTH - 160, 10, 150, 150, LIGHTGRAY);

        for (int dy = -brushSize; dy <= brushSize; dy++)
            for (int dx = -brushSize; dx <= brushSize; dx++)
                if (!brushIsCircle || dx * dx + dy * dy <= brushSize * brushSize)
                    DrawPixel(mx + dx, my + dy, drawColor);

        // 🆕 Draw Color Wheel
        int wheelX = WIDTH - 100, wheelY = HEIGHT - 120;
        DrawColorWheel(wheelX, wheelY, 50);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int dx = mx - wheelX, dy = my - wheelY;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= 50) {
                float angle = atan2f(dy, dx);
                if (angle < 0) angle += 2 * PI;
                float hue = angle / (2 * PI);
                drawColor = HSVtoRGB(hue, 1, 1);
                AddColor(drawColor);
            }
        }

        // 🆕 Draw Palette based on drawColor
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
        std::string credits = std::string("main.cpp - Raylib Icon Maker by Demi 💖 PRO MAX") +
                      " (" + PLATFORM + ")";
        DrawText(credits.c_str(), 10, 10, 10, GRAY);

        if (inputActive) {
            DrawRectangle(50, HEIGHT / 2 - 20, 400, 40, LIGHTGRAY);
            DrawRectangleLines(50, HEIGHT / 2 - 20, 400, 40, DARKGRAY);
            DrawText(inputBuffer, 60, HEIGHT / 2 - 10, 20, BLACK);
            DrawText("Press ENTER to confirm | BACKSPACE to delete", 60, HEIGHT / 2 + 20, 10, GRAY);
            
        }
        if (showInfo) {
            DrawRectangle(60, 60, WIDTH - 120, HEIGHT - 120, Fade(LIGHTGRAY, 0.95f));
            DrawRectangleLines(60, 60, WIDTH - 120, HEIGHT - 120, GRAY);
            int x = 80, y = 80;
            DrawText("🎨 IconMaker Controls:", x, y, 24, BLACK); y += 40;
            DrawText("Left Click       - Paint", x, y, 20, DARKGRAY); y += 30;
            DrawText("TAB             - Toggle Eraser", x, y, 20, DARKGRAY); y += 30;
            DrawText("R               - RGB input mode", x, y, 20, DARKGRAY); y += 30;
            DrawText("C               - HSV input mode", x, y, 20, DARKGRAY); y += 30;
            DrawText("H               - HEX input mode", x, y, 20, DARKGRAY); y += 30;
            DrawText("B               - Toggle brush shape", x, y, 20, DARKGRAY); y += 30;
            DrawText("[ / ]           - Decrease / Increase brush size", x, y, 20, DARKGRAY); y += 30;
            DrawText("X               - Clear current frame", x, y, 20, DARKGRAY); y += 30;
            DrawText("Z / Y           - Undo / Redo", x, y, 20, DARKGRAY); y += 30;
            DrawText("N / P           - Next / Previous frame", x, y, 20, DARKGRAY); y += 30;
            DrawText("S / L           - Save / Load PNG", x, y, 20, DARKGRAY); y += 30;
            DrawText("I               - Toggle this help box", x, y, 20, DARKGRAY); y += 40;
            DrawText("Press [I] to close this window.", x, y, 18, GRAY);
        }
        

        EndDrawing();
    }

    UnloadTexture(canvas);
    for (int i = 0; i < NUM_FRAMES; i++) UnloadImage(frames[i]);
    while (!undoStack.empty()) { free(undoStack.top()); undoStack.pop(); }
    while (!redoStack.empty()) { free(redoStack.top()); redoStack.pop(); }

    CloseWindow();
    return 0;
}


// main.cpp - Raylib Icon Maker by Demi 💖 PRO MAX (with GUI Text Input + Color Wheel + Palette) - MACOS