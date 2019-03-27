//
// Created by flandolfi on 16/03/19.
//

#include <dac/scheduler.h>

#define P_VALUE_0_750 0.101
#define P_VALUE_0_500 0.455
#define P_VALUE_0_250 1.323
#define P_VALUE_0_200 1.642
#define P_VALUE_0_100 2.706
#define P_VALUE_0_050 3.841
#define P_VALUE_0_020 5.412
#define P_VALUE_0_010 6.635
#define P_VALUE_0_005 7.879
#define P_VALUE_0_002 9.550
#define P_VALUE_0_001 10.828


#ifdef DEBUG
unsigned int Scheduler::ID = 0;
#endif

Scheduler::Scheduler(unsigned long n_workers, bool global_fifo, Scheduler::Policy policy)
        : global_list(global_fifo), n_workers(n_workers) {
    for (auto id = 0ul; id < n_workers; ++id)
        workers.emplace_back(*this, id);

    set_policy(policy);

#ifdef DEBUG
    id = Scheduler::ID++;
#endif
}

void Scheduler::schedule(Scheduler::JobType &&job, unsigned long to) {
    global_list.inc_remaining();
    workers[to].schedule(std::forward<JobType>(job));
}

void Scheduler::mark_done(unsigned long from) {
    global_list.dec_remaining();

#ifdef DEBUG
    workers[from].log("J_DONE", "", "");
#endif
}

void Scheduler::set_policy(Scheduler::Policy policy) {
    switch (policy) {
        case Policy::relaxed:
            chi_limit = P_VALUE_0_005;
            break;

        case Policy::strict:
            chi_limit = P_VALUE_0_050;
            break;

        case Policy::strong:
            chi_limit = P_VALUE_0_500;
            break;

        case Policy::perfect:
            chi_limit = 0;
            break;

        case Policy::only_local:
            chi_limit = std::numeric_limits<float>::max();
            break;

        case Policy::only_global:
            chi_limit = -1;
            break;
    }
}

void Scheduler::reset(unsigned long n_workers, Policy policy) {
    this->n_workers = n_workers;
    global_list.clear();
    workers.clear();

    for (auto id = 0ul; id < n_workers; ++id)
        workers.emplace_back(*this, id);

    set_policy(policy);
}

bool Scheduler::get_job(Scheduler::JobType &job, unsigned long from) {
    return workers[from].get_job(job);
}

unsigned long long Scheduler::remaining_jobs() {
    return global_list.get_remaining();
}
