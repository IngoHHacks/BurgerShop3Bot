#ifndef BS3BOT_CONTENT_H
#define BS3BOT_CONTENT_H

#include <atomic>
#include <objidl.h>
#include <windows.h>
#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include <functional>
#include <unordered_map>
#include <ntstatus.h>
#include <tlhelp32.h>
#include <fstream>
#include <iostream>
#include <filesystem>

class MemoryProbe {
public:
    explicit MemoryProbe(DWORD address) : address(address) {}

    virtual ~MemoryProbe() = default;

    DWORD GetAddress();

    void PrintInts(int length);

    void PrintBytes(int length);

    void PrintFloats(int length);

protected:
    DWORD address;
    int hash = 0;
};

class ItemBase : public MemoryProbe {
public:
    explicit ItemBase(DWORD address) : MemoryProbe(address) {}
};

class SimpleItem : public ItemBase {
public:
    explicit SimpleItem(DWORD address) : ItemBase(address) {}

    bool isValid(HANDLE hProcess);

    int GetItemId(HANDLE hProcess);

    int GetIngredientId(HANDLE hProcess);

    int GetConveyorIndex(HANDLE hProcess);

    std::string GetName(HANDLE hProcess);

    std::string GetIngredientName(HANDLE hProcess);

    float GetX(HANDLE hProcess);

    float GetY(HANDLE hProcess);

    std::pair<float, float> GetPos(HANDLE hProcess);

    float GetMouseX(HANDLE hProcess);

    float GetMouseY(HANDLE hProcess);

    std::pair<float, float> GetMousePos(HANDLE hProcess);

    int GetLayer(HANDLE hProcess);

    void SetItemId(HANDLE hProcess, int value);

    void SetIngredientId(HANDLE hProcess, int value);

    bool HasChanged();

private:
    int _itemId = -1;
    int _ingredientId = -1;
};

class ComplexItem : public ItemBase {
public:
    explicit ComplexItem(DWORD address) : ItemBase(address) {}

    bool isValid(HANDLE hProcess);

    std::list<SimpleItem> GetItems(HANDLE hProcess);

    int GetConveyorIndex(HANDLE hProcess);

    float GetX(HANDLE hProcess);

    float GetY(HANDLE hProcess);

    bool HasChanged();
};

struct ItemData {
    int id;
    std::string name;
    std::string type;
    std::string oven = "";
    std::string pot = "";
    std::string pan = "";

    ItemData(int id, const std::string &name, const std::string &type, const std::string &oven, const std::string &pot,
             const std::string &pan) : id(id), name(name), type(type), oven(oven), pot(pot), pan(pan) {}

    ItemData() : id(-1), name(""), type("") {}
};

struct Point {
    float x;
    float y;

    Point(float x, float y) : x(x), y(y) {}
};

struct ItemInfo {
    int mNumCopies;
    int mNumComplete;
    bool mRobotComplete;
    DWORD mItem;
    DWORD mFlyList;
    bool mDraw;
    bool mInThoughtBubble;
    bool mRequired;
    bool mIsFree;
    int mHideCount;
    Point mGroupOffset;
    bool mHiliteAll;
    bool mSpecialDraw;
    int mCustomParam;

    ItemInfo() : mNumCopies(0), mNumComplete(0), mRobotComplete(false), mItem(0), mFlyList(0), mDraw(false),
                 mInThoughtBubble(false), mRequired(false), mIsFree(false), mHideCount(0), mGroupOffset(
                    Point(0, 0)), mHiliteAll(false), mSpecialDraw(false), mCustomParam(0) {}

    std::unique_ptr<ItemBase> GetItem();
};

class Customer : public MemoryProbe {
public:
    Customer(DWORD address) : MemoryProbe(address) {}

    bool isValid(HANDLE hProcess);

    int GetId(HANDLE hProcess);

    std::list<ItemInfo> GetItems(HANDLE hProcess);
};

#endif // BS3BOT_CONTENT_H
