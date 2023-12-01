#include <Windows.h>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <Content.h>
#include <Managers.h>
#include <Debugging.h>
#include <string>
#include <Utils.h>

using namespace Gdiplus;
namespace fs = std::filesystem;

#pragma comment (lib, "Gdiplus.lib")

// Image drawing functions

void DrawImage(HDC hdc, Image *image, int x, int y, int width, int height) {
    Graphics graphics(hdc);
    graphics.DrawImage(image, x, y, width, height);
}

void DrawItem(HDC pHdc, int itemId, int x, int y) {
    float scale = 0.425;
    if (itemId == 850 || itemId == 851) {
        scale = 1;
    }
    std::wstring imageName = L"resources/img2048/" + std::to_wstring(itemId) + L".png";
    Image *image = Utils::GetImageFromPath(imageName.c_str());
    ItemData data = ItemManager::GetItemData(itemId);
    int xOffset = data.oX;
    int yOffset = data.oY;
    int width = image->GetWidth() * scale;
    int height = image->GetHeight() * scale;
    DrawImage(pHdc, image, x - xOffset, y - yOffset, width, height);
}

// Window message handling

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Draw background
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 220, 200));
            FillRect(hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);
            // Draw conveyor items
            TextOut(hdc, 10, 10, "Conveyor Items:", 15);
            GameState::SortConveyorItems();
            std::list<std::unique_ptr<ItemBase>> itemList = GameState::GetConveyorItems();
            for (const std::unique_ptr<ItemBase> &item: itemList) {
                if (SimpleItem * simpleItem = dynamic_cast<SimpleItem *>(item.get())) {
                    if (!simpleItem->isValid(GameState::GetHandle())) {
                        continue;
                    }
                    int id = simpleItem->GetItemId(GameState::GetHandle());
                    float itemX = simpleItem->GetX(GameState::GetHandle());
                    float itemY = simpleItem->GetY(GameState::GetHandle());
                    DrawItem(hdc, id, itemX, itemY);
                } else if (ComplexItem * multiItem = dynamic_cast<ComplexItem *>(item.get())) {
                    if (!multiItem->isValid(GameState::GetHandle())) {
                        continue;
                    }
                    float itemX = multiItem->GetX(GameState::GetHandle());
                    float itemY = multiItem->GetY(GameState::GetHandle());
                    int j = 0;
                    // Sort sub items by layer
                    std::list<SimpleItem> subItems = multiItem->GetItems(GameState::GetHandle());
                    subItems.sort([](SimpleItem &a, SimpleItem &b) {
                        return a.GetLayer(GameState::GetHandle()) < b.GetLayer(GameState::GetHandle());
                    });
                    for (SimpleItem subItem: multiItem->GetItems(GameState::GetHandle())) {
                        if (!subItem.isValid(GameState::GetHandle())) {
                            break;
                        }
                        int id = subItem.GetItemId(GameState::GetHandle());
                        float subX = subItem.GetX(GameState::GetHandle());
                        float subY = subItem.GetY(GameState::GetHandle());
                        DrawItem(hdc, id, itemX + subX, itemY + subY);
                        j++;
                    }
                }
            }
            // Draw customers
            std::vector<Customer> customers = GameState::GetCustomers();
            for (int i = customers.size() - 1; i >= 0; i--) {
                Customer customer = customers[i];
                if (!customer.isValid(GameState::GetHandle())) {
                    GameState::RemoveCustomer(i);
                    continue;
                }
                int id = customer.GetId(GameState::GetHandle());
                std::list<ItemInfo> items = customer.GetItems(GameState::GetHandle());
                // Draw items ordered by customer
                for (ItemInfo item: items) {
                    if (item.mItem == 0) {
                        break;
                    }
                    std::unique_ptr<ItemBase> itemBase = item.GetItem();
                    if (SimpleItem * simpleItem = dynamic_cast<SimpleItem *>(itemBase.get())) {
                        if (!simpleItem->isValid(GameState::GetHandle())) {
                            break;
                        }
                        int itemId = simpleItem->GetItemId(GameState::GetHandle());
                        float itemX = simpleItem->GetX(GameState::GetHandle());
                        float itemY = simpleItem->GetY(GameState::GetHandle());
                        DrawItem(hdc, itemId, 100 + 200 * i + itemX, 400 + itemY);
                    } else if (ComplexItem * multiItem = dynamic_cast<ComplexItem *>(itemBase.get())) {
                        if (!multiItem->isValid(GameState::GetHandle())) {
                            break;
                        }
                        float itemX = multiItem->GetX(GameState::GetHandle());
                        float itemY = multiItem->GetY(GameState::GetHandle());
                        int j = 0;
                        // Sort sub items by layer
                        std::list<SimpleItem> subItems = multiItem->GetItems(GameState::GetHandle());
                        subItems.sort([](SimpleItem &a, SimpleItem &b) {
                            return a.GetLayer(GameState::GetHandle()) < b.GetLayer(GameState::GetHandle());
                        });
                        for (SimpleItem subItem: multiItem->GetItems(GameState::GetHandle())) {
                            if (!subItem.isValid(GameState::GetHandle())) {
                                break;
                            }
                            int id = subItem.GetItemId(GameState::GetHandle());
                            float subX = subItem.GetX(GameState::GetHandle());
                            float subY = subItem.GetY(GameState::GetHandle());
                            DrawItem(hdc, id, 100 + (200 * i) + itemX + subX, 400 + itemY + subY);
                            j++;
                        }
                    }
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

int originX = 0;
int originY = 0;
float scaleX = 1.0/800.0;
float scaleY = 1.0/600.0;

static std::pair<float, float> TranslateCoords(HWND hwndOverlay, float x, float y) {
    RECT overlayRect;
    GetWindowRect(hwndOverlay, &overlayRect);
    HWND gameWindow = GameState::GetWindowHandle();
    RECT gameRect;
    GetWindowRect(gameWindow, &gameRect);
    int gameWidth = gameRect.right - gameRect.left;
    int gameHeight = gameRect.bottom - gameRect.top;
    float relScaleX = gameWidth * scaleX;
    float relScaleY = gameHeight * scaleY;
    int offsetX = gameRect.left - overlayRect.left;
    int offsetY = gameRect.top - overlayRect.top;
    float xPrime = originX + (x * relScaleX) + offsetX;
    float yPrime = originY + (y * relScaleY) + offsetY;
    return std::make_pair(xPrime, yPrime);
}

VOID OverlayPaint(HWND hwnd) {
    int maxWidth = GetSystemMetrics(SM_CXSCREEN);
    int maxHeight = GetSystemMetrics(SM_CYSCREEN);
    HDC hdc = GetDC(NULL);
    HDC hdcImage = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, maxWidth, maxHeight);
    HBITMAP oldBitmap = (HBITMAP) SelectObject(hdcImage, bitmap);
    Graphics graphics(hdcImage);
    Bitmap bmp(maxWidth, maxHeight, PixelFormat32bppARGB);
    Graphics bmpGraphics(&bmp);

    Pen smallPen(Color(255, 255, 0, 0), 2);

    std::list<std::unique_ptr<ItemBase>> itemList = GameState::GetConveyorItems();
    for (const std::unique_ptr<ItemBase> &item: itemList) {
        if (SimpleItem * simpleItem = dynamic_cast<SimpleItem *>(item.get())) {
            if (!simpleItem->isValid(GameState::GetHandle())) {
                continue;
            }
            int id = simpleItem->GetItemId(GameState::GetHandle());
            float itemX = simpleItem->GetX(GameState::GetHandle());
            float itemY = simpleItem->GetY(GameState::GetHandle());
            // Draw dot on item after translation
            std::pair<float, float> coords = TranslateCoords(hwnd, itemX, itemY);
            int xPrime = coords.first;
            int yPrime = coords.second;
            bmpGraphics.DrawEllipse(&smallPen, xPrime - 4, yPrime - 4, 8, 8);
        } else if (ComplexItem * multiItem = dynamic_cast<ComplexItem *>(item.get())) {
            if (!multiItem->isValid(GameState::GetHandle())) {
                continue;
            }
            float itemX = multiItem->GetX(GameState::GetHandle());
            float itemY = multiItem->GetY(GameState::GetHandle());
            // Draw dot on item
            std::pair<float, float> coords = TranslateCoords(hwnd, itemX, itemY);
            int xPrime = coords.first;
            int yPrime = coords.second;
            bmpGraphics.DrawEllipse(&smallPen, xPrime - 4, yPrime - 4, 8, 8);
        }
    }

    graphics.DrawImage(&bmp, 0, 0, maxWidth, maxHeight);

    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    POINT pos = {rect.left, rect.top};
    SIZE size = {rect.right - rect.left, rect.bottom - rect.top};
    POINT sourcePos = {0, 0};

    UpdateLayeredWindow(hwnd, hdc, &pos, &size, hdcImage, &sourcePos, 0, &blend, ULW_ALPHA);

    SelectObject(hdcImage, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(hdcImage);
    ReleaseDC(NULL, hdc);
}

LRESULT CALLBACK WindowProcedureOverlay(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_NCHITTEST: {
            return HTTRANSPARENT;
        case WM_CREATE:
            SetTimer(hwnd, 1, 33, NULL);
            return 0;
        case WM_TIMER:
            OverlayPaint(hwnd);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    // If Ctrl is held, scroll wheel will change first item's id.
    if (nCode >= 0) {
        if (wParam == WM_MOUSEWHEEL) {
            MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *) lParam;
            if (pMouseStruct != NULL) {
                int delta = GET_WHEEL_DELTA_WPARAM(pMouseStruct->mouseData);
                if (delta > 0 && GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    GameState::IncrementFirstItem();
                } else if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    GameState::DecrementFirstItem();
                }
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {

    std::cout << "Starting BS3 Memory Reader" << std::endl;

    SetConsoleTitle("BS3 Memory Reader");

    if (!ItemManager::LoadContent()) {
        std::cerr << "Failed to load content" << std::endl;
        return -1;
    }

    std::thread debugThread(Debugging::DebugLoop);

    // Initialize GDI+ for drawing images to a window

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Ok) {
        std::cerr << "Failed to initialize GDI+" << std::endl;
        return -1;
    }

    const char CLASS_NAME[] = "BS3 Item Display";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "BS3 Item Display", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nShowCmd);
    UpdateWindow(hwnd);

    const char CLASS_NAME_OVERLAY[] = "BS3 Overlay";

    WNDCLASS wcOverlay = {};
    wcOverlay.lpfnWndProc = WindowProcedureOverlay;
    wcOverlay.hInstance = hInstance;
    wcOverlay.lpszClassName = CLASS_NAME_OVERLAY;

    RegisterClass(&wcOverlay);

    int maxWidth = GetSystemMetrics(SM_CXSCREEN);
    int maxHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwndOverlay = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, CLASS_NAME_OVERLAY, "BS3 Overlay", WS_DLGFRAME,
                               0, 0, maxWidth, maxHeight, NULL, NULL, hInstance, NULL);

    if (hwndOverlay == NULL) {
        return 0;
    }

    ShowWindow(hwndOverlay, nShowCmd);
    UpdateWindow(hwndOverlay);

    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    if (mouseHook == NULL) {
        std::cerr << "Failed to set mouse hook" << std::endl;
        return -1;
    }

    // Main loop

    MSG msg;
    long timeMillis = 0;
    bool running = true;
    while (running) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            long currentTimeMillis = GetTickCount();
            // Update every 33ms if dirty
            if (currentTimeMillis - timeMillis > 33) {
                if (GameState::CheckItemsDirty()) {
                    InvalidateRect(hwnd, NULL, TRUE);
                    UpdateWindow(hwnd);
                    GameState::Update();
                }
                timeMillis = currentTimeMillis;
            }
        }
    }
    GdiplusShutdown(gdiplusToken);
    UnhookWindowsHookEx(mouseHook);

    return 0;
}