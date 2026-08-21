#pragma once

#include <atomic>

// Coordinates the background network worker with LVGL model rendering.
// Touch processing continues while busy; screens simply retain their last
// rendered values until a service update has completed.
class NetworkUpdateState
{
public:
    static bool isBusy()
    {
        return storage().load(std::memory_order_acquire);
    }

    static void setBusy(bool value)
    {
        storage().store(value, std::memory_order_release);
    }

private:
    static std::atomic<bool>& storage()
    {
        static std::atomic<bool> busy{false};
        return busy;
    }
};
