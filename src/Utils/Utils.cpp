#include <windows.h>
#include <atomic>
#include <thread>
#include <list>
#include <tlhelp32.h>
#include <iostream>
#include <Managers.h>
#include <Content.h>
#include <string>
#include <Utils.h>

uintptr_t Utils::GetModuleBaseAddress(DWORD procId, const char *modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!strcmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t) modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

bool Utils::GetWow64ThreadContext(HANDLE hThread, WOW64_CONTEXT &context) {
    ZeroMemory(&context, sizeof(WOW64_CONTEXT));
    context.ContextFlags = CONTEXT_FULL;

    if (!Wow64GetThreadContext(hThread, &context)) {
        std::cerr << "Failed to get 32-bit thread context." << std::endl;
        return false;
    }

    return true;
}

bool Utils::SetWow64ThreadContext(HANDLE hThread, WOW64_CONTEXT &context) {
    if (!Wow64SetThreadContext(hThread, &context)) {
        std::cerr << "Failed to set 32-bit thread context." << std::endl;
        return false;
    }

    return true;
}

bool Utils::ReadBufferToProcessMemory(HANDLE processHandle, DWORD address, LPVOID buffer, SIZE_T size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(static_cast<DWORD_PTR>(address)), buffer,
                             size, &bytesRead) && bytesRead == size;
}

bool Utils::WriteBufferToProcessMemory(HANDLE processHandle, DWORD address, LPCVOID buffer, SIZE_T size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(static_cast<DWORD_PTR>(address)), buffer,
                              size, &bytesWritten) && bytesWritten == size;
}

bool Utils::IsSentinelNode(HANDLE hProcess, DWORD address, const Node &node) {
    // 1. Try reading content. If it fails, we know it's the sentinel node.
    int value;
    if (!ReadBufferToProcessMemory(hProcess, node.content, &value, sizeof(value))) {
        return true;
    }
    // 2. Try reading the value as a pointer. If it fails, we know it's the sentinel node.
    int pointer;
    if (!ReadBufferToProcessMemory(hProcess, value, &pointer, sizeof(pointer))) {
        return true;
    }
    // 3. Try reading the pointer + 0x4 as pointer. If it fails or is not 1317794187 or 113766795, we know it's the sentinel node.
    int pointerToPointer;
    if (!ReadBufferToProcessMemory(hProcess, pointer + 0x4, &pointerToPointer, sizeof(pointerToPointer))) {
        return true;
    }
    if (pointerToPointer != 1317794187 && pointerToPointer != 113766795) {
        return true;
    }
    // Otherwise, it's not the sentinel node.
    return false;
}

bool Utils::TryFindFood(HANDLE hProcess, DWORD startAddress, int depth, int width, std::list<std::string> &path) {

    if (depth <= 0) {
        return false;
    }

    int test1;
    if (!ReadBufferToProcessMemory(hProcess, startAddress, &test1, sizeof(test1))) {
        return false;
    }
    int test2;
    if (!ReadBufferToProcessMemory(hProcess, test1 + 0x4, &test2, sizeof(test2))) {
        return false;
    }
    if (test2 == 113766795) {
        int values[32];
        if (!ReadBufferToProcessMemory(hProcess, test2, &values, sizeof(values))) {
            return {};
        }
        int size = values[22];
        if (size < 1 || size > 32) {
            return false;
        }
        return true;
    }

	std::vector<int> values(width);
	if (!ReadBufferToProcessMemory(hProcess, startAddress, &values, values.size() * sizeof(int))) {
        return false;
    }

    for (int i = 0; i < width; i++) {
        if (TryFindFood(hProcess, values[i], depth - 1, width, path)) {
            path.push_back(std::to_string(i) + "/" + std::to_string(startAddress));
            return true;
        }
    }
    return false;
}

std::list<Node> Utils::TraverseAndCollectNodes(HANDLE hProcess, DWORD startAddress) {
    std::list<Node> nodeList;
    Node currentNode;

    // Find the first node in the list
    DWORD currentAddress = startAddress;
    DWORD prevAddress = 0;
    DWORD sentinelAddress = 0;
    std::list<Node> path;
    while (true) {
        if (!ReadBufferToProcessMemory(hProcess, currentAddress, &currentNode, sizeof(currentNode))) {
            std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
            break;
        }

        if (IsSentinelNode(hProcess, currentAddress, currentNode)) {
            if (currentAddress == startAddress) {
                return {};
            }
            sentinelAddress = currentAddress;
            currentAddress = prevAddress;
            break;
        }
        prevAddress = currentAddress;
        currentAddress = currentNode.prev;
        path.push_front(currentNode);
        if (path.size() > 100) {
            // Circular list
            currentAddress = startAddress;
            sentinelAddress = startAddress;
            break;
        }
    }

    std::list<DWORD> addresses;
    // Traverse the list forwards, adding each item to the list until we reach the end
    while (true) {
        if (!ReadBufferToProcessMemory(hProcess, currentAddress, &currentNode, sizeof(currentNode))) {
            std::cout << "Error: Could not read from process memory. Line: " << __LINE__ << std::endl;
            break;
        }
        nodeList.push_back(currentNode);
        addresses.push_back(currentNode.content);
        if (currentNode.next == sentinelAddress) {
            break;
        }
        currentAddress = currentNode.next;
    }

    return nodeList;
}

void Utils::GetItems(std::list<Node> nodeList, HANDLE hProcess) {
    std::list<std::unique_ptr<ItemBase>> items;
    bool valid = true;
    for (const Node &node: nodeList) {
        int values[32];
        if (!ReadBufferToProcessMemory(hProcess, node.content, &values, sizeof(values))) {
            continue;
        }
        int type = values[0];
        int typeValue;
        ReadBufferToProcessMemory(hProcess, type, &typeValue, sizeof(typeValue));
        int actualValue;
        ReadBufferToProcessMemory(hProcess, typeValue + 0x4, &actualValue, sizeof(actualValue));
        if (actualValue == 1317794187) {
            std::unique_ptr<SingleItem> item = std::make_unique<SingleItem>(node.content);
            if (!item->isValid(hProcess)) {
                valid = false;
                break;
            }
            items.push_back(std::move(item));
        } else if (actualValue == 113766795) {
            std::unique_ptr<MultiItem> item = std::make_unique<MultiItem>(node.content);
            if (!item->isValid(hProcess)) {
                valid = false;
                break;
            }
            items.push_back(std::move(item));
        } else {
            valid = false;
        }
    }
    if (valid && items.size() > 0 && items.size() >= GameState::GetNumConveyorItems() - 1) {
        GameState::SetConveyorItems(std::move(items));
    }
}

void Utils::RecursivePrint(DWORD address, int depth, int items) {
    if (depth <= 0) {
        return;
    }
    std::cout << "Address: " << address << std::endl;
    std::cout << "Values:";
	std::vector<int> values(items);
	if (!ReadBufferToProcessMemory(GameState::GetHandle(), address, &values, values.size() * sizeof(int))) {
        std::cout << " (Unreadable)" << std::endl;
        return;
    }
    for (int i = 0; i < items; i++) {
        std::cout << " " << values[i];
    }
    std::cout << std::endl;
    RecursivePrint(values[0], depth - 1, items);
}

static std::unordered_map<std::wstring, Gdiplus::Image *> imageCache = {};

Gdiplus::Image *Utils::GetImageFromPath(std::wstring path) {
    if (imageCache.find(path) != imageCache.end()) {
        return imageCache[path];
    }
    Gdiplus::Image *image = Gdiplus::Image::FromFile(path.c_str());
    imageCache[path] = image;
    return image;
}