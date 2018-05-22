#include <algorithm>
#include <iostream>
#include <vector>

#include "autotune/autotune.hpp"
#include "autotune/parameter.hpp"
#include "autotune/tuners/parallel_line_search.hpp"

// defines kernel, put in single compilation unit
AUTOTUNE_KERNEL(int(int), add_one, "examples/kernel_minimal")

int main(void) {
  // register parameters
  autotune::add_one.set_verbose(true);
  // autotune::add_one.get_builder<cppjit::builder::gcc>().set_verbose(true);
  autotune::countable_set parameters;
  autotune::fixed_set_parameter<std::size_t> p1("ADD_ONE", {0, 1});
  parameters.add_parameter(p1);
  autotune::countable_continuous_parameter p2("CONT_PAR_DOUBLE", 2.0, 1.0, 1.0,
                                              5.0);
  parameters.add_parameter(p2);

  std::function<bool(int)> test_result = [](int) -> bool {
    /* tests values generated by kernel */
    return true;
  };

  int a = 5;

  autotune::tuners::parallel_line_search tuner(autotune::add_one, parameters, 2);
  tuner.setup_test(test_result);
  tuner.set_verbose(true);
  autotune::countable_set optimal_parameters = tuner.tune(a);
  autotune::add_one.set_parameter_values(optimal_parameters);
  return 0;
}
