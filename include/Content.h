#ifndef MEMREAD_CONTENT_H
#define MEMREAD_CONTENT_H

#include <atomic>
#include <objidl.h>
#include <windows.h>
#include <gdiplus.h>
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

class SingleItem : public ItemBase {
public:
    explicit SingleItem(DWORD address) : ItemBase(address) {}

    bool isValid(HANDLE hProcess);

    int GetItemId(HANDLE hProcess);

    int GetIngredientId(HANDLE hProcess);

    int GetConveyorIndex(HANDLE hProcess);

    std::string GetName(HANDLE hProcess);

    std::string GetIngredientName(HANDLE hProcess);

    float GetX(HANDLE hProcess);

    float GetY(HANDLE hProcess);

    int GetLayer(HANDLE hProcess);

    void SetItemId(HANDLE hProcess, int value);

    void SetIngredientId(HANDLE hProcess, int value);

    bool HasChanged();
};

class MultiItem : public ItemBase {
public:
    explicit MultiItem(DWORD address) : ItemBase(address) {}

    bool isValid(HANDLE hProcess);

    std::list<SingleItem> GetItems(HANDLE hProcess);

    int GetConveyorIndex(HANDLE hProcess);

    float GetX(HANDLE hProcess);

    float GetY(HANDLE hProcess);

    bool HasChanged();
};

struct ItemData {
    std::string type;
    int index;
    int adjustX;
    int adjustY;
    int oX;
    int oY;
    int absX;
    int absY;
    int height;
    int cornerX;
    int cornerY;
    int layer;
    std::vector<ItemData> subItems;

    ItemData() : index(-1), adjustX(0), adjustY(0), oX(0), oY(0), absX(0), absY(0), height(0), cornerX(0), cornerY(0),
                 layer(0) {
        subItems = {};
    }
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
    Gdiplus::Point mGroupOffset;
    bool mHiliteAll;
    bool mSpecialDraw;
    int mCustomParam;

    ItemInfo() : mNumCopies(0), mNumComplete(0), mRobotComplete(false), mItem(0), mFlyList(0), mDraw(false),
                 mInThoughtBubble(false), mRequired(false), mIsFree(false), mHideCount(0), mGroupOffset(
                    Gdiplus::Point(0, 0)), mHiliteAll(false), mSpecialDraw(false), mCustomParam(0) {}

    std::unique_ptr<ItemBase> GetItem();
};

class Customer : public MemoryProbe {
public:
    Customer(DWORD address) : MemoryProbe(address) {}

    bool isValid(HANDLE hProcess);

    int GetId(HANDLE hProcess);

    std::list<ItemInfo> GetItems(HANDLE hProcess);
};

#endif // MEMREAD_CONTENT_H
