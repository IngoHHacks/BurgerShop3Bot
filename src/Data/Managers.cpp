#include <Content.h>
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
std::list<std::unique_ptr<ItemBase>> GameState::conveyorItems;
std::vector<Customer> GameState::customers;
bool GameState::dirty = false;
bool GameState::needsSorting = false;
HANDLE GameState::handle = NULL;

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
std::list<std::unique_ptr<ItemBase>> GameState::GetConveyorItems() {
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    std::list<std::unique_ptr<ItemBase>> items;
    for (const std::unique_ptr<ItemBase> &item: conveyorItems) {
        if (SimpleItem * singleItem = dynamic_cast<SimpleItem *>(item.get())) {
            items.push_back(std::make_unique<SimpleItem>(*singleItem));
        } else if (ComplexItem * multiItem = dynamic_cast<ComplexItem *>(item.get())) {
            items.push_back(std::make_unique<ComplexItem>(*multiItem));
        }
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
 * Sets the conveyor items.
 * @param value The new conveyor items.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::SetConveyorItems(std::list<std::unique_ptr<ItemBase>> value) {
    std::lock_guard<std::mutex> lock(conveyorItemsMutex);
    conveyorItems = std::move(value);
    dirty = true;
    needsSorting = true;
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
 * Sorts the conveyor items by conveyor index.
 * @note This function is thread-safe, but locks the conveyor items mutex.
 */
void GameState::SortConveyorItems() {
    if (!needsSorting) {
        return;
    }
    conveyorItems.sort([](const std::unique_ptr<ItemBase> &a, const std::unique_ptr<ItemBase> &b) {
        if (SimpleItem * singleItemA = dynamic_cast<SimpleItem *>(a.get())) {
            if (SimpleItem * singleItemB = dynamic_cast<SimpleItem *>(b.get())) {
                return singleItemA->GetConveyorIndex(GameState::handle) <
                       singleItemB->GetConveyorIndex(GameState::handle);
            }
        } else if (ComplexItem * multiItemA = dynamic_cast<ComplexItem *>(a.get())) {
            if (ComplexItem * multiItemB = dynamic_cast<ComplexItem *>(b.get())) {
                return multiItemA->GetConveyorIndex(GameState::handle) <
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
    context.ContextFlags = CONTEXT_CONTROL;
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
    context.ContextFlags = CONTEXT_CONTROL;
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

std::vector<std::string> ItemManager::itemNames;
std::unordered_map<int, ItemData> ItemManager::itemData;