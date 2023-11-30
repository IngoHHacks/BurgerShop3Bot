#include <Windows.h>
#include <Utils.h>
#include <Managers.h>
#include <Content.h>
#include <atomic>
#include <list>
#include <functional>
#include <ntstatus.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>
#include <queue>
#include <condition_variable>
#include <Debugging.h>

#ifndef VERSION_0_5_8A
#define VERSION_0_5_8A
#endif

std::mutex queueMutex;
std::condition_variable queueCondition;
std::queue<std::pair<DEBUG_EVENT, std::function<void(const DEBUG_EVENT &, HANDLE)>>> eventQueue;

void WorkerThread(HANDLE handle) {
    while (true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondition.wait(lock, [] { return !eventQueue.empty(); });

        auto item = eventQueue.front();
        eventQueue.pop();

        lock.unlock();

        // Execute the callback with the event and handle
        item.second(item.first, handle);
    }
}

void Debugging::EnqueueEvent(const DEBUG_EVENT &debugEvent, std::function<void(const DEBUG_EVENT &, HANDLE)> callback,
                             HANDLE handle) {
    std::lock_guard<std::mutex> lock(queueMutex);
    eventQueue.push(std::make_pair(debugEvent, callback));
    queueCondition.notify_one();
}

void Debugging::DebugLoop() {

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD pid = 0;
    if (Process32First(snapshot, &entry)) {
        do {
            if (!strcmp(entry.szExeFile, "BurgerShop3.exe")) {
                std::cout << "Found BurgerShop3.exe, PID: " << entry.th32ProcessID << std::endl;
                std::cout << "Attaching to process..." << std::endl;
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
        if (pid == 0) {
            std::cout << "Error: Could not find BurgerShop3.exe." << std::endl;
            exit(0);
        }
    } else {
        std::cout << "Error: Could not enumerate processes." << std::endl;
        exit(0);
    }
    CloseHandle(snapshot);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    GameState::SetHandle(hProcess);
    BreakpointManager bpManager(hProcess);
    std::thread workerThread(WorkerThread, hProcess);

    /*
     * F3 0F10 96 4C010000 | movss xmm2,[esi+0000014C]
     * 8D 8E 48010000      | lea ecx,[esi+00000148]
     * > F3 0F10 09        | movss xmm1,[ecx] <-- Breakpoint here
     * 0F2E D1             | ucomiss xmm2,xmm1
     * 9F                  | lahf
     * Literal: F30F10964C0100008D8E48010000F30F10090F2ED19F
     */
    LPVOID baseAddress = (LPVOID) Utils::GetModuleBaseAddress(pid, "BurgerShop3.exe");
#if defined(VERSION_0_5_7D)
    LPVOID bbOffset = (LPVOID) 0x160143;
#elif defined(VERSION_0_5_8A)
    LPVOID bbOffset = (LPVOID) 0x163373;
#endif
    LPVOID bbAddress = (LPVOID) ((uintptr_t) baseAddress + (uintptr_t) bbOffset);
    bpManager.SetBreakpoint(bbAddress, [](const DEBUG_EVENT &debugEvent, HANDLE hProcess) {
        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (hThread != NULL) {
            WOW64_CONTEXT context;
            if (Utils::GetWow64ThreadContext(hThread, context)) {
                DWORD_PTR address = context.Ecx;
                float value;
                if (!ReadProcessMemory(hProcess, (LPVOID) address, &value, sizeof(value), NULL)) {
                    return;
                }
                GameState::SetBBPercent(value);
            }
            CloseHandle(hThread);
        }
    });

    /*
     * 8B F9            | mov edi,ecx
     * E8 ????????      | call BurgerShop3.exe+????????
     * > 8B 87 0C010000 | mov eax,[edi+0000010C] <-- Breakpoint here
     * 89 45 E0         | mov [ebp-20],eax
     * 85 C0            | test eax,eax
     * Literal: 8BF9E8????????8B870C0100008945E085C0
     */
#if defined(VERSION_0_5_7D)
    LPVOID conveyorSizeOffset = (LPVOID) 0x4F16E;
#elif defined(VERSION_0_5_8A)
    LPVOID conveyorSizeOffset = (LPVOID) 0x4FCCE;
#endif
    LPVOID conveyorSizeAddress = (LPVOID) ((uintptr_t) baseAddress + (uintptr_t) conveyorSizeOffset);
    bpManager.SetBreakpoint(conveyorSizeAddress, [](const DEBUG_EVENT &debugEvent, HANDLE hProcess) {
        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (hThread != NULL) {
            WOW64_CONTEXT context;
            if (Utils::GetWow64ThreadContext(hThread, context)) {
                DWORD_PTR address = context.Edi + 0x10C;
                int value;
                if (!ReadProcessMemory(hProcess, (LPVOID) address, &value, sizeof(value), NULL)) {
                    return;
                }
                GameState::SetNumConveyorItems(value);
            }
            CloseHandle(hThread);
        }
    });

    /*
     * 89 42 04 | mov [edx+04],eax
     * 89 51 04 | mov [ecx+04],edx
     * >> 89 10 | mov [eax],edx <-- Breakpoint here
     * 8B C2    | mov eax,edx
     * 8B 4D F4 | mov ecx,[ebp-0C]
     * Literal: 89420489510489108BC28B4DF4
     */
#if defined(VERSION_0_5_7D)
    LPVOID addToConveyorOffset = (LPVOID) 0x3E103;
#elif defined(VERSION_0_5_8A)
    LPVOID addToConveyorOffset = (LPVOID) 0x3EB63;
#endif
    LPVOID addToConveyorAddress = (LPVOID) ((uintptr_t) baseAddress + (uintptr_t) addToConveyorOffset);
    bpManager.SetBreakpoint(addToConveyorAddress, [](const DEBUG_EVENT &debugEvent, HANDLE hProcess) {
        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (hThread != NULL) {
            WOW64_CONTEXT context;
            if (Utils::GetWow64ThreadContext(hThread, context)) {
                DWORD startAddress = context.Edx;
                std::list<Node> nodeList = Utils::TraverseAndCollectNodes(hProcess, startAddress);
                Utils::GetItems(nodeList, hProcess);

            }
            CloseHandle(hThread);
        }
    });

    /*
     * 8B 46 04          | mov eax,[esi+04]
     * 89 41 04          | mov [ecx+04],eax
     * >> FF 8F 0C010000 | dec [edi+0000010C] <-- Breakpoint here
     * 8B 4E 08          | mov ecx,[esi+08]
     * 85 C9             | test ecx,ecx
     * Literal: 8B4604894104FF8F0C0100008B4E0885C9
     */
#if defined(VERSION_0_5_7D)
    LPVOID removeFromConveyorOffset = (LPVOID) 0x4D489;
#elif defined(VERSION_0_5_8A)
    LPVOID removeFromConveyorOffset = (LPVOID) 0x409A4;
#endif
    LPVOID removeFromConveyorAddress = (LPVOID) ((uintptr_t) baseAddress + (uintptr_t) removeFromConveyorOffset);
    bpManager.SetBreakpoint(removeFromConveyorAddress, [](const DEBUG_EVENT &debugEvent, HANDLE hProcess) {
        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (hThread != NULL) {
            WOW64_CONTEXT context;
            if (Utils::GetWow64ThreadContext(hThread, context)) {
                DWORD startAddress = context.Edi + 0x0000010C;
                std::list<Node> nodeList = Utils::TraverseAndCollectNodes(hProcess, startAddress);
                Utils::GetItems(nodeList, hProcess);
            }
            CloseHandle(hThread);
        }
    });

    /*
     * 8B 03          | mov eax,[ebx]
     * 8B CB          | mov ecx,ebx
     * >> 57          | push edi <-- Breakpoint here
     * FF 90 ???????? | call dword ptr [eax+????????]
     * FF 87 ???????? | inc [edi+????????]
     * Literal: 8B038BCB57FF90????????FF87????????
     */
#if defined(VERSION_0_5_7D)
    LPVOID customerOffset = (LPVOID) 0x16756;
#elif defined(VERSION_0_5_8A)
    LPVOID customerOffset = (LPVOID) 0x169B6;
#endif
    LPVOID customerAddress = (LPVOID) ((uintptr_t) baseAddress + (uintptr_t) customerOffset);
    bpManager.SetBreakpoint(customerAddress, [](const DEBUG_EVENT &debugEvent, HANDLE hProcess) {
        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
        if (hThread != NULL) {
            WOW64_CONTEXT context;
            if (Utils::GetWow64ThreadContext(hThread, context)) {
                DWORD address = context.Ebx;
                Customer customer(address);
                if (customer.isValid(hProcess)) {
                    GameState::AddCustomer(customer);
                }
            }
            CloseHandle(hThread);
        }
    });

    if (DebugActiveProcess(pid)) {
        DEBUG_EVENT debugEvent;
        while (true) {
            while (WaitForDebugEvent(&debugEvent, INFINITE)) {
                DWORD continueStatus = DBG_CONTINUE;
                if (debugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
                    long startTick = GetTickCount();
                    long time;
                    switch (debugEvent.u.Exception.ExceptionRecord.ExceptionCode) {
                        case static_cast<DWORD>(EXCEPTION_BREAKPOINT):
                        case static_cast<DWORD>(STATUS_WX86_BREAKPOINT):
                            if (!bpManager.HandleBreakpoint(debugEvent)) {
                                continueStatus = DBG_EXCEPTION_NOT_HANDLED;
                            } else {
                                // Decrement the instruction pointer to repeat the instruction that was interrupted
                                CONTEXT context;
                                context.ContextFlags = CONTEXT_CONTROL;
                                HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
                                if (hThread != NULL) {
#ifdef _WIN64
                                    WOW64_CONTEXT context;
                                    if (Utils::GetWow64ThreadContext(hThread, context)) {
                                        context.Eip--;
                                        if (!Utils::SetWow64ThreadContext(hThread, context)) {
                                            std::cout << "Error: Could not set thread context." << std::endl;
                                        }
                                    } else {
                                        std::cout << "Error: Could not get thread context." << std::endl;
                                    }
#else
                                    if (GetThreadContext(hThread, &context)) {
                                        context.Eip--;
                                        if (!SetThreadContext(hThread, context)) {
                                            std::cout << "Error: Could not set thread context." << std::endl;
                                        }
                                    } else {
                                        std::cout << "Error: Could not get thread context." << std::endl;
                                    }
#endif
                                    CloseHandle(hThread);
                                }
                            }
                            time = GetTickCount() - startTick;
                            if (time > 50) {
                                std::cerr << "Warning: Breakpoint callback took " << time << "ms to execute."
                                          << std::endl;
                            } else if (time > 1) {
                                std::cout << "Breakpoint callback took " << time << "ms to execute." << std::endl;
                            }
                            break;
                        case static_cast<DWORD>(EXCEPTION_SINGLE_STEP):
                        case static_cast<DWORD>(STATUS_WX86_SINGLE_STEP):
                            bpManager.ReinstateBreakpoint();
                            bpManager.ClearSingleStep();
                            break;
                        default:
                            continueStatus = DBG_EXCEPTION_NOT_HANDLED;
                            break;
                    }
                }
                ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);
            }
        }
    } else {
        std::cout << "Error: Could not attach debugger to process." << std::endl;
    }
    CloseHandle(hProcess);
}