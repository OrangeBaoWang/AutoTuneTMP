#pragma once

#include "../autotune.hpp"
#include "../parameter.hpp"
#include "abstract_tuner.hpp"
#include "countable_set.hpp"

namespace autotune {
namespace tuners {

template <typename R, typename... Args>
class full_neighborhood_search
    : public abstract_tuner<countable_set, R, Args...> {
private:
  size_t iterations;

public:
  full_neighborhood_search(
      autotune::cppjit_kernel<R, cppjit::detail::pack<Args...>> &f,
      countable_set &parameters, size_t iterations)
      : abstract_tuner<countable_set, R, Args...>(f, parameters),
        iterations(iterations) {}

private:
  void tune_impl(Args &... args) override {
    for (size_t i = 0; i < iterations; i++) {
      if (this->verbose) {
        std::cout << "--------- next iteration: " << i << " ---------"
                  << std::endl;
      }
      this->parameters.print_values();

      // initialize to first parameter combination and evaluate it
      size_t cur_index = 0;
      std::vector<int64_t> cur_offsets(this->parameters.size());

      for (size_t m = 0; m < this->parameters.size(); m++) {
        if (this->parameters[m]->prev()) {
          cur_offsets[m] = -1;
        } else {
          cur_offsets[m] = 0;
        }
      }

      this->evaluate(args...);

      while (true) {
        if (cur_index == this->parameters.size()) {
          break;
        }

        // next() increments parameter and checks whether still within bounds
        if (cur_offsets[cur_index] < 1 && this->parameters[cur_index]->next()) {
          cur_offsets[cur_index] += 1;

          for (size_t k = 0; k < cur_index; k++) {
            // resets parameters
            // prev() won't do anything if result would be out of bounds
            if (this->parameters[k]->prev()) {
              cur_offsets[k] -= 1;
            }
            // if was at 1, now at 0, attempt -1
            if (cur_offsets[k] == 0) {
              if (this->parameters[k]->prev()) {
                cur_offsets[k] -= 1;
              }
            }
          }
          cur_index = 0;

          this->evaluate(args...);
        } else {
          cur_index += 1;
        }
      }

      this->parameters = this->optimal_parameters;
    }
  }

  void reset_impl() override {}
};
} // namespace tuners
} // namespace autotune
