#pragma once

#include "abstract_tuner.hpp"
#include "countable_set.hpp"

#include <random>

namespace autotune {
namespace tuners {

template <typename R, typename... Args>
class line_search : public abstract_tuner<countable_set, R, Args...> {
private:
  // autotune::abstract_kernel<R, cppjit::detail::pack<Args...>> &f;
  // countable_set &parameters;
  size_t max_iterations;
  size_t restarts;

  void evaluate_parameter_set(bool &first, countable_set &optimal_parameters,
                              double &optimal_duration, Args &... args) {
    // if a valid new index value was found, test it
    bool is_valid = true;
    double duration = this->evaluate(is_valid, args...);
    if (is_valid && (first || duration < optimal_duration)) {
      first = false;
      optimal_parameters = this->parameters;
      optimal_duration = duration;
      if (this->verbose) {
        this->report_verbose("new best kernel", optimal_duration,
                             this->parameters);
      }
    }
  }

  std::map<countable_set, bool> result_cache;

public:
  line_search(autotune::abstract_kernel<R, cppjit::detail::pack<Args...>> &f,
              countable_set &parameters, size_t max_iterations, size_t restarts)
      : abstract_tuner<countable_set, R, Args...>(f, parameters),
        max_iterations(max_iterations), restarts(restarts) {}

  countable_set tune(Args &... args) {

    parameter_value_set original_values = this->f.get_parameter_values();

    // set to initial guess (or whatever the parameter type is doing)
    for (size_t i = 0; i < this->parameters.size(); i++) {
      this->parameters[i]->set_min();
    }

    // evaluate initial guess
    countable_set optimal_parameters = this->parameters;

    double optimal_duration = -1.0;
    bool first = true;

    size_t counter = 0;
    size_t cur_index = 0;
    while (counter < max_iterations) {
      if (this->verbose) {
        std::cout << "current parameter index: " << cur_index << std::endl;
      }

      auto &p = this->parameters[cur_index];
      std::string old_value = p->get_value();
      p->set_initial();

      // if (first || p->get_value().compare(old_value) != 0) {
      if (first ||
          result_cache.find(optimal_parameters) != result_cache.end()) {
        result_cache[optimal_parameters] = true;
        evaluate_parameter_set(first, optimal_parameters, optimal_duration,
                               args...);
      }

      while (p->next()) {
        // if (first || p->get_value().compare(old_value) != 0) {
        if (first ||
            result_cache.find(optimal_parameters) != result_cache.end()) {
          result_cache[optimal_parameters] = true;
          evaluate_parameter_set(first, optimal_parameters, optimal_duration,
                                 args...);
        }
      }

      p->set_initial();
      // do not evaluate resetted value
      while (p->prev()) {
        // if (first || p->get_value().compare(old_value) != 0) {
        if (first ||
            result_cache.find(optimal_parameters) != result_cache.end()) {
          result_cache[optimal_parameters] = true;
          evaluate_parameter_set(first, optimal_parameters, optimal_duration,
                                 args...);
        }
      }
      cur_index = (cur_index + 1) % this->parameters.size();
      counter += 1;
      this->parameters = optimal_parameters;
    }

    this->f.set_parameter_values(original_values);

    result_cache.clear();

    return optimal_parameters;
  }
};

} // namespace tuners
} // namespace autotune
