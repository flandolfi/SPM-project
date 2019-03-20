//
// Created by flandolfi on 15/03/19.
//

#ifndef SPM_PROJECT_DAC_H
#define SPM_PROJECT_DAC_H

#include <functional>
#include <future>
#include <atomic>
#include "scheduler.h"


template <typename TypeIn, typename TypeOut>
class DAC {
private:
    using DivideFun = std::function<void(const TypeIn&, std::vector<TypeIn>&)>;
    using ConquerFun = std::function<void(std::vector<TypeOut>&, TypeOut&)>;
    using BaseTestFun = std::function<bool(const TypeIn&)>;
    using BaseCaseFun = std::function<void(const TypeIn&, TypeOut&)>;

    const DivideFun& divide;
    const ConquerFun& conquer;
    const BaseTestFun& base_test;
    const BaseCaseFun& base_case;

    Scheduler forks, joins;
    std::mutex mtx;

    void run(unsigned long id);
    void fork(const TypeIn &input, TypeOut &output, std::promise<void> &promise, long level, unsigned long id);
    void join(std::vector<std::promise<void>> *sub_promises, std::vector<TypeOut> *sub_results, TypeOut &output,
            unsigned long id);

public:
    DAC(const DivideFun& divide,
        const ConquerFun& conquer,
        const BaseTestFun& base_test,
        const BaseCaseFun& base_case);

    void compute(const TypeIn& input, TypeOut& output, unsigned long workers = 1,
                 Scheduler::Policy fork_policy = Scheduler::Policy::strong,
                 Scheduler::Policy join_policy = Scheduler::Policy::relaxed);
};



template<typename TypeIn, typename TypeOut>
DAC<TypeIn, TypeOut>::DAC(const DAC::DivideFun &divide, const DAC::ConquerFun &conquer,
                          const DAC::BaseTestFun &base_test, const DAC::BaseCaseFun &base_case)
                          : divide(divide), conquer(conquer), base_test(base_test),
                            base_case(base_case), forks(0), joins(0) {}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::compute(const TypeIn &input, TypeOut &output, unsigned long workers,
                                   Scheduler::Policy fork_policy, Scheduler::Policy join_policy) {
    std::unique_lock<std::mutex> lock(mtx);
    std::promise<void> promise;

    forks.reset(workers, fork_policy);
    joins.reset(workers, join_policy);

    forks.schedule([&](unsigned long id) {
        fork(input, output, promise, 0ul, id);
    }, 0ul, 0ul);

    std::vector<std::thread> threads;
    threads.reserve(workers - 1ul);

    for (auto id = 0ul; id < workers - 1ul; ++id)
        threads.emplace_back(&DAC::run, this, id);

    run(workers - 1ul);

    for (auto &thread: threads)
        thread.join();
}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::fork(const TypeIn &input, TypeOut &output, std::promise<void> &promise, long level,
        unsigned long id) {
    if (base_test(input)) {
        base_case(input, output);
        promise.set_value();
        forks.mark_done(id);

        return;
    }

    auto sub_problems = new std::vector<TypeIn>();
    divide(input, *sub_problems);
    auto size = sub_problems->size();
    auto sub_results = new std::vector<TypeOut>(size);
    auto sub_promises = new std::vector<std::promise<void>>(size);
    std::vector<Scheduler::JobType> sub_forks;
    sub_forks.reserve(size);

    for (auto i = 0ul; i < size; ++i) {
        sub_forks.emplace_back([=](unsigned long id){
            fork((*sub_problems)[i], (*sub_results)[i], (*sub_promises)[i], level + 1, id);
        });
    }

    joins.schedule(Scheduler::JobType([=, &output, &promise](unsigned long id) {
        join(sub_promises, sub_results, output, id);
        promise.set_value();
        sub_problems->clear();

        delete sub_problems;
    }), -level, id);

    auto continuation = std::move(sub_forks.back());
    sub_forks.pop_back();

    for (auto &&sf: sub_forks)
        forks.schedule(std::move(sf), level + 1ul, id);

    continuation(id);
}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::join(std::vector<std::promise<void>> *sub_promises, std::vector<TypeOut> *sub_results,
                                TypeOut &output, unsigned long id) {
    for (auto &sf: *sub_promises)
        sf.get_future().wait();

    conquer(*sub_results, output);
    joins.mark_done(id);
    sub_promises->clear();
    sub_results->clear();

    delete sub_promises;
    delete sub_results;
}

template<typename TypeIn, typename TypeOut>
void DAC<TypeIn, TypeOut>::run(unsigned long id) {
    Scheduler::JobType job;

    while (forks.get_job(job, id)) job(id);
    while (joins.get_job(job, id)) job(id);
}


#endif //SPM_PROJECT_DAC_H
