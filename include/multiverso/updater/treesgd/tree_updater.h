#pragma once
//
// Copyright (c) Microsoft. All rights reserved.
//
// Licensed under custom Microsoft Research License Terms for
// Delayed Compensation Async Stochastic Gradient Descent.
// See LICENSE.md file in the project root for full license information.
//
// See https://arxiv.org/abs/1609.08326 for the details.
//
#ifndef MULTIVERSO_UPDATER_TREE_UPDATER_H_
#define MULTIVERSO_UPDATER_TREE_UPDATER_H_

#include "multiverso/updater/updater.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <string>

#include "multiverso/util/common.h"

namespace multiverso {
    template <typename T>
    class TreeUpdater : public Updater<T> {
    public:
        class VectorClock {
        public:
            explicit VectorClock(int n) :
                local_clock_(n, 0), global_clock_(0), size_(0) {}

            // Return true when all clock reach a same number
            virtual bool Update(int i) {
                ++local_clock_[i];
                if (global_clock_ < *(std::min_element(std::begin(local_clock_),
                    std::end(local_clock_)))) {
                    ++global_clock_;
                    if (global_clock_ == max_element()) {
                        return true;
                    }
                }
                return false;
            }

            virtual bool FinishTrain(int i) {
                local_clock_[i] = std::numeric_limits<int>::max();
                if (global_clock_ < min_element()) {
                    global_clock_ = min_element();
                    if (global_clock_ == max_element()) {
                        return true;
                    }
                }
                return false;
            }

            std::string DebugString() {
                std::string rank = "rank :" + std::to_string(MV_Rank());
                std::string os = rank + " global ";
                os += std::to_string(global_clock_) + " local: ";
                for (auto i : local_clock_) {
                    if (i == std::numeric_limits<int>::max()) os += "-1 ";
                    else os += std::to_string(i) + " ";
                }
                return os;
            }

            int local_clock(int i) const { return local_clock_[i]; }
            int global_clock() const { return global_clock_; }

            void set_local_clock(int worker, int clock) { local_clock_[worker] = clock; };

        private:
            int max_element() const {
                int max = global_clock_;
                for (auto val : local_clock_) {
                    max = (val != std::numeric_limits<int>::max() && val > max) ? val : max;
                }
                return max;
            }
            int min_element() const {
                return *std::min_element(std::begin(local_clock_), std::end(local_clock_));
            }
        protected:
            std::vector<int> local_clock_;
            int global_clock_;
            int size_;
        };

        explicit TreeUpdater(size_t size) :
            size_(size), e(1e-6) {
            Log::Info("[TREEUpdater] Init. \n");

            int num_worker = multiverso::MV_NumWorkers();
            mid_nodes_ = find_mid_node();
            int num_mid_node = mid_nodes_.size();
            worker_add_clocks_ = new std::unique_ptr<VectorClock>[num_mid_node];

            for (int node = 0; node < num_mid_node; ++node)
            {
                worker_add_clocks_[node].reset(new VectorClock(num_worker));
                for (int worker = 0; worker < leafnum_; ++worker)
                    worker_add_clocks_[node]->set_local_clock(worker, std::numeric_limits<int>::max());
                for (int idx = 0; idx < mid_nodes_[node].size(); ++idx)
                    worker_add_clocks_[node]->set_local_clock(mid_nodes_[node][idx], 0);
            }

            shadow_copies_.resize(MV_NumWorkers(), std::vector<T>(size_, 0));
            mean_square_.resize(MV_NumWorkers(), std::vector<T>(size_, 0));
            acc_gradients_.resize(size_, 0);
            midnode_copies_.resize(num_mid_node, std::vector<T>(size_, 0));
            midnode_meansquare_.resize(num_mid_node, std::vector<T>(size_, 0));
        }

        void Update(size_t num_element, T* data, T* delta,
            AddOption* option, size_t offset) override
        {
            int worker = option->worker_id();
            int x = 0;
            if (mixtreeps_[worker] > 0)
            {
                if (worker_add_clocks_[mixtreeps_[worker]]->Update(worker))
                {
                    for (size_t index = 0; index < num_element; ++index)
                    {
                        T g = delta[index] / option->learning_rate();
                        acc_gradients_[index + offset] += g;
                        acc_gradients_[index + offset] /= mid_nodes_[mixtreeps_[worker]].size();

                        /******************************ASGD*********************************/
                        //data[index + offset] -= option->learning_rate() * acc_gradients_[index + offset];

                        /******************************DC-ASGD-c*********************************/
                        //data[index + offset] -= option->learning_rate() *
                        //    (acc_gradients_[index + offset] + option->lambda() * 
                        //        acc_gradients_[index + offset] * acc_gradients_[index + offset] *
                        //    (data[index + offset] - midnode_copies_[mixtreeps_[worker]][index + offset]));

                        /******************************DC-ASGD-a*********************************/
                        midnode_meansquare_[mixtreeps_[worker]][index + offset] *= option->momentum();
                        midnode_meansquare_[mixtreeps_[worker]][index + offset] += (1 - option->momentum()) * 
                            acc_gradients_[index + offset] * acc_gradients_[index + offset];
                        data[index + offset] -= option->learning_rate() *
                            (acc_gradients_[index + offset] + option->lambda() / sqrt(midnode_meansquare_[mixtreeps_[worker]][index + offset] + e)*
                                acc_gradients_[index + offset] * acc_gradients_[index + offset] *
                                (data[index + offset] - midnode_copies_[mixtreeps_[worker]][index + offset]));

                        //caching each worker's latest version of parameter
                        acc_gradients_[index + offset] = 0;
                        midnode_copies_[mixtreeps_[worker]][index + offset] = data[index + offset];
                    }
                }
                else
                {
                    for (size_t index = 0; index < num_element; ++index)
                    {
                        T g = delta[index] / option->learning_rate();
                        acc_gradients_[index + offset] += g;
                    }
                }
            }
            else
            {
                for (size_t index = 0; index < num_element; ++index) {
                    T g = delta[index] / option->learning_rate();

                    /******************************ASGD*********************************/
                    //data[index + offset] -= option->learning_rate() * g;

                    /******************************DC-ASGD-c*********************************/
                    //data[index + offset] -= option->learning_rate() *
                    //    (g + option->lambda() *	g * g *
                    //    (data[index + offset] - shadow_copies_[option->worker_id()][index + offset]));


                    /******************************DC-ASGD-a*********************************/
                    mean_square_[option->worker_id()][index + offset] *= option->momentum();
                    mean_square_[option->worker_id()][index + offset] += (1 - option->momentum()) * g * g;
                    data[index + offset] -= option->learning_rate() *
                        (g + option->lambda() / sqrt(mean_square_[option->worker_id()][index + offset] + e)*
                            g * g *
                            (data[index + offset] - shadow_copies_[option->worker_id()][index + offset]));

                    ///******************************ASGD-dev*********************************/
                    //data[index + offset] -= option->learning_rate() *
                    //	(g + option->lambda() *	std::abs(g) * g * g);


                    //caching each worker's latest version of parameter
                    shadow_copies_[option->worker_id()][index + offset] = data[index + offset];
                }
            }
        }

        void Access(size_t num_element, T* data, T* blob_data,
            size_t offset, AddOption*) override {
            memcpy(blob_data, data + offset, sizeof(T) * num_element);
        }

        ~TreeUpdater() {
            shadow_copies_.clear();
            mean_square_.clear();
            midnode_meansquare_.clear();
            midnode_copies_.clear();
            acc_gradients_.clear();

            mid_nodes_.clear();
        }

    protected:
        std::vector< std::vector<T>> shadow_copies_;
        std::vector< std::vector<T>> mean_square_;
        std::vector<T> acc_gradients_;
        std::vector< std::vector<T>> midnode_meansquare_;
        std::vector< std::vector<T>> midnode_copies_;

        size_t size_;

        std::unique_ptr<VectorClock>* worker_add_clocks_;

        std::vector<std::vector<int>> mid_nodes_;

        float e;

    private:
        bool check_mixtree_valid()
        {
            std::vector<int> mid_node(leafnum_, 0);
            for (int worker : mixtreeps_)
                if (worker < 0 || worker >= leafnum_)
                    return false;
                else
                    mid_node[worker]++;
            bool zero_flag = false;
            for (int i = 1; i < leafnum_; ++i)
            {
                if (mid_node[i] == 0)
                    zero_flag = true;
                else if (mid_node[i] == 1)
                    return false;
                else if (zero_flag)
                    return false;
            }
            return true;
        }

        std::vector<std::vector<int>> find_mid_node()
        {
            std::vector<std::vector<int>> mid_node;
            for (int worker = 0; worker < leafnum_; ++worker)
            {
                if (mixtreeps_[worker] >(int)mid_node.size() - 1)
                {
                    int require = mixtreeps_[worker] - mid_node.size() + 1;
                    for (int i = 0; i < require; ++i)
                        mid_node.push_back(std::vector<int>());
                }
                mid_node[mixtreeps_[worker]].push_back(worker);
            }
            return mid_node;
        }
    };
}

#endif // MULTIVERSO_UPDATER_TREE_UPDATER_H_
