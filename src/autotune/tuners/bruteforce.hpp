#pragma once

#include "../parameter.hpp"

namespace autotune {

template <class F, typename... Args>
std::vector<size_t> bruteforce(F f, Args... args) {
  std::vector<tunable_parameter> &parameters = f->get_parameters();

  // brute-force tuner
  std::vector<std::string> values(parameters.size());
  for (size_t i = 0; i < parameters.size(); i++) {
    values[i] = parameters[i].get_value(0);
  }
  std::vector<size_t> indices(parameters.size(), 0);
  // evalute initial vector, always valid
  f->print_values(values);

  size_t current_index = 0;
  while (true) {
    // left the range of valid indices, done!
    if (current_index == parameters.size()) {
      break;
    }

    // the is another value for the current parameter
    if (indices[current_index] + 1 < parameters[current_index].size()) {
      values[current_index] =
          parameters[current_index].get_value(indices[current_index] + 1);
      indices[current_index] += 1;
      // reset the parameters "below" and start with the first parameter again
      for (size_t i = 0; i < current_index; i++) {
        values[i] = parameters[i].get_value(0);
        indices[i] = 0;
      }
      current_index = 0;

      // evalute new valid value vector
      f->print_values(values);

    } else {
      // no valid more values, try next parameter "above"
      current_index += 1;
    }
  }
  return indices;
}
}