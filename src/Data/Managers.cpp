#include <Content.h>
#include <Utils.h>
#include <Windows.h>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <atomic>
#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include <functional>
#include <Debugging.h>
#include <Managers.h>
#include <string>

float GameState::bbPercent = 0.0f;
int GameState::numConveyorItems = 0;
std::vector<std::unique_ptr<ItemBase>> GameState::conveyorItems;
std::vector<Customer> GameState::customers;
bool GameState::dirty = false;
bool GameState::needsSorting = false;
HANDLE GameState::handle = NULL;
HWND GameState::windowHandle = NULL;

bool botRetryFlag = false;

std::mutex GameState::conveyorItemsMutex;
std::mutex GameState::customersMutex;

/**
 * Returns the BurgerBot percentage.
 * @return The BurgerBot percentage.
 */
float GameState::GetBBPercent() {
    return bbPercent;
}

/**
 * Returns the number of conveyor items.
 * @return The number of conveyor items.
 */
float GameState::GetNumConveyorItems() {
    return numConveyorItems;
}

/**
 * Returns a copy of the conveyor items.
 * @return A copy of the conveyor items.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
std::vector<std::unique_ptr<ItemBase>> GameState::GetConveyorItems() {
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    std::vector<std::unique_ptr<ItemBase>> items;
    int i = 0;
    for (const std::unique_ptr<ItemBase> &item: conveyorItems) {
        if (SimpleItem * singleItem = dynamic_cast<SimpleItem *>(item.get())) {
            if (singleItem->isValid(handle)) {
                items.push_back(std::make_unique<SimpleItem>(*singleItem));
            }
        } else if (ComplexItem * multiItem = dynamic_cast<ComplexItem *>(item.get())) {
            if (singleItem->isValid(handle)) {
                items.push_back(std::make_unique<ComplexItem>(*multiItem));
            }
        }
        i++;
    }
    return items;
}

/**
 * Returns a copy of the customers.
 * @return A copy of the customers.
 * @note This function is thread-safe, but locks the customers mutex.
 */
std::vector<Customer> GameState::GetCustomers() {
    std::lock_guard<std::mutex> lock(customersMutex);
    for (int i = customers.size() - 1; i >= 0; i--) {
        if (!customers[i].isValid(handle)) {
            customers.erase(customers.begin() + i);
        }
    }
    std::vector<Customer> customersCopy(customers);
    return customersCopy;
}

/**
 * Returns whether the game state changed.
 * @return Whether the game state changed.
 */
bool GameState::IsDirty() {
    return dirty;
}

/**
 * Returns the handle to the game process.
 * @return The handle to the game process.
 */
HANDLE GameState::GetHandle() {
    return handle;
}

/**
 * Returns the handle to the game window.
 * @return The handle to the game window.
 */
HWND GameState::GetWindowHandle() {
    return windowHandle;
}

/**
 * Sets the BurgerBot percentage.
 * @param value The new BurgerBot percentage.
 */
void GameState::SetBBPercent(float value) {
    if (bbPercent != value) {
        bbPercent = value;
        dirty = true;
    }
}

/**
 * Sets the number of conveyor items.
 * @param value The new number of conveyor items.
 */
void GameState::SetNumConveyorItems(int value) {
    if (numConveyorItems != value) {
        numConveyorItems = value;
        dirty = true;
    }
}

/**
 * Adds a conveyor item from an address.
 * @param address The address of the conveyor item.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::AddItemFromAddress(DWORD address) {
    int values[32];
    if (!Utils::ReadMemoryToBuffer(handle, address, &values, sizeof(values))) {
        return;
    }
    int type = values[0];
    int typeValue;
    Utils::ReadMemoryToBuffer(handle, type, &typeValue, sizeof(typeValue));
    int actualValue;
    Utils::ReadMemoryToBuffer(handle, typeValue + 0x4, &actualValue, sizeof(actualValue));
    if (actualValue == 1317794187) {
        std::unique_ptr<SimpleItem> item = std::make_unique<SimpleItem>(address);
        if (!item->isValid(handle)) {
            return;
        }
        std::lock_guard<std::mutex> lock(conveyorItemsMutex);
        conveyorItems.push_back(std::move(item));
    } else if (actualValue == 113766795) {
        std::unique_ptr<ComplexItem> item = std::make_unique<ComplexItem>(address);
        if (!item->isValid(handle)) {
            return;
        }
        std::lock_guard<std::mutex> lock(conveyorItemsMutex);
        conveyorItems.push_back(std::move(item));
    }
}

/**
 * Removes a conveyor item from an address.
 * @param address The address of the conveyor item.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::RemoveItemFromAddress(DWORD address) {
    botRetryFlag = false;
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    for (int i = 0; i < conveyorItems.size(); i++) {
        if (conveyorItems[i]->GetAddress() == address) {
            conveyorItems.erase(conveyorItems.begin() + i);
            return;
        }
    }
    ComplexItem item(address);
    if (!item.isValid(handle)) {
        return;
    }
    for (int i = 0; i < conveyorItems.size(); i++) {
        std::list<SimpleItem> subItems = item.GetItems(handle);
        for (SimpleItem &subItem: subItems) {
            if (conveyorItems[i]->GetAddress() == subItem.GetAddress()) {
                conveyorItems.erase(conveyorItems.begin() + i);
                return;
            }
        }
    }
}

/**
 * Sets the handle to the game process.
 * @param pHandle The new handle to the game process.
 */
void GameState::SetHandle(HANDLE pHandle) {
    if (handle != nullptr) {
        std::cout << "Warning: Overwriting game handle." << std::endl;
    }
    handle = pHandle;
}

/**
 * Sets the handle to the game window.
 * @param pHandle The new handle to the game window.
 */
void GameState::SetWindowHandle(HWND pHandle) {
    if (windowHandle != nullptr) {
        std::cout << "Warning: Overwriting window handle." << std::endl;
    }
    windowHandle = pHandle;
}

/**
 * Sorts the conveyor items by conveyor index.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::SortConveyorItems() {
    if (!needsSorting) {
        return;
    }
    std::sort(conveyorItems.begin(), conveyorItems.end(),
              [](const std::unique_ptr<ItemBase> &a, const std::unique_ptr<ItemBase> &b) {
            if (SimpleItem * singleItemA = dynamic_cast<SimpleItem *>(a.get())) {
                if (SimpleItem * singleItemB = dynamic_cast<SimpleItem *>(b.get())) {
                    return singleItemA->GetConveyorIndex(GameState::handle) >
                           singleItemB->GetConveyorIndex(GameState::handle);
                }
            } else if (ComplexItem * multiItemA = dynamic_cast<ComplexItem *>(a.get())) {
                if (ComplexItem * multiItemB = dynamic_cast<ComplexItem *>(b.get())) {
                    return multiItemA->GetConveyorIndex(GameState::handle) >
                           multiItemB->GetConveyorIndex(GameState::handle);
                }
            }
        return false;
    });
    needsSorting = false;
}

/**
 * Adds a customer.
 * @param customer The customer to add.
 * @note This function is thread-safe, but locks the customers mutex.
 */
void GameState::AddCustomer(Customer customer) {
    std::lock_guard<std::mutex> lock(customersMutex);
    customers.push_back(customer);
    dirty = true;
}

/**
 * Removes a customer.
 * @param index The index of the customer to remove.
 * @note This function is thread-safe, but locks the customers mutex.
 */
void GameState::RemoveCustomer(int index) {
    std::lock_guard<std::mutex> lock(customersMutex);
    if (index >= 0 && index < customers.size()) {
        customers.erase(customers.begin() + index);
        dirty = true;
    }
}

/**
 * Resets the game state.
 */
void GameState::Reset() {
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    conveyorItems.clear();
    customers.clear();
    numConveyorItems = 0;
    dirty = true;
}

/**
 * Increments the first item on the conveyor.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::IncrementFirstItem() {
    SortConveyorItems();
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    if (!GameState::conveyorItems.empty()) {
        ItemBase *item = GameState::conveyorItems.front().get();
        if (SimpleItem * singleItem = dynamic_cast<SimpleItem *>(item)) {
            int id = singleItem->GetItemId(GameState::GetHandle());
            singleItem->SetItemId(GameState::GetHandle(), id + 1);
            singleItem->SetIngredientId(GameState::GetHandle(), id + 1);
        }
    }
}

/**
 * Decrements the first item on the conveyor.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::DecrementFirstItem() {
    SortConveyorItems();
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    if (!GameState::conveyorItems.empty()) {
        ItemBase *item = GameState::conveyorItems.front().get();
        if (SimpleItem * singleItem = dynamic_cast<SimpleItem *>(item)) {
            int id = singleItem->GetItemId(GameState::GetHandle());
            singleItem->SetItemId(GameState::GetHandle(), id - 1);
            singleItem->SetIngredientId(GameState::GetHandle(), id - 1);
        }
    }
}

/**
 * Unsets the dirty flag.
 */
void GameState::Update() {
    if (dirty) {
        dirty = false;
    }
}

/**
 * Checks whether any of the items have changed. Also sets the dirty flag if any of the items have changed.
 * @return Whether any of the items have changed.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
bool GameState::CheckItemsDirty() {
    if (dirty) {
        return true;
    }
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    for (const std::unique_ptr<ItemBase> &item: conveyorItems) {
        if (SimpleItem * singleItem = dynamic_cast<SimpleItem *>(item.get())) {
            if (singleItem->HasChanged()) {
                dirty = true;
            }
        } else if (ComplexItem * multiItem = dynamic_cast<ComplexItem *>(item.get())) {
            if (multiItem->HasChanged()) {
                dirty = true;
            }
        }
    }
    return dirty;
}

void ClickMouseAt(HWND window, int x, int y) {
    if (window != GetForegroundWindow()) {
        SetForegroundWindow(window);
        Sleep(100);
    }


    POINT p = {x, y - 30};
    ClientToScreen(window, &p);

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<long>(p.x * (65535.0 / GetSystemMetrics(SM_CXSCREEN)));
    input.mi.dy = static_cast<long>(p.y * (65535.0 / GetSystemMetrics(SM_CYSCREEN)));
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
    POINT cp;
    do {
        GetCursorPos(&cp);
        Sleep(5);
    } while (abs(cp.x - p.x) > 2 || abs(cp.y - p.y) > 2);
    Sleep(5);

    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void ClickRightMouse(HWND window) {

    POINT p = {10,40};
    ClientToScreen(window, &p);

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    SendInput(1, &input, sizeof(INPUT));

    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

int delay = 0;
int skip = 0;
DWORD prev = 0;
int attempts = 0;

bool makingItem = false;
ItemInfo targetItem;
std::vector<std::unique_ptr<SimpleItem>> ingredientsLeft;

std::unique_ptr<SimpleItem> itemToRetry = nullptr;

void Shuffle(std::vector<std::unique_ptr<SimpleItem>> &v) {
    for (int i = 0; i < v.size(); i++) {
        int j = rand() % v.size();
        std::swap(v[i], v[j]);
    }
}

void GameState::PerformActions() {
    if (GetAsyncKeyState(VK_END)) {
        if (delay > 100 && delay < 999999) {
            delay = 0;
            makingItem = false;
            ingredientsLeft.clear();
        } else {
            delay = 999999;
        }
        return;
    }
    if (GetAsyncKeyState(VK_HOME)) {
        delay = 0;
        return;
    }
    if (delay > 0) {
        delay--;
        return;
    }
    if (GameState::GetCustomers().size() > 0) {
        if (!makingItem) {
            int cskip = skip;
            std::vector<ItemInfo> items;
            for (Customer &customer: GameState::GetCustomers()) {
                for (ItemInfo &item: customer.GetItems(GameState::GetHandle())) {
                    items.push_back(item);
                }
            }
            ItemInfo target;
            std::vector<std::unique_ptr<SimpleItem>> ingredientsBackup;
            bool found = false;
            int i = 0;
            std::vector<std::unique_ptr<SimpleItem>> ingredients;
            while (!found && i < items.size()) {
                ItemInfo &item = items[i];
                if (item.mNumCopies == item.mNumComplete || item.mRobotComplete) {
                    i++;
                    continue;
                }
                if (cskip > 0) {
                    cskip--;
                    continue;
                }
                std::unique_ptr<ItemBase> ib = item.GetItem();
                ingredients.clear();
                if (SimpleItem * si = dynamic_cast<SimpleItem *>(ib.get())) {
                    if (ItemManager::IngredientLimit(si->GetIngredientId(GameState::GetHandle())) != -1) {
                        int numIngredients = 0;
                        for (std::unique_ptr<SimpleItem> &ingredient: ingredients) {
                            if (ingredient->GetIngredientId(GameState::GetHandle()) ==
                                si->GetIngredientId(GameState::GetHandle())) {
                                numIngredients++;
                            }
                        }
                        if (numIngredients < ItemManager::IngredientLimit(si->GetIngredientId(GameState::GetHandle()))) {
                            ingredients.push_back(std::make_unique<SimpleItem>(*si));
                        }
                    } else {
                        ingredients.push_back(std::make_unique<SimpleItem>(*si));
                    }
                } else if (ComplexItem * ci = dynamic_cast<ComplexItem *>(ib.get())) {
                    for (SimpleItem &ingredient: ci->GetItems(handle)) {
                        if (ItemManager::IngredientLimit(ingredient.GetIngredientId(GameState::GetHandle())) != -1) {
                            int numIngredients = 0;
                            for (std::unique_ptr<SimpleItem> &ingredient2: ingredients) {
                                if (ingredient2->GetIngredientId(GameState::GetHandle()) ==
                                    ingredient.GetIngredientId(GameState::GetHandle())) {
                                    numIngredients++;
                                }
                            }
                            if (numIngredients < ItemManager::IngredientLimit(ingredient.GetIngredientId(GameState::GetHandle()))) {
                                ingredients.push_back(std::make_unique<SimpleItem>(ingredient));
                            }
                        } else {
                            ingredients.push_back(std::make_unique<SimpleItem>(ingredient));
                        }
                    }
                }
                std::vector<bool> foundIngredients;
                for (int i = 0; i < ingredients.size(); i++) {
                    foundIngredients.push_back(false);
                }
                for (const std::unique_ptr<ItemBase> &conveyorItem: GameState::GetConveyorItems()) {
                    if (SimpleItem * si = dynamic_cast<SimpleItem *>(conveyorItem.get())) {
                        for (int i = 0; i < ingredients.size(); i++) {
                            if (ingredients[i]->GetIngredientId(GameState::GetHandle()) ==
                                si->GetIngredientId(GameState::GetHandle()) && !foundIngredients[i]) {
                                foundIngredients[i] = true;
                                break;
                            }
                        }
                    } else if (ComplexItem * ci = dynamic_cast<ComplexItem *>(conveyorItem.get())) {
                        // Ignored for now
                    }
                }
                bool allFound = true;
                for (bool foundIngredient: foundIngredients) {
                    if (!foundIngredient) {
                        allFound = false;
                        break;
                    }
                }
                if (allFound) {
                    if (cskip > 0) {
                        cskip--;
                        continue;
                    }
                    target = item;
                    found = true;
                }
                i++;
            }
            if (found) {
                targetItem = target;
                makingItem = true;
                ingredientsLeft = std::move(ingredients);
            } else {
                int cskip = skip;
                bool didFind = false;
                for (Customer &customer: GameState::GetCustomers()) {
                    for (ItemInfo &item: customer.GetItems(GameState::GetHandle())) {
                        if (item.mNumCopies == item.mNumComplete || item.mRobotComplete) {
                            continue;
                        }
                        if (cskip > 0) {
                            cskip--;
                            continue;
                        }
                        targetItem = item;
                        makingItem = true;
                        std::unique_ptr<ItemBase> ib = targetItem.GetItem();
                        ingredients.clear();
                        if (SimpleItem * si = dynamic_cast<SimpleItem *>(ib.get())) {
                            if (ItemManager::IngredientLimit(si->GetIngredientId(GameState::GetHandle())) != -1) {
                                int numIngredients = 0;
                                for (std::unique_ptr<SimpleItem> &ingredient: ingredients) {
                                    if (ingredient->GetIngredientId(GameState::GetHandle()) ==
                                        si->GetIngredientId(GameState::GetHandle())) {
                                        numIngredients++;
                                    }
                                }
                                if (numIngredients < ItemManager::IngredientLimit(si->GetIngredientId(GameState::GetHandle()))) {
                                    ingredients.push_back(std::make_unique<SimpleItem>(*si));
                                }
                            } else {
                                ingredients.push_back(std::make_unique<SimpleItem>(*si));
                            }
                        } else if (ComplexItem * ci = dynamic_cast<ComplexItem *>(ib.get())) {
                            for (SimpleItem &ingredient: ci->GetItems(handle)) {
                                if (ItemManager::IngredientLimit(ingredient.GetIngredientId(GameState::GetHandle())) != -1) {
                                    int numIngredients = 0;
                                    for (std::unique_ptr<SimpleItem> &ingredient2: ingredients) {
                                        if (ingredient2->GetIngredientId(GameState::GetHandle()) ==
                                            ingredient.GetIngredientId(GameState::GetHandle())) {
                                            numIngredients++;
                                        }
                                    }
                                    if (numIngredients < ItemManager::IngredientLimit(ingredient.GetIngredientId(GameState::GetHandle()))) {
                                        ingredients.push_back(std::make_unique<SimpleItem>(ingredient));
                                    }
                                } else {
                                    ingredients.push_back(std::make_unique<SimpleItem>(ingredient));
                                }
                            }
                        }
                        ingredientsLeft = std::move(ingredients);
                        didFind = true;
                        break;
                    }
                    if (didFind) {
                        break;
                    }
                }
                if (!didFind) {
                    skip = 0;
                    delay = 10;
                }
            }
        } else {
            if (botRetryFlag) {
                ingredientsLeft.insert(ingredientsLeft.begin(), std::move(itemToRetry));
            }
            while (!ingredientsLeft.empty() && ingredientsLeft[0] == nullptr) {
                ingredientsLeft.erase(ingredientsLeft.begin());
            }
            if (!ingredientsLeft.empty()) {
                std::vector<std::unique_ptr<ItemBase>> conveyorItems = GameState::GetConveyorItems();
                std::pair<float, float> coords = std::make_pair(-1, -1);
                if (attempts < 10) {
                    int i = 0;
                    do {
                        SimpleItem &ingredient = *ingredientsLeft[i];
                        std::cout << "Finding ingredient " << ingredient.GetIngredientName(handle) << std::endl;
                        // Find on the conveyor
                        for (const std::unique_ptr<ItemBase> &conveyorItem: conveyorItems) {
                            if (SimpleItem * si = dynamic_cast<SimpleItem *>(conveyorItem.get())) {
                                if (si->GetIngredientId(GameState::GetHandle()) ==
                                    ingredient.GetIngredientId(GameState::GetHandle())) {
                                    coords = std::make_pair(si->GetX(GameState::GetHandle()),
                                                            si->GetY(GameState::GetHandle()));
                                    coords = Utils::TranslateCoords(coords.first, coords.second);
                                    if (coords.first <= 5 || coords.second <= 5) {
                                        coords = std::make_pair(-1, -1);
                                    } else {
                                        if (prev != si->GetAddress()) {
                                            prev = si->GetAddress();
                                            attempts = 0;
                                        } else {
                                            attempts++;
                                        }
                                    }
                                    break;
                                }
                            } else if (ComplexItem * ci = dynamic_cast<ComplexItem *>(conveyorItem.get())) {
                                // Ignored for now
                            }
                        }
                    } while (coords.first == -1 && ++i < ingredientsLeft.size());
                }
                if (coords.first != -1) {
                    std::cout << "Clicking at " << coords.first << ", " << coords.second << std::endl;
                    botRetryFlag = true;
                    itemToRetry = std::move(ingredientsLeft[0]);
                    ClickMouseAt(GameState::GetWindowHandle(), coords.first, coords.second);
                    ingredientsLeft.erase(ingredientsLeft.begin());
                    skip = 0;
                    dirty = true;
                } else {
                    // Give up because you're bad at the game.
                    std::cout << "I give up. This game is too hard." << std::endl;
                    makingItem = false;
                    botRetryFlag = false;
                    ClickMouseAt(GameState::GetWindowHandle(), 400, 120);
                    skip++;
                    prev = 0;
                    attempts = 0;
                    dirty = true;
                }
            } else {
                std::cout << "Done with item! Delivering." << std::endl;
                makingItem = false;
                ClickRightMouse(GameState::GetWindowHandle());
                dirty = true;
            }
            delay = 2;
        }
    }
}

/**
 * Sets a breakpoint at the specified address.
 * @param address The address to set the breakpoint at.
 * @param callback The callback to call when the breakpoint is hit.
 * @return Whether the breakpoint was set successfully.
 */
bool BreakpointManager::SetBreakpoint(LPVOID address, std::function<void(const DEBUG_EVENT &, HANDLE)> callback) {
    Breakpoint bp;
    bp.address = address;
    bp.callback = callback;

    std::cout << "Setting breakpoint at address: " << address << std::endl;

    // Read the original byte
    BYTE originalByte;
    if (!ReadProcessMemory(processHandle, address, &originalByte, sizeof(originalByte), NULL)) {
        return false;
    }

    // Write 0xCC to set the breakpoint
    BYTE int3 = 0xCC;
    if (!WriteProcessMemory(processHandle, address, &int3, sizeof(int3), NULL)) {
        return false;
    }

    bp.originalByte = originalByte;
    bp.isActive = true;
    breakpoints[address] = bp;
    return true;
}

/**
 * Handles a breakpoint.
 * @param debugEvent The debug event.
 * @return Whether the breakpoint was handled successfully.
 */
bool BreakpointManager::HandleBreakpoint(const DEBUG_EVENT &debugEvent) {
    LPVOID address = (LPVOID) debugEvent.u.Exception.ExceptionRecord.ExceptionAddress;
    auto it = breakpoints.find(address);
    if (it == breakpoints.end()) {
        return false;
    }

    Breakpoint &bp = it->second;

    // Restore the original instruction
    if (!WriteProcessMemory(processHandle, address, &bp.originalByte, sizeof(bp.originalByte), NULL)) {
        return false;
    }

    // Get the thread context
    threadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
    CONTEXT context;
    ZeroMemory(&context, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    GetThreadContext(threadHandle, &context);

    // Set the trap flag for single-step execution
    context.EFlags |= 0x100; // Set trap flag
    SetThreadContext(threadHandle, &context);

    CloseHandle(threadHandle);

    // Schedule the callback
    if (bp.callback) {
        Debugging::EnqueueEvent(debugEvent, bp.callback, processHandle);
    }

    // Remember to reinstate this breakpoint after single-step
    lastBreakpoint = &bp;

    return true;
}

/**
 * @internal
 * Reinstates the last breakpoint.
 */
void BreakpointManager::ReinstateBreakpoint() {
    if (lastBreakpoint && lastBreakpoint->isActive) {
        BYTE int3 = 0xCC;
        WriteProcessMemory(processHandle, lastBreakpoint->address, &int3, sizeof(int3), NULL);
        lastBreakpoint = nullptr;
    }
}

/**
 * @internal
 * Clears the single-step flag.
 */
void BreakpointManager::ClearSingleStep() {
    CONTEXT context;
    ZeroMemory(&context, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    GetThreadContext(threadHandle, &context);
    context.EFlags &= ~0x100; // Clear trap flag
    SetThreadContext(threadHandle, &context);
}

/**
 * Loads the item names from the specified file.
 * @param filename The filename to load the item names from.
 * @return Whether the item names were loaded successfully.
 */
bool ItemManager::LoadItemNames(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file." << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        itemNames.push_back(line);
    }
    file.close();
    return true;
}

/**
 * Loads the item data from the specified folder.
 * @param folder The folder to load the item data from.
 * @return Whether the item data was loaded successfully.
 */
bool ItemManager::LoadData(const std::string &folder) {
    for (const auto &entry: std::filesystem::directory_iterator(folder)) {
        std::string filename = entry.path().filename().string();
        std::ifstream file(entry.path());
        if (!file.is_open()) {
            std::cout << "Error: Could not open file." << std::endl;
            return false;
        }
        ItemData data;
        std::string line;
        bool isSubItem = filename.find('@') != std::string::npos;
        int index = -1;
        int parentIndex = -1;
        while (std::getline(file, line)) {
            std::string key = line.substr(0, line.find(':'));
            std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return tolower(c); });
            std::string value = line.substr(line.find(':') + 2);
            if (key == "type") {
                data.type = value;
            } else if (key == "index") {
                data.index = std::stoi(value);
                index = data.index;
            } else if (key == "parent") {
                parentIndex = std::stoi(value);
            } else if (key == "adjustx") {
                data.adjustX = std::stoi(value);
            } else if (key == "adjusty") {
                data.adjustY = std::stoi(value);
            } else if (key == "ox") {
                data.oX = std::stoi(value);
            } else if (key == "oy") {
                data.oY = std::stoi(value);
            } else if (key == "absx") {
                data.absX = std::stoi(value);
            } else if (key == "absy") {
                data.absY = std::stoi(value);
            } else if (key == "height") {
                data.height = std::stoi(value);
            } else if (key == "corner") {
                data.cornerX = std::stoi(value.substr(value.find('(') + 1, value.find(',') - value.find('(') - 1));
                data.cornerY = std::stoi(value.substr(value.find(',') + 2, value.find(')') - value.find(',') - 2));
            } else if (key == "layer") {
                data.layer = std::stoi(value);
            } else {
                std::cout << "Error: Unknown key: " << key << std::endl;
                return false;
            }
        }
        if (isSubItem) {
            itemData[parentIndex].subItems.push_back(data);
        } else {
            itemData[index] = data;
            if (data.oX == 0 || data.oY == 0) {
                std::cout << "Warning: Item " << data.type << " has no offset." << std::endl;
            }
        }
    }
    return true;
}

/**
 * Returns the name of the item with the specified ID.
 * @param id The ID of the item.
 * @return The name of the item with the specified ID.
 */
std::string ItemManager::GetItemName(int id) {
    if (id < 0 || id >= itemNames.size()) {
        return "Unknown";
    }
    return itemNames[id];
}

/**
 * Returns the data of the item with the specified ID.
 * @param id The ID of the item.
 * @return The data of the item with the specified ID.
 */
ItemData ItemManager::GetItemData(int id) {
    if (id < 0 || id >= itemData.size()) {
        return ItemData();
    }
    return itemData[id];
}

/**
 * Returns the number of items.
 * @return The number of items.
 */
int ItemManager::GetNumItems() {
    return itemNames.size();
}

/**
 * Loads the item names and data.
 * @return Whether the item names and data were loaded successfully.
 */
bool ItemManager::LoadContent() {
    if (!ItemManager::LoadItemNames("resources/bsfood.txt")) {
        return false;
    }
    if (!ItemManager::LoadData("resources/itemdata")) {
        return false;
    }
    return true;
}

int ItemManager::IngredientLimit(int id) {
    switch (id) {
        case 4:
        case 37:
        case 38:
        case 39:
        case 42:
        case 48:
        case 57:
        case 129:
        case 130:
        case 150:
        case 155:
            return 1;
        default:
            if (id >= 234){
                return 0;
            }
            return -1;
    }
}

std::vector<std::string> ItemManager::itemNames;
std::unordered_map<int, ItemData> ItemManager::itemData;