#include <list>
#include <iostream>
#include <Content.h>
#include <Managers.h>
#include <Utils.h>

DWORD MemoryProbe::GetAddress() {
    return address;
}

/**
 * Prints the integer values starting at the address of this MemoryProbe.
 * @param length The number of integers to print.
 */
void MemoryProbe::PrintInts(int length) {
    std::vector<int> values(length);
	if (!Utils::ReadMemoryToBuffer(GameState::GetHandle(), address, values.data(), values.size() * sizeof(int))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    for (int i = 0; i < length; i++) {
        std::cout << values[i] << ", ";
    }
    std::cout << std::endl;
}

/**
 * Prints the byte values starting at the address of this MemoryProbe.
 * @param length The number of bytes to print.
 */
void MemoryProbe::PrintBytes(int length) {
    std::vector<BYTE> values(length);
	if (!Utils::ReadMemoryToBuffer(GameState::GetHandle(), address, values.data(), values.size() * sizeof(BYTE))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    for (int i = 0; i < length; i++) {
        std::cout << "0x" << std::hex << (int) values[i] << std::dec << ", ";
    }
    std::cout << std::endl;
}

/**
 * Prints the float values starting at the address of this MemoryProbe.
 * @param length The number of floats to print.
 */
void MemoryProbe::PrintFloats(int length) {
	std::vector<float> values(length);
	if (!Utils::ReadMemoryToBuffer(GameState::GetHandle(), address, values.data(), values.size() * sizeof(float))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    for (int i = 0; i < length; i++) {
        std::cout << values[i] << ", ";
    }
    std::cout << std::endl;
}

/**
 * Checks if the address still points to a valid simple item.
 * @param hProcess The handle to the process.
 */
bool SimpleItem::isValid(HANDLE hProcess) {

    int values[18];
    if (!Utils::ReadMemoryToBuffer(hProcess, address, &values, sizeof(values))) {
        return false;
    }
    if (values[17] != 0) {
        return false;
    }
    int typeValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, values[0], &typeValue, sizeof(typeValue))) {
        return false;
    }
    int actualValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return false;
    }
    return actualValue == 1317794187;
}

/**
 * Gets the item id of this item.
 * @param hProcess The handle to the process.
 * @return The item id.
 */
int SimpleItem::GetItemId(HANDLE hProcess) {
    if (_itemId != -1) {
        return _itemId;
    }
    int value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x2C, &value, sizeof(value))) {
        return -1;
    }
    _itemId = value;
    return value;
}

/**
 * Gets the ingredient id of this item.
 * @param hProcess The handle to the process.
 * @return The ingredient id.
 */
int SimpleItem::GetIngredientId(HANDLE hProcess) {
    if (_ingredientId != -1) {
        return _ingredientId;
    }
    int value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x30, &value, sizeof(value))) {
        return -1;
    }
    _ingredientId = value;
    return value;
}

/**
 * Gets the conveyor index of this item.
 * @param hProcess The handle to the process.
 * @return The conveyor index.
 */
int SimpleItem::GetConveyorIndex(HANDLE hProcess) {
    int value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x38, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

/**
 * Gets the name of this item.
 * @param hProcess The handle to the process.
 * @return The name of this item.
 */
std::string SimpleItem::GetName(HANDLE hProcess) {
    int id = GetItemId(hProcess);
    return ItemManager::GetItemName(id);
}

/**
 * Gets the ingredient name of this item.
 * @param hProcess The handle to the process.
 * @return The ingredient name of this item.
 */
std::string SimpleItem::GetIngredientName(HANDLE hProcess) {
    int id = GetIngredientId(hProcess);
    return ItemManager::GetItemName(id);
}

/**
 * Gets the x coordinate of this item.
 * @param hProcess The handle to the process.
 * @return The x coordinate.
 */
float SimpleItem::GetX(HANDLE hProcess) {
    float value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x24, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

/**
 * Gets the y coordinate of this item.
 * @param hProcess The handle to the process.
 * @return The y coordinate.
 */
float SimpleItem::GetY(HANDLE hProcess) {
    float value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x28, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

/**
 * Gets the position of this item.
 * @param hProcess The handle to the process.
 * @return The position.
 */
std::pair<float, float> SimpleItem::GetPos(HANDLE hProcess) {
    return std::make_pair(GetX(hProcess), GetY(hProcess));
}

/**
 * Gets the x coordinate of the mouse position of this item.
 * @param hProcess The handle to the process.
 * @return The x coordinate of the mouse position.
 */
float SimpleItem::GetMouseX(HANDLE hProcess) {
    std::pair<float, float> pos = GetMousePos(hProcess);
    return pos.first;
}

/**
 * Gets the y coordinate of the mouse position of this item.
 * @param hProcess The handle to the process.
 * @return The y coordinate of the mouse position.
 */
float SimpleItem::GetMouseY(HANDLE hProcess) {
    std::pair<float, float> pos = GetMousePos(hProcess);
    return pos.second;
}

/**
 * Gets the mouse position of this item.
 * @param hProcess The handle to the process.
 * @return The mouse position.
 */
std::pair<float, float> SimpleItem::GetMousePos(HANDLE hProcess) {
    return Utils::GamePosToMouseAbsolute(GameState::GetWindowHandle(), GetX(hProcess), GetY(hProcess));
}

/**
 * Sets the item id of this item.
 * @param hProcess The handle to the process.
 * @param value The new item id.
 */
void SimpleItem::SetItemId(HANDLE hProcess, int value) {
    if (value < 0 || value >= ItemManager::GetNumItems()) {
        return;
    }
    if (!Utils::WriteBufferToProcessMemory(hProcess, address + 0x2C, &value, sizeof(value))) {
        std::cout << "Error: Could not write to process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    _itemId = value;
}

/**
 * Sets the ingredient id of this item.
 * @param hProcess The handle to the process.
 * @param value The new ingredient id.
 */
void SimpleItem::SetIngredientId(HANDLE hProcess, int value) {
    if (value < 0 || value >= ItemManager::GetNumItems()) {
        return;
    }
    if (!Utils::WriteBufferToProcessMemory(hProcess, address + 0x30, &value, sizeof(value))) {
        std::cout << "Error: Could not write to process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    _ingredientId = value;
}

/**
 * Determines if this item has changed since the last call to this function.
 * @return @c true if this item has changed, @c false otherwise.
 */
bool SimpleItem::HasChanged() {
    int newHash = 0;
    newHash += GetItemId(GameState::GetHandle());
    newHash += GetIngredientId(GameState::GetHandle());
    newHash += GetConveyorIndex(GameState::GetHandle());
    newHash += GetX(GameState::GetHandle());
    newHash += GetY(GameState::GetHandle());
    if (newHash != hash) {
        hash = newHash;
        return true;
    }
    return false;
}

/**
 * Checks if the address still points to a valid complex item.
 * @param hProcess The handle to the process.
 */
bool ComplexItem::isValid(HANDLE hProcess) {
    if (address == 0) {
        return false;
    }
    int values[32];
    if (!Utils::ReadMemoryToBuffer(hProcess, address, &values, sizeof(values))) {
        return false;
    }
    int type = values[0];
    int typeValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, type, &typeValue, sizeof(typeValue))) {
        return false;
    }
    int actualValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return false;
    }
    return actualValue == 113766795;
}

/**
 * Gets the sub-items of this complex item.
 * @param hProcess The handle to the process.
 * @return The sub-items.
 */
std::list<SimpleItem> ComplexItem::GetItems(HANDLE hProcess) {
    int values[32];
    if (!Utils::ReadMemoryToBuffer(hProcess, address, &values, sizeof(values))) {
        return {};
    }
    // Validate
    int type = values[0];
    int typeValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, type, &typeValue, sizeof(typeValue))) {
        return {};
    }
    int actualValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return {};
    }
    if (actualValue != 113766795) {
        return {};
    }
    int listAddress = values[30];
    int size = values[22];
	std::vector<int> itemAddresses(size);
	if (!Utils::ReadMemoryToBuffer(hProcess, listAddress, itemAddresses.data(), itemAddresses.size() * sizeof(int))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return {};
    }
    std::list<SimpleItem> subItems;
    for (int i = 0; i < size; i++) {
        subItems.push_back(SimpleItem(itemAddresses[i]));
    }
    return subItems;
}

/**
 * Gets the conveyor index of this item.
 * @param hProcess The handle to the process.
 * @return The conveyor index.
 */
int ComplexItem::GetConveyorIndex(HANDLE hProcess) {
    int value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x38, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

/**
 * Gets the x coordinate of this item.
 * @param hProcess The handle to the process.
 * @return The x coordinate.
 */
float ComplexItem::GetX(HANDLE hProcess) {
    float value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x24, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

/**
 * Gets the y coordinate of this item.
 * @param hProcess The handle to the process.
 * @return The y coordinate.
 */
float ComplexItem::GetY(HANDLE hProcess) {
    float value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x28, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

/**
 * Determines if this item has changed since the last call to this function.
 * @return @c true if this item has changed, @c false otherwise.
 */
bool ComplexItem::HasChanged() {
    int newHash = 0;
    std::list<SimpleItem> items = GetItems(GameState::GetHandle());
    for (SimpleItem item: items) {
        newHash += item.GetItemId(GameState::GetHandle());
        newHash += item.GetIngredientId(GameState::GetHandle());
        newHash += item.GetConveyorIndex(GameState::GetHandle());
        newHash += item.GetX(GameState::GetHandle());
        newHash += item.GetY(GameState::GetHandle());
    }
    newHash += GetX(GameState::GetHandle());
    newHash += GetY(GameState::GetHandle());
    if (newHash != hash) {
        hash = newHash;
        return true;
    }
    return false;
}

/**
 * Gets the item this ItemInfo points to.
 * @return The item.
 */
std::unique_ptr<ItemBase> ItemInfo::GetItem() {
    if (mItem == 0) {
        return nullptr;
    }
    int values[32];
    if (!Utils::ReadMemoryToBuffer(GameState::GetHandle(), mItem, &values, sizeof(values))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return nullptr;
    }
    int type = values[0];
    int typeValue;
    if (!Utils::ReadMemoryToBuffer(GameState::GetHandle(), type, &typeValue, sizeof(typeValue))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return nullptr;
    }
    int actualValue;
    if (!Utils::ReadMemoryToBuffer(GameState::GetHandle(), typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return nullptr;
    }
    if (actualValue == 1317794187) {
        return std::make_unique<SimpleItem>(mItem);
    } else if (actualValue == 113766795) {
        return std::make_unique<ComplexItem>(mItem);
    }
    return nullptr;
}

/**
 * Checks if the address still points to a valid customer.
 * @param hProcess The handle to the process.
 */
bool Customer::isValid(HANDLE hProcess) {
    int value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address, &value, sizeof(value))) {
        return false;
    }
    int typeValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, value, &typeValue, sizeof(typeValue))) {
        return false;
    }
    int actualValue;
    if (!Utils::ReadMemoryToBuffer(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return false;
    }
    return actualValue == 113766795;
}

/**
 * Gets the id of this customer.
 * @param hProcess The handle to the process.
 * @return The id.
 * @note This isn't very useful, as the id always increments by 1 for each customer and seemingly never resets.
 */
int Customer::GetId(HANDLE hProcess) {
    int value;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x488, &value, sizeof(value))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return -1;
    }
    return value;
}

/**
 * Gets the items ordered by this customer.
 * @param hProcess The handle to the process.
 * @return The items.
 */
std::list<ItemInfo> Customer::GetItems(HANDLE hProcess) {
    std::list<ItemInfo> items;
    DWORD vectorAddress;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x15C, &vectorAddress, sizeof(vectorAddress))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
    }
    DWORD vectorEnd;
    if (!Utils::ReadMemoryToBuffer(hProcess, address + 0x160, &vectorEnd, sizeof(vectorEnd))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
    }
    int vectorLength = (vectorEnd - vectorAddress) / 48;
    for (int i = 0; i < vectorLength; i++) {
        DWORD itemAddress = vectorAddress + i * 48;
        ItemInfo itemInfo;
        if (!Utils::ReadMemoryToBuffer(hProcess, itemAddress, &itemInfo, sizeof(itemInfo))) {
            std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        }
        items.push_back(itemInfo);
    }
    return items;
}