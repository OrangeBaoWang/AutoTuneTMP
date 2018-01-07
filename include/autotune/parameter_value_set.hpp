#pragma once

#include <map>

#include "abstract_parameter.hpp"

namespace autotune {

using parameter_value_set = std::map<std::string, std::string>;

inline void print_parameter_values(parameter_value_set &parameter_values) {
  std::cout << "parameter name  | ";
  bool first = true;
  for (auto &p : parameter_values) {
    if (!first) {
      std::cout << ", ";
    } else {
      first = false;
    }
    std::cout << p.first;
    int64_t padding =
        std::max(p.first.size(), p.second.size()) - p.first.size();
    if (padding > 0) {
      std::stringstream ss;
      for (int64_t i = 0; i < padding; i++) {
        ss << " ";
      }
      std::cout << ss.str();
    }
  }
  std::cout << std::endl;

  std::cout << "parameter value | ";
  first = true;
  for (auto &p : parameter_values) {
    if (!first) {
      std::cout << ", ";
    } else {
      first = false;
    }
    std::cout << p.second;
    int64_t padding =
        std::max(p.first.size(), p.second.size()) - p.second.size();
    if (padding > 0) {
      std::stringstream ss;
      for (int64_t i = 0; i < padding; i++) {
        ss << " ";
      }
      std::cout << ss.str();
    }
  }
  std::cout << std::endl;
}
}
