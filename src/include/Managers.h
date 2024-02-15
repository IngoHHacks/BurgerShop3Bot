#ifndef BS3BOT_MANAGERS_H
#define BS3BOT_MANAGERS_H

#include <atomic>
#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include <functional>
#include <unordered_map>
#include <ntstatus.h>
#include <tlhelp32.h>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <list>
#include <vector>
#include <mutex>
#include <Content.h>

class GameState {
public:
    static float GetBBPercent();

    static float GetNumConveyorItems();

    static std::vector<std::unique_ptr<ItemBase>> GetConveyorItems();

    static std::vector<Customer> GetCustomers();

    static bool IsDirty();

    static HANDLE GetHandle();

    static HWND GetWindowHandle();

    static void SetBBPercent(float value);

    static void SetNumConveyorItems(int value);

    static void AddItemFromAddress(DWORD address);

    static void RemoveItemFromAddress(DWORD address);

    static void SetHandle(HANDLE pVoid);

    static void SetWindowHandle(HWND pVoid);

    static void SortConveyorItems();

    static void AddCustomer(Customer customer);

    static void RemoveCustomer(int index);

    static void Reset();

    static void IncrementFirstItem();

    static void DecrementFirstItem();

    static void Update();

    static bool CheckItemsDirty();

    static std::vector<std::unique_ptr<ItemBase>> conveyorItems;

    static void PerformActions();

private:
    static std::mutex conveyorItemsMutex;
    static std::mutex customersMutex;
    static float bbPercent;
    static int numConveyorItems;
    static std::vector<Customer> customers;
    static bool dirty;
    static bool needsSorting;
    static void *handle;
    static HWND windowHandle;
    static int timeWithNoCustomers;
};

class ItemManager {
public:
    static std::vector<std::string> itemNames;
    static std::unordered_map<int, ItemData> itemData;
    static std::unordered_map<std::string, ItemData> itemDataByName;

    static bool LoadItems(const std::string &filename);

    static std::string GetItemName(int id);

    static ItemData GetItemData(int id);

    static int GetNumItems();

    static bool LoadContent();

    static int IngredientLimit(int id);
};

class Breakpoint {

public:
    LPVOID address;

    Breakpoint() : originalByte(0), isActive(false) {}

    std::function<void(const DEBUG_EVENT &, HANDLE)> callback;
    BYTE originalByte;
    bool isActive;
};

class BreakpointManager {
private:
    HANDLE processHandle;
    std::unordered_map<LPVOID, Breakpoint> breakpoints;
    Breakpoint *lastBreakpoint;
    HANDLE threadHandle;

public:
    BreakpointManager(HANDLE process) : processHandle(process), lastBreakpoint(nullptr) {}

    bool SetBreakpoint(LPVOID address, std::function<void(const DEBUG_EVENT &, HANDLE)> callback);

    bool HandleBreakpoint(const DEBUG_EVENT &debugEvent);

    void ReinstateBreakpoint();

    void ClearSingleStep();
};

#endif // BS3BOT_MANAGERS_H
