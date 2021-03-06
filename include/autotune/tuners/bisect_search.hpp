#pragma once

#include "../autotune.hpp"
#include "../parameter.hpp"
#include "abstract_tuner.hpp"
#include "limited_set.hpp"

namespace autotune {
namespace tuners {

template <typename R, typename... Args>
class bisect_search : public abstract_tuner<limited_set, R, Args...> {
private:
  size_t iterations;

public:
  bisect_search(autotune::abstract_kernel<R, cppjit::detail::pack<Args...>> &f,
                limited_set &parameters, size_t iterations)
      : abstract_tuner<limited_set, R, Args...>(f, parameters),
        iterations(iterations) {}

private:
  void tune_impl(Args &... args) override {

    // this->result_cache.clear();

    // parameter_value_set original_values = this->f.get_parameter_values();

    // limited_set optimal_parameters = this->parameters;

    std::vector<double> min(this->parameters.size());
    std::vector<double> max(this->parameters.size());
    std::vector<double> mid(this->parameters.size());
    std::vector<std::pair<double, bool>> min_eval(this->parameters.size());
    std::vector<std::pair<double, bool>> max_eval(this->parameters.size());
    std::vector<std::pair<double, bool>> mid_eval(this->parameters.size());

    for (size_t p_idx = 0; p_idx < this->parameters.size(); p_idx++) {
      auto &p = this->parameters[p_idx];
      min[p_idx] = p->get_min();
      mid[p_idx] = p->get_raw_value();
      max[p_idx] = p->get_max();
      min_eval[p_idx].second = false;
      max_eval[p_idx].second = false;
      mid_eval[p_idx].second = false;
    }

    for (size_t i = 0; i < iterations; i++) {
      for (size_t p_idx = 0; p_idx < this->parameters.size(); p_idx++) {
        std::cout << min[p_idx] << " " << mid[p_idx] << " " << max[p_idx]
                  << std::endl;
        auto &p = this->parameters[p_idx];
        if (!mid_eval[p_idx].second) {
          p->set_value(mid[p_idx]);
          // mid_eval[p_idx].first =
          this->evaluate(args...);
        }
        if (!min_eval[p_idx].second) {
          p->set_value(0.5 * (min[p_idx] + mid[p_idx]));
          // min_eval[p_idx].first =
          this->evaluate(args...);
        }
        if (!max_eval[p_idx].second) {
          p->set_value(0.5 * (max[p_idx] + mid[p_idx]));
          // max_eval[p_idx].first =
          this->evaluate(args...);
        }
        double opt_val = 0.5 * (min[p_idx] + mid[p_idx]);
        std::pair<double, bool> opt = min_eval[p_idx];
        if (!opt.second ||
            (mid_eval[p_idx].second && mid_eval[p_idx].first < opt.first)) {
          opt = mid_eval[p_idx];
          opt_val = mid[p_idx];
        }
        if (!opt.second ||
            (max_eval[p_idx].second && max_eval[p_idx].first < opt.first)) {
          opt = max_eval[p_idx];
          opt_val = 0.5 * (max[p_idx] + mid[p_idx]);
        }
        if (opt.second) {
          if (opt_val < mid[p_idx]) {
            max[p_idx] = mid[p_idx];
            max_eval = mid_eval;
            mid[p_idx] = opt_val;
            mid_eval[p_idx] = opt;
            min_eval[p_idx].second = false;
          } else if (opt_val > mid[p_idx]) {
            min[p_idx] = mid[p_idx];
            min_eval = mid_eval;
            mid[p_idx] = opt_val;
            mid_eval[p_idx] = opt;
            max_eval[p_idx].second = false;
          } else {
            min[p_idx] = 0.5 * (min[p_idx] + mid[p_idx]);
            max[p_idx] = 0.5 * (max[p_idx] + mid[p_idx]);
            min_eval[p_idx].second = false;
            max_eval[p_idx].second = false;
          }
          p->set_value(mid[p_idx]);
          // optimal_parameters = this->parameters;
        }
        p->set_value(mid[p_idx]);
      }
    }

    // this->f.set_parameter_values(original_values);
    // return optimal_parameters;
  }

  void reset_impl() override {}
};
} // namespace tuners
} // namespace autotune
