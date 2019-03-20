//
// Created by flandolfi on 18/03/19.
//

#include <dac/scheduler.h>


Scheduler::Worker::Worker(Scheduler &parent, unsigned long id) : parent(parent), id(id) {
#ifdef DEBUG
    char name[20];
    std::sprintf(name, "S%i_W%ld.txt", parent.id, id);
    file.open(name);
#endif
}

bool Scheduler::Worker::get_job(Scheduler::Schedule &job) {
    if (local_list.empty()) {
#ifdef DEBUG
        bool result = parent.global_list.pop(job);
        log_time();
        file << "Retrieved GLOBAL job with priority " << job.second << std::endl;

        return result;
#else
        return parent.global_list.pop(job);
#endif
    }

    job = std::move(local_list.back());
    local_list.pop_back();

#ifdef DEBUG
    log_time();
    file << "Retrieved LOCAL job with priority " << job.second << std::endl;
#endif

    return true;
}

void Scheduler::Worker::schedule(Scheduler::Schedule &&job) {
#ifdef DEBUG
    log_time();
    file << "Scheduling job with priority " << job.second << "..." << std::endl;
#endif

    local_list.push_back(std::forward<Schedule>(job));

    if (!chi_square_test()) {
        parent.global_list.push(std::move(local_list.front()));
        local_list.pop_front();

#ifdef DEBUG
        log_time();
        file << "Job scheduled GLOBALLY" << std::endl;

        return;
#endif
    }

#ifdef DEBUG
    log_time();
    file << "Job scheduled LOCALLY" << std::endl;
#endif
}

bool Scheduler::Worker::chi_square_test() {
    float par_degree = parent.n_workers;

    if (parent.chi_limit < 0 || par_degree < 2)
        return false;

    auto remaining = parent.global_list.get_remaining();

    if (remaining == 0)
        return false;

    float obs_jobs = local_list.size();
    float exp_jobs = remaining / par_degree;
    float chi_square = (obs_jobs - exp_jobs)*(obs_jobs - exp_jobs);
    chi_square += chi_square/(par_degree - 1.f);
    chi_square /= exp_jobs;

#ifdef DEBUG
    log_time();
    file << "Chi-Square test " << (chi_square <= parent.chi_limit? "passed" : "failed")
         << " with " << chi_square << " (LIMIT " << parent.chi_limit << ")" << std::endl;
#endif

    return chi_square <= parent.chi_limit;
}

#ifdef DEBUG
void Scheduler::Worker::log_time() {
    file << "[" << std::setfill(' ') << std::setw(8) << std::clock() << "] ";
    file << "THREAD " << id << ": ";
}
#endif