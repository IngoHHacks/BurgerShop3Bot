#ifndef MEMREAD_DEBUGGING_H
#define MEMREAD_DEBUGGING_H


class Debugging {
public:
    static void EnqueueEvent(const DEBUG_EVENT &debugEvent, std::function<void(const DEBUG_EVENT &, HANDLE)> callback,
                             HANDLE handle);

    static void DebugLoop();
};


#endif //MEMREAD_DEBUGGING_H
