/**
 * @file dac.h
 * @brief Contains the DAC class header and implementation.
 *
 * @author Francesco Landolfi
 */

#ifndef SPM_PROJECT_DAC_H
#define SPM_PROJECT_DAC_H

#include <functional>
#include <future>
#include <atomic>
#include "scheduler.h"

/**
 * @class DAC
 * @brief Framework for parallel Divide and Conquer computation.
 *
 * @tparam TypeIn the type of the input (to be divided)
 * @tparam TypeOut the type of the output (to be conquered)
 */
template<typename TypeIn, typename TypeOut>
class DAC {
private:
    using DivideFun = std::function<void(const TypeIn &, std::vector<TypeIn> &)>;
    using ConquerFun = std::function<void(std::vector<TypeOut> &, TypeOut &)>;
    using BaseTestFun = std::function<bool(const TypeIn &)>;
    using BaseCaseFun = std::function<void(const TypeIn &, TypeOut &)>;

    const DivideFun &divide;
    const ConquerFun &conquer;
    const BaseTestFun &base_test;
    const BaseCaseFun &base_case;

    Scheduler forks, joins;
    std::mutex mtx;

    void run(unsigned long id);
    void fork(const TypeIn &input, std::promise<TypeOut> &promise, unsigned long id);
    void join(std::vector<std::promise<TypeOut>> *sub_promises, std::promise<TypeOut> &promise);

public:
    /**
     * Creates a DAC instance.
     *
     * @param divide the divide function.
     * @param conquer the conquer function.
     * @param base_test the test function. It should return true if the input belongs to the base case, false otherwise.
     * @param base_case the base case function.
     */
    DAC(const DivideFun &divide, const ConquerFun &conquer,
        const BaseTestFun &base_test, const BaseCaseFun &base_case);

    /**
     * Computes the solution for @p input and stores the result in @p output, using the functions passed to the
     * constructor.
     *
     * @param input the input to be processed
     * @param output the computed result
     * @param workers the number of threads to use to compute the solution (i.e., the parallelism degree)
     * @param fork_policy the balancing policy to use in the scheduler that manages the "fork" tasks
     *     (@see Scheduler::Policy)
     */
    void compute(const TypeIn &input, TypeOut &output, unsigned long workers = 1,
                 Scheduler::Policy policy = Scheduler::Policy::best);
};


template<typename TypeIn, typename TypeOut>
DAC<TypeIn, TypeOut>::DAC(const DAC::DivideFun &divide, const DAC::ConquerFun &conquer,
                          const DAC::BaseTestFun &base_test, const DAC::BaseCaseFun &base_case)
        : divide(divide), conquer(conquer), base_test(base_test),
          base_case(base_case), forks(0), joins(0) {}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::compute(const TypeIn &input, TypeOut &output, unsigned long workers,
                                   Scheduler::Policy policy) {
    std::unique_lock<std::mutex> lock(mtx);
    std::promise<TypeOut> promise;

    forks.reset(workers, policy);
    joins.reset(workers, Scheduler::Policy::only_local);

    forks.schedule([&](unsigned long id) {
        fork(input, promise, id);
    }, 0ul);

    std::vector<std::thread> threads;

    for (auto id = 0ul; id < workers - 1ul; ++id)
        threads.emplace_back(&DAC::run, this, id);

    run(workers - 1ul);

    for (auto &thread: threads)
        thread.join();

    output = std::move(promise.get_future().get());
}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::fork(const TypeIn &input, std::promise<TypeOut> &promise, unsigned long id) {
    if (base_test(input)) {
        TypeOut output;
        base_case(input, output);
        promise.set_value(std::move(output));

        return;
    }

    auto sub_problems = new std::vector<TypeIn>();
    divide(input, *sub_problems);
    auto size = sub_problems->size();
    auto sub_promises = new std::vector<std::promise<TypeOut>>(size);
    std::vector<Scheduler::JobType> sub_forks;
    sub_forks.reserve(size);

    for (auto i = 0ul; i < size; ++i) {
        sub_forks.emplace_back([=](unsigned long id) {
            fork((*sub_problems)[i], (*sub_promises)[i], id);
        });
    }

    joins.schedule(Scheduler::JobType([=, &promise](unsigned long ignored) {
        join(sub_promises, promise);

        delete sub_problems;
    }), id);

    auto continuation = std::move(sub_forks.back());
    sub_forks.pop_back();

    for (auto &&sf: sub_forks)
        forks.schedule(std::move(sf), id);

    continuation(id);
}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::join(std::vector<std::promise<TypeOut>> *sub_promises, std::promise<TypeOut> &promise) {
    std::vector<TypeOut> results;
    std::transform(sub_promises->begin(), sub_promises->end(), std::back_inserter(results),
                   [](std::promise<TypeOut> &p) { return p.get_future().get(); });

    TypeOut output;
    conquer(results, output);
    promise.set_value(std::move(output));

    delete sub_promises;
}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::run(unsigned long id) {
    while (forks.compute_next(id));
    while (joins.compute_next(id));
}


#endif //SPM_PROJECT_DAC_H
