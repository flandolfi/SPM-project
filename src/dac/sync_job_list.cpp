//
// Created by flandolfi on 18/03/19.
//

#include <dac/scheduler.h>

#define ST_MEM_ORDER std::memory_order_release
#define LD_MEM_ORDER std::memory_order_consume


Scheduler::SyncJobList::SyncJobList() : remaining(0ull) {}

Scheduler::SyncJobList::SyncJobList(SyncJobList &&list) noexcept
    : queue(std::move(list.queue)), remaining(list.get_remaining()) {}

void Scheduler::SyncJobList::push(JobType &&item) {
    std::unique_lock<std::mutex> lock(mtx);

    queue.push_back(std::forward<JobType>(item));
    cv.notify_one();
}

bool Scheduler::SyncJobList::pop(JobType &item) {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&](){ return !queue.empty() || get_remaining() == 0; });

    if (queue.empty())
        return false;  // No more jobs

    item = std::move(queue.front());
    queue.pop_front();

    return true;
}

void Scheduler::SyncJobList::inc_remaining(unsigned long long by) {
    remaining.fetch_add(by, ST_MEM_ORDER);
}

void Scheduler::SyncJobList::dec_remaining(unsigned long long by) {
    if (remaining.fetch_sub(by, ST_MEM_ORDER) - by == 0) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.notify_all();  // All jobs are done, rejoice!
    }
}

unsigned long long Scheduler::SyncJobList::get_remaining() {
    return remaining.load(LD_MEM_ORDER);
}

void Scheduler::SyncJobList::clear() {
    queue = JobList();
    remaining = 0ul;
}
