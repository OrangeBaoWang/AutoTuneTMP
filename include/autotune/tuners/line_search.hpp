#pragma once

#include "abstract_tuner.hpp"
#include "autotune/tuners/evaluate.hpp"
#include "countable_set.hpp"

#include <random>

namespace autotune {
namespace tuners {

template <typename R, typename... Args>
class line_search : public abstract_tuner<countable_set, R, Args...> {
private:
  size_t max_iterations;
  int64_t last_updated;

public:
  line_search(autotune::abstract_kernel<R, cppjit::detail::pack<Args...>> &f,
              countable_set &parameters, size_t max_iterations)
      : abstract_tuner<countable_set, R, Args...>(f, parameters),
        max_iterations(max_iterations), last_updated(-1) {}

private:
  virtual void tune_impl(Args &... args) override {
    size_t counter = 0;
    size_t cur_index = 0;
    while (counter < max_iterations &&
           counter - last_updated < this->parameters.size()) {
      if (this->verbose) {
        std::cout << "current parameter index: " << cur_index << std::endl;
      }

      auto &p = this->parameters[cur_index];
      std::string old_value = p->get_value();
      p->set_initial();

      autotune::evaluate_t is_better = this->evaluate(args...);
      if (is_better == autotune::evaluate_t::better) {
        last_updated = counter;
      }

      while (p->next()) {
        autotune::evaluate_t is_better = this->evaluate(args...);
        if (is_better == autotune::evaluate_t::better) {
          last_updated = counter;
        }
      }

      p->set_initial();
      // do not evaluate resetted value
      while (p->prev()) {
        autotune::evaluate_t is_better = this->evaluate(args...);
        if (is_better == autotune::evaluate_t::better) {
          last_updated = counter;
        }
      }
      cur_index = (cur_index + 1) % this->parameters.size();
      counter += 1;
      this->parameters = this->optimal_parameters;
    }
  }

  void reset_impl() override { last_updated = -1; }
};

} // namespace tuners
} // namespace autotune
