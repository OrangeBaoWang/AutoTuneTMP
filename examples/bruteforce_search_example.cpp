#include <algorithm>
#include <iostream>
#include <vector>

#include "autotune/autotune.hpp"
#include "autotune/parameter.hpp"
#include "autotune/tuners/bruteforce.hpp"

// defines kernel, put in single compilation unit
AUTOTUNE_DECLARE_DEFINE_KERNEL(int(int), add_one)

int main(void) {
  autotune::add_one.set_verbose(false);
  autotune::add_one.set_source_dir("examples/kernel_minimal");

  // register parameters
  autotune::countable_set parameters;
  autotune::fixed_set_parameter p1("ADD_ONE", {"0", "1"});
  parameters.add_parameter(p1);
  autotune::countable_continuous_parameter p2("CONT_PAR_DOUBLE", 2.0, 1.0, 1.0,
                                              5.0);
  parameters.add_parameter(p2);

  std::function<bool(int)> test_result = [](int) -> bool {
    /* tests values generated by kernel */
    return true;
  };

  int a = 5;

  autotune::tuners::bruteforce tuner(
      autotune::add_one, parameters);
  tuner.setup_test(test_result);
  tuner.set_verbose(true);
  autotune::countable_set optimal_parameters = tuner.tune(a);
  autotune::add_one.set_parameter_values(optimal_parameters);
  return 0;
}
