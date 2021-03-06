#include <algorithm>
#include <iostream>
#include <vector>

#include "autotune/autotune.hpp"
#include "autotune/parameter.hpp"
#include "autotune/tuners/parallel_line_search.hpp"

// // defines kernel, put in single compilation unit
// AUTOTUNE_KERNEL(int(int), add_one, "examples/kernel_minimal")

AUTOTUNE_KERNEL(int(int), smooth_cost_function,
                "examples/kernel_smooth_cost_function")

int main(void) {
  autotune::countable_set parameters;
  parameters.emplace_parameter<autotune::countable_continuous_parameter>(
      "PAR_1", 1.0, 1.0, 1.0, 5.0);

  parameters.emplace_parameter<autotune::countable_continuous_parameter>(
      "PAR_2", 1.0, 1.0, 1.0, 5.0);

  int a = 5;

  autotune::tuners::parallel_line_search tuner(autotune::smooth_cost_function,
                                               parameters, 10);
  tuner.set_verbose(true);
  tuner.set_write_measurement("parallel_line_search_demo");
  autotune::countable_set optimal_parameters = tuner.tune(a);
  // autotune::smooth_cost_function.set_parameter_values(optimal_parameters);
  std::cout << "optimal_parameters:" << std::endl;
  optimal_parameters.print_values();

  return 0;
}
