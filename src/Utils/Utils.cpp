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

/**
 * Get the base address of a module in a process.
 * @param procId The process ID.
 * @param modName The name of the module.
 * @return The base address of the module.
 */
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

#ifdef _WIN64
/**
 * Get the 32-bit thread context of a thread on a 64-bit system.
 * @param hThread The thread handle.
 * @param context (out) The thread context.
 */
bool Utils::GetWow64ThreadContext(HANDLE hThread, WOW64_CONTEXT &context) {
    ZeroMemory(&context, sizeof(WOW64_CONTEXT));
    context.ContextFlags = CONTEXT_FULL;

    if (!Wow64GetThreadContext(hThread, &context)) {
        std::cerr << "Failed to get 32-bit thread context." << std::endl;
        return false;
    }

    return true;
}

/**
 * Set the 32-bit thread context of a thread on a 64-bit system.
 * @param hThread The thread handle.
 * @param context The thread context.
 */
bool Utils::SetWow64ThreadContext(HANDLE hThread, WOW64_CONTEXT &context) {
    if (!Wow64SetThreadContext(hThread, &context)) {
        std::cerr << "Failed to set 32-bit thread context." << std::endl;
        return false;
    }

    return true;
}
#endif

/**
 * Read a buffer from process memory.
 * @param processHandle The process handle.
 * @param address The address to read from.
 * @param buffer (out) The buffer to read into.
 * @param size The size of the buffer.
 * @return @c true if the read was successful. @c false if the read failed or the number of bytes read was not equal to
 * the size of the buffer.
 */
bool Utils::ReadBufferToProcessMemory(HANDLE processHandle, DWORD address, LPVOID buffer, SIZE_T size) {
    SIZE_T bytesRead;
    return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(static_cast<DWORD_PTR>(address)), buffer,
                             size, &bytesRead) && bytesRead == size;
}

/**
 * Write a buffer to process memory.
 * @param processHandle The process handle.
 * @param address The address to write to.
 * @param buffer The buffer to write.
 * @param size The size of the buffer.
 * @return @c true if the write was successful. @c false if the write failed or the number of bytes written was not
 * equal to the size of the buffer.
 */
bool Utils::WriteBufferToProcessMemory(HANDLE processHandle, DWORD address, LPCVOID buffer, SIZE_T size) {
    SIZE_T bytesWritten;
    return WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(static_cast<DWORD_PTR>(address)), buffer,
                              size, &bytesWritten) && bytesWritten == size;
}

/**
 * Check if a node is an item.
 * @param hProcess The process handle.
 * @param address The address of the node.
 * @param node The node.
 * @return @c true if the node is not an item. @c false if the node is an item.
 * @note This function is intended for determining sentinel nodes in doubly-linked lists of items.
 */
bool Utils::IsNotItem(HANDLE hProcess, DWORD address, const Node &node) {
    // 1. Try reading content. If it fails, we know it's not an item.
    int value;
    if (!ReadBufferToProcessMemory(hProcess, node.content, &value, sizeof(value))) {
        return true;
    }
    // 2. Try reading the value as a pointer. If it fails, we know it's not an item.
    int pointer;
    if (!ReadBufferToProcessMemory(hProcess, value, &pointer, sizeof(pointer))) {
        return true;
    }
    // 3. Try reading the pointer + 0x4 as pointer. If it fails or is not 1317794187 or 113766795, we know it's not an
    // item.
    int pointerToPointer;
    if (!ReadBufferToProcessMemory(hProcess, pointer + 0x4, &pointerToPointer, sizeof(pointerToPointer))) {
        return true;
    }
    if (pointerToPointer != 1317794187 && pointerToPointer != 113766795) {
        return true;
    }
    // Otherwise, it's an item.
    return false;
}

/**
 * Try to find food in a list of nodes.
 * @param hProcess The process handle.
 * @param startAddress The address of the first node in the list.
 * @param depth The depth to search.
 * @param width The width to search.
 * @param path (out) The path to the food.
 * @return @c true if food was found. @c false if food was not found.
 * @warning This function is intended for memory analysis. It is not recommended to use this function in production.
 */
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

/**
 * Traverse a list of nodes and collect the nodes from a doubly-linked list of items.
 * @param hProcess The process handle.
 * @param startAddress The address of anode in the list.
 * @return A list of the nodes in the list.
 */
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

        if (IsNotItem(hProcess, currentAddress, currentNode)) {
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

/**
 * Get the conveyor items from a list of nodes and sets them to the game state.
 * @param nodeList The list of nodes.
 * @param hProcess The process handle.
 */
void Utils::ApplyConveyorItems(std::list<Node> nodeList, HANDLE hProcess) {
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
            std::unique_ptr<SimpleItem> item = std::make_unique<SimpleItem>(node.content);
            if (!item->isValid(hProcess)) {
                valid = false;
                break;
            }
            items.push_back(std::move(item));
        } else if (actualValue == 113766795) {
            std::unique_ptr<ComplexItem> item = std::make_unique<ComplexItem>(node.content);
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

/**
 * Recursively print a list of nodes.
 * @param address The address of the first node in the list.
 * @param depth The depth to print.
 * @param items The number of items to print.
 * @warning This function is intended for memory analysis. It is not recommended to use this function in production.
 */
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

/**
 * Get an image from a path.
 * @param path The path to the image.
 * @return The image.
 * @note This function caches images for performance.
 */
Gdiplus::Image *Utils::GetImageFromPath(std::wstring path) {
    if (imageCache.find(path) != imageCache.end()) {
        return imageCache[path];
    }
    Gdiplus::Image *image = Gdiplus::Image::FromFile(path.c_str());
    imageCache[path] = image;
    return image;
}