#pragma once

#include "../autotune.hpp"
#include "../parameter.hpp"
#include "common.hpp"

namespace autotune {

template <class... T> class kernel;

namespace tuners {

template <class... Args> class bruteforce;

template <typename R, typename... Args>
class bruteforce<autotune::kernel<R, cppjit::detail::pack<Args...>>>
    : public abstract_tuner<R, Args...> {
private:
  autotune::kernel<R, cppjit::detail::pack<Args...>> &f;
  bool verbose;

public:
  bruteforce(autotune::kernel<R, cppjit::detail::pack<Args...>> &f) : f(f) {}

  parameter_set tune(Args &... args) {
    parameter_set &parameters = f.get_parameters();
    bool is_valid = true;

    f.write_header();

    double total_combinations = 1.0;
    for (size_t i = 0; i < parameters.size(); i++) {
      total_combinations *=
          parameters.get_as<fixed_set_parameter>(i)->get_values().size();
    }

    if (verbose) {
      std::cout << "total combinations to test: " << total_combinations
                << std::endl;
    }

    parameter_set original_parameters = parameters.clone();

    // brute-force tuner
    for (size_t i = 0; i < parameters.size(); i++) {
      parameters.get_as<fixed_set_parameter>(i)->set_index(0);
    }

    // evaluate initial vector, always valid
    // f.print_values(values);
    size_t combination_counter = 1;
    if (verbose) {
      std::cout << "evaluating combination " << combination_counter
                << " (out of " << total_combinations << ")" << std::endl;
      std::cout << "current attempt:" << std::endl;
      f.print_values();
    }
    combination_counter += 1;
    bool first = true;

    double optimal_duration = this->evaluate(is_valid, f, args...);
    parameter_set optimal_parameters;
    if (is_valid) {
      first = false;
      optimal_parameters = parameters.clone();
      this->report_verbose("new best kernel", optimal_duration, f);
    }

    size_t current_index = 0;
    while (true) {
      // left the range of valid indices, done!
      if (current_index == parameters.size()) {
        break;
      }

      // the is another value for the current parameter
      if (parameters.get_as<fixed_set_parameter>(current_index)->next()) {
        // reset the parameters "below" and start with the first parameter
        // again
        for (size_t i = 0; i < current_index; i++) {
          parameters.get_as<fixed_set_parameter>(i)->set_index(0);
        }
        current_index = 0;

        // evaluate new valid value vector
        if (verbose) {
          std::cout << "evaluating combination " << combination_counter
                    << " (out of " << total_combinations << ")" << std::endl;
          std::cout << "current attempt:" << std::endl;
          f.print_values();
        }
        combination_counter += 1;
        double duration = this->evaluate(is_valid, f, args...);
        if (is_valid && (first || duration < optimal_duration)) {
          first = false;
          optimal_duration = duration;
          optimal_parameters = parameters.clone();
          this->report_verbose("new best kernel", optimal_duration, f);
        }

      } else {
        // no valid more values, try next parameter "above"
        current_index += 1;
      }
    }

    f.set_parameters(original_parameters);
    return optimal_parameters;
  }

  void set_verbose(bool verbose) { this->verbose = verbose; }
};
} // namespace tuners
} // namespace autotune
