#include <list>
#include <iostream>
#include <Content.h>
#include <Managers.h>
#include <Utils.h>


void MemoryProbe::PrintInts(int length) {
    std::vector<int> values(length);
	if (!Utils::ReadBufferToProcessMemory(GameState::GetHandle(), address, &values, values.size() * sizeof(int))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    for (int i = 0; i < length; i++) {
        std::cout << values[i] << ", ";
    }
    std::cout << std::endl;
}

void MemoryProbe::PrintBytes(int length) {
    std::vector<BYTE> values(length);
	if (!Utils::ReadBufferToProcessMemory(GameState::GetHandle(), address, &values, values.size() * sizeof(BYTE))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    for (int i = 0; i < length; i++) {
        std::cout << "0x" << std::hex << (int) values[i] << std::dec << ", ";
    }
    std::cout << std::endl;
}

void MemoryProbe::PrintFloats(int length) {
	std::vector<float> values(length);
	if (!Utils::ReadBufferToProcessMemory(GameState::GetHandle(), address, &values, values.size() * sizeof(float))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return;
    }
    for (int i = 0; i < length; i++) {
        std::cout << values[i] << ", ";
    }
    std::cout << std::endl;
}

bool SingleItem::isValid(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address, &value, sizeof(value))) {
        return false;
    }
    int typeValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, value, &typeValue, sizeof(typeValue))) {
        return false;
    }
    int actualValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return false;
    }
    return actualValue == 1317794187;
}

int SingleItem::GetItemId(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x2C, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

int SingleItem::GetIngredientId(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x30, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

int SingleItem::GetConveyorIndex(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x38, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

std::string SingleItem::GetName(HANDLE hProcess) {
    int id = GetItemId(hProcess);
    return ItemManager::GetItemName(id);
}

std::string SingleItem::GetIngredientName(HANDLE hProcess) {
    int id = GetIngredientId(hProcess);
    return ItemManager::GetItemName(id);
}

float SingleItem::GetX(HANDLE hProcess) {
    float value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x24, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

float SingleItem::GetY(HANDLE hProcess) {
    float value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x28, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

int SingleItem::GetLayer(HANDLE hProcess) {
    return ItemManager::GetItemData(GetItemId(hProcess)).layer;
}

void SingleItem::SetItemId(HANDLE hProcess, int value) {
    if (value < 0 || value >= ItemManager::GetNumItems()) {
        return;
    }
    if (!Utils::WriteBufferToProcessMemory(hProcess, address + 0x2C, &value, sizeof(value))) {
        std::cout << "Error: Could not write to process memory. Line: " << __LINE__ << std::endl;
    }
}

void SingleItem::SetIngredientId(HANDLE hProcess, int value) {
    if (value < 0 || value >= ItemManager::GetNumItems()) {
        return;
    }
    if (!Utils::WriteBufferToProcessMemory(hProcess, address + 0x30, &value, sizeof(value))) {
        std::cout << "Error: Could not write to process memory. Line: " << __LINE__ << std::endl;
    }
}

bool SingleItem::HasChanged() {
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

bool MultiItem::isValid(HANDLE hProcess) {
    int values[32];
    if (!Utils::ReadBufferToProcessMemory(hProcess, address, &values, sizeof(values))) {
        return false;
    }
    int type = values[0];
    int typeValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, type, &typeValue, sizeof(typeValue))) {
        return false;
    }
    int actualValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return false;
    }
    return actualValue == 113766795;
}

std::list<SingleItem> MultiItem::GetItems(HANDLE hProcess) {
    int values[32];
    if (!Utils::ReadBufferToProcessMemory(hProcess, address, &values, sizeof(values))) {
        return {};
    }
    // Validate
    int type = values[0];
    int typeValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, type, &typeValue, sizeof(typeValue))) {
        return {};
    }
    int actualValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return {};
    }
    if (actualValue != 113766795) {
        return {};
    }
    int listAddress = values[30];
    int size = values[22];
	std::vector<int> itemAddresses(size);
	if (!Utils::ReadBufferToProcessMemory(hProcess, listAddress, &itemAddresses, itemAddresses.size() * sizeof(int))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return {};
    }
    std::list<SingleItem> subItems;
    for (int i = 0; i < size; i++) {
        subItems.push_back(SingleItem(itemAddresses[i]));
    }
    return subItems;
}

int MultiItem::GetConveyorIndex(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x38, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

float MultiItem::GetX(HANDLE hProcess) {
    float value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x24, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

float MultiItem::GetY(HANDLE hProcess) {
    float value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x28, &value, sizeof(value))) {
        return -1;
    }
    return value;
}

bool MultiItem::HasChanged() {
    int newHash = 0;
    std::list<SingleItem> items = GetItems(GameState::GetHandle());
    for (SingleItem item: items) {
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

std::unique_ptr<ItemBase> ItemInfo::GetItem() {
    if (mItem == 0) {
        return nullptr;
    }
    int values[32];
    if (!Utils::ReadBufferToProcessMemory(GameState::GetHandle(), mItem, &values, sizeof(values))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return nullptr;
    }
    int type = values[0];
    int typeValue;
    if (!Utils::ReadBufferToProcessMemory(GameState::GetHandle(), type, &typeValue, sizeof(typeValue))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return nullptr;
    }
    int actualValue;
    if (!Utils::ReadBufferToProcessMemory(GameState::GetHandle(), typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return nullptr;
    }
    if (actualValue == 1317794187) {
        return std::make_unique<SingleItem>(mItem);
    } else if (actualValue == 113766795) {
        return std::make_unique<MultiItem>(mItem);
    }
    return nullptr;
}

bool Customer::isValid(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address, &value, sizeof(value))) {
        return false;
    }
    int typeValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, value, &typeValue, sizeof(typeValue))) {
        return false;
    }
    int actualValue;
    if (!Utils::ReadBufferToProcessMemory(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue))) {
        return false;
    }
    return actualValue == 113766795;
}

int Customer::GetId(HANDLE hProcess) {
    int value;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x488, &value, sizeof(value))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        return -1;
    }
    return value;
}

std::list<ItemInfo> Customer::GetItems(HANDLE hProcess) {
    std::list<ItemInfo> items;
    DWORD vectorAddress;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x15C, &vectorAddress, sizeof(vectorAddress))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
    }
    DWORD vectorEnd;
    if (!Utils::ReadBufferToProcessMemory(hProcess, address + 0x160, &vectorEnd, sizeof(vectorEnd))) {
        std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
    }
    int vectorLength = (vectorEnd - vectorAddress) / 48;
    for (int i = 0; i < vectorLength; i++) {
        DWORD itemAddress = vectorAddress + i * 48;
        ItemInfo itemInfo;
        if (!Utils::ReadBufferToProcessMemory(hProcess, itemAddress, &itemInfo, sizeof(itemInfo))) {
            std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
        }
        items.push_back(itemInfo);
    }
    return items;
}