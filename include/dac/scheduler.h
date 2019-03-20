//
// Created by flandolfi on 16/03/19.
//

#ifndef SPM_PROJECT_SCHEDULER_H
#define SPM_PROJECT_SCHEDULER_H

#include <future>
#include <vector>
#include <random>
#include <list>
#include <queue>
#include <iostream>

#ifdef DEBUG
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#endif


class Scheduler {
public:
    using JobType = std::function<void(unsigned long)>;
    enum class Policy { relaxed, strict, strong, perfect, only_local, only_global };

    explicit Scheduler(unsigned long n_workers = 1ul,
                       Policy policy = Policy::only_global);
    void schedule(JobType &&job, long priority, unsigned long to);
    bool get_job(JobType &job, unsigned long from);
    void mark_done(unsigned long from);
    void set_policy(Policy policy);
    void reset(unsigned long n_workers, Policy policy);

private:
    using Schedule = std::pair<JobType, long>;

    class SyncJobList {
    private:
        using Comparator = std::function<bool(const Schedule&, const Schedule&)>;

        std::priority_queue<Schedule, std::vector<Schedule>, Comparator> queue;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic_ullong remaining;

    public:
        explicit SyncJobList();
        SyncJobList(SyncJobList&& sync_list) noexcept;
        void push(Schedule &&item);
        bool pop(Schedule &item);
        void clear();
        void inc_remaining(unsigned long long by = 1ull);
        void dec_remaining(unsigned long long by = 1ull);
        unsigned long long get_remaining();
    };

    class Worker {
    private:
        std::list<Schedule> local_list;
        Scheduler& parent;
        unsigned long id;

        bool chi_square_test();

    public:
#ifdef DEBUG
        std::ofstream file;

        void log_time();
#endif

        explicit Worker(Scheduler& parent, unsigned long id);
        bool get_job(Schedule &job);
        void schedule(Schedule&& job);
    };

    SyncJobList global_list;
    std::vector<Worker> workers;
    unsigned long n_workers;
    float chi_limit;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<std::mt19937::result_type> dist;

#ifdef DEBUG
    static unsigned int ID;
    unsigned int id;
#endif
};

#endif //SPM_PROJECT_SCHEDULER_H
