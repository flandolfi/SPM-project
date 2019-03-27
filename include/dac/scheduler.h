/**
 * @file scheduler.h
 * @brief Contains the Scheduler class header.
 *
 * @author Francesco Landolfi
 */

#ifndef SPM_PROJECT_SCHEDULER_H
#define SPM_PROJECT_SCHEDULER_H

#include <future>
#include <vector>
#include <list>
#include <queue>

#ifdef DEBUG
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#endif

/**
 * @class Scheduler
 * @brief A parallel and general-purpose task scheduler.
 *
 * This scheduler divides the scheduled tasks over multiple threads, each one owning a local queue of jobs, and a global
 * queue accessible by all threads.
 */
class Scheduler {
public:
    using JobType = std::function<void(unsigned long)>; /** Type alias */

    /**
     * @enum Policy
     * @brief Balancing policy adopted by the scheduler.
     *
     * The values of this enum class should be used to regulate the amount of jobs to be scheduled in the global queue.
     * The tasks to be scheduled will fall in the local queues only depending on the current balancing, i.e., on the
     * number of jobs in the local queue of a thread w.r.t. the overall jobs remaining. In detail, taking as null
     * hypothesis that the jobs are perfectly evenly distributed between the threads, we have:
     *     - "relaxed": the probability to observe the current size of local queue of the thread is higher than 0.005;
     *     - "strict": the probability to observe the current size of local queue of the thread is higher than 0.05;
     *     - "strong": the probability to observe the current size of local queue of the thread is higher than 0.5;
     *     - "perfect": the local queue is perfectly balanced.
     *
     * Other options are:
     *     - "only_local": the task will be scheduled in the given local queue;
     *     - "only_global": the task will be scheduled in the global queue;
     *
     * Notice that this impacts in the performance in opposite ways: the more jobs are scheduled in the global queue,
     * the more they will be evenly distributed, but since the accesses to the global queue are serialized, the process
     * will not scale up. Vice versa, the more jobs are scheduled locally, the more we will observe a better
     * parallelization, but they may not be evenly distributed.     *
     */
    enum class Policy { relaxed, strict, strong, perfect, only_local, only_global };

    /**
     * Creates a Scheduler instance.
     *
     * @param n_workers number of parallel threads used to compute the scheduled tasks
     * @param policy the balancing policy
     */
    explicit Scheduler(unsigned long n_workers = 1ul, Policy policy = Policy::only_global);

    /**
     * Schedules a task to the given thread. It will increase the global job counter by 1.
     *
     * @warning It is not ensured that the specified thread will eventually compute the task (it depends on the given
     *     balancing policy).
     * @param job the task to be executed
     * @param to the recipient thread ID (it should be a number between 0 and @p n_workers - 1)
     */
    void schedule(JobType &&job, unsigned long to);

    /**
     * Retrieves a task from the local queue of a given thread. If the local queue is empty, it will be retrieved from
     * the global queue.
     *
     * @warning If there are no task in the local queue nor in the global one, this method will halt until either a job
     * is scheduled (either locally or globally) or the internal counter is set to 0 (@see Scheduler:mark_done).
     * @warning Calling this function will not decrease the global counter by 1.
     * @param job the retrieved job
     * @param from the thread ID (it should be a number between 0 and @p n_workers - 1)
     * @return true if a job is found, false if there will be no more jobs to be retrieved.
     */
    bool get_job(JobType &job, unsigned long from);

    /**
     * Set a job as done (i.e., decrease the global job counter by 1).
     *
     * @param from the thread that completed the task (useful only for debugging purposes)
     */
    void mark_done(unsigned long from);

    /**
     * Sets the policy of the scheduler.
     *
     * @param policy the new policy to be adopted
     */
    void set_policy(Policy policy);

    /**
     * Gets the number of remaining jobs to be completed.
     *
     * @return the number of scheduled jobs remaining
     */
    unsigned long long remaining_jobs();

    /**
     * Resets the scheduler. It will erase any pending task and reset the internal job counter.
     *
     * @param n_workers the new number of parallel threads to be employed
     * @param policy the new policy to be adopted
     */
    void reset(unsigned long n_workers, Policy policy);

private:
    using JobList = std::list<JobType>;

    // This is just a synchronized version of the priority_queue of the standard library. It will be also maintain the
    // number of remaining jobs to be completed.
    class SyncJobList {
    private:
        JobList queue;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic_ullong remaining;

    public:
        explicit SyncJobList();
        SyncJobList(SyncJobList&& sync_list) noexcept;
        void push(JobType &&item);
        bool pop(JobType &item);
        void clear();
        void inc_remaining(unsigned long long by = 1ull);
        void dec_remaining(unsigned long long by = 1ull);
        unsigned long long get_remaining();
    };

    // Parallel worker
    class Worker {
    private:
        JobList local_list;
        Scheduler& parent;
        unsigned long id;

        // Computes the Chi-squared test on the local queue, given the number of remaining jobs to be completed
        bool chi_squared_test();

    public:
        explicit Worker(Scheduler& parent, unsigned long id);
        bool get_job(JobType &job);
        void schedule(JobType&& job);

#ifdef DEBUG
        std::ofstream file;
        static std::chrono::time_point<std::chrono::high_resolution_clock> START;

        /*
         * This function logs on a (private) file the action of the worker as a CSV line with columns time, id, code,
         * info1, and info2, with the following meanings:
         *     - time: the time of the event from the beginning of the execution (in milliseconds);
         *     - id: the id of the worker;
         *     - code: a six char code of the event, that may be one of the following:
         *         - CREATE: the worker has been instantiated. info1 will contain the ID of the parent Scheduler class,
         *         while info2 the id of the worker;
         *         - RT_BGN: the worker started to retrieve a job;
         *         - RT_GLB: a job has been retrieved globally.
         *         - RT_LOC: a job has been retrieved locally.
         *         - NO_JOB: no job has been found;
         *         - SC_BGN: the worker started to schedule a job.
         *         - SC_GLB: the job has been scheduled globally;
         *         - SC_LOC: the job has been scheduled locally;
         *         - CHI_SK: the Chi squared test has been skipped (jobs below average). info1 will contain the number
         *         of job in the local queue and info1 the remaining jobs overall;
         *         - CHI_OK: the Chi squared test has been passed. info1 will contain the Chi squared value and info2
         *         its limit value;
         *         - CHI_NO: the Chi squared test has not been passed. info1 will contain the Chi squared value and
         *         info2 its limit value;
         *         - J_DONE: a job has been completed.
         *     - info1, info2: a value that depends on code. They might be empty.
         */
        template <typename T1, typename T2>
        void log(const char* code, T1 info1, T2 info2);
        void log_header();
#endif
    };

    SyncJobList global_list;
    std::vector<Worker> workers;
    unsigned long n_workers;
    float chi_limit;

#ifdef DEBUG
    static unsigned int ID;
    unsigned int id;
#endif
};

#endif //SPM_PROJECT_SCHEDULER_H
