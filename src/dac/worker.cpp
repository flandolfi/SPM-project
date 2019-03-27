//
// Created by flandolfi on 18/03/19.
//

#include <dac/scheduler.h>


Scheduler::Worker::Worker(Scheduler &parent, unsigned long id) : parent(parent), id(id) {
#ifdef DEBUG
    char name[20];
    std::sprintf(name, "S%i_W%ld.csv", parent.id, id);
    file.open(name);

    log_header();
    log("CREATE", parent.id, id);
#endif
}

bool Scheduler::Worker::get_job(Scheduler::Schedule &job) {
#ifdef DEBUG
    log("RT_BGN", "", "");
#endif

    if (local_list.empty()) {
#ifdef DEBUG
        bool result = parent.global_list.pop(job);

        if (result)
            log("RT_GLB", job.second, "");
        else
            log("NO_JOB", "", "");

        return result;
#else
        return parent.global_list.pop(job);
#endif
    }

    job = std::move(local_list.back());
    local_list.pop_back();

#ifdef DEBUG
    log("RT_LOC", job.second, "");
#endif

    return true;
}

void Scheduler::Worker::schedule(Scheduler::Schedule &&job) {
#ifdef DEBUG
    log("SC_BGN", job.second, "");
#endif

    local_list.push_back(std::forward<Schedule>(job));

    if (!chi_squared_test()) {
        parent.global_list.push(std::move(local_list.front()));
        local_list.pop_front();

#ifdef DEBUG
        log("SC_GLB", "", "");
        return;
#endif
    }

#ifdef DEBUG
    log("SC_LOC", "", "");
#endif
}

bool Scheduler::Worker::chi_squared_test() {
    float par_degree = parent.n_workers;

    // Only local
    if (par_degree < 2 || parent.chi_limit == std::numeric_limits<float>::max())
        return true;

    // Only global
    if (parent.chi_limit < 0)
        return false;

    auto remaining = parent.global_list.get_remaining();

    // Straight to global (and avoid divide by 0)
    if (remaining == 0)
        return false;

    float obs_jobs = local_list.size();
    float exp_jobs = remaining / par_degree;

    // Skip test: jobs are less than expected!
    if (obs_jobs < exp_jobs) {
#ifdef DEBUG
        log("CHI_SK", obs_jobs, exp_jobs);
#endif

        return true;
    }

    float chi_square = (obs_jobs - exp_jobs)*(obs_jobs - exp_jobs);
    chi_square += chi_square/(par_degree - 1.f);
    chi_square /= exp_jobs;

#ifdef DEBUG
    if (chi_square <= parent.chi_limit)
        log("CHI_OK", chi_square, parent.chi_limit);
    else
        log("CHI_NO", chi_square, parent.chi_limit);
#endif

    return chi_square <= parent.chi_limit;
}

#ifdef DEBUG
std::chrono::time_point<std::chrono::high_resolution_clock> Scheduler::Worker::START = std::chrono::high_resolution_clock::now();

template <typename T1, typename T2>
void Scheduler::Worker::log(const char* code, T1 info1, T2 info2) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - Scheduler::Worker::START;

    file << 1000.f*elapsed.count() << "," << id << "," << code << "," << info1 << "," << info2 << std::endl;
}

void Scheduler::Worker::log_header() {
    file << "time,id,code,info1,info2" << std::endl;
}
#endif