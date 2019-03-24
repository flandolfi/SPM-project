//
// Created by flandolfi on 18/03/19.
//

#include <dac/scheduler.h>

#define ST_MEM_ORDER std::memory_order_release
#define LD_MEM_ORDER std::memory_order_consume
#define CMP [](const Schedule &lhs, const Schedule &rhs){ return lhs.second >= rhs.second; }


Scheduler::SyncJobList::SyncJobList() : queue(CMP), remaining(0ull) {}

Scheduler::SyncJobList::SyncJobList(SyncJobList &&list) noexcept
    : queue(std::move(list.queue)), remaining(list.get_remaining()) {}

void Scheduler::SyncJobList::push(Schedule &&item) {
    std::unique_lock<std::mutex> lock(mtx);

    queue.push(std::forward<Schedule>(item));
    cv.notify_one();
}

bool Scheduler::SyncJobList::pop(Schedule &item) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&](){ return !queue.empty() || get_remaining() == 0; });

    if (queue.empty())
        return false;

    item = std::move(const_cast<Schedule&>(queue.top()));
    queue.pop();

    return true;
}

void Scheduler::SyncJobList::inc_remaining(unsigned long long by) {
    remaining.fetch_add(by, ST_MEM_ORDER);
}

void Scheduler::SyncJobList::dec_remaining(unsigned long long by) {
    if (remaining.fetch_sub(by, ST_MEM_ORDER) - by == 0) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.notify_all();
    }
}

unsigned long long Scheduler::SyncJobList::get_remaining() {
    return remaining.load(LD_MEM_ORDER);
}

void Scheduler::SyncJobList::clear() {
    queue = std::priority_queue<Schedule, std::vector<Schedule>, Comparator>(CMP);
    remaining = 0ul;
}
