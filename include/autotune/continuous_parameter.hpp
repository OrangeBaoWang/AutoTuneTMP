#pragma once

#include <iomanip>

namespace autotune {

class stepper {
private:
  double step;

public:
  stepper(double step) : step(step) {}

  double operator()(double current) { return current + step; }
};

class continuous_parameter : public step_parameter {
private:
  double current;
  double initial;
  // modifies the current value to a next value
  // this depends on the parameter, could be adding a increment, double
  // something more complex
  std::function<double(double)> next_function;
  // modifies the current to a previous value (opposite to next)
  std::function<double(double)> prev_function;
  // validates values, used by the next and prev functions
  // can be used to implement lower and upper bounds
  std::function<bool(double)> valid_function;

public:
  continuous_parameter(const std::string &name, double initial)
      : step_parameter(name), current(initial), initial(initial) {}

  virtual const std::string get_value() const override {
    return std::to_string(current);
  }

  void set_next_function(std::function<double(double)> next_function) {
    this->next_function = next_function;
  }

  virtual bool next() override {
    double temp = this->next_function(current);
    if (valid_function) {
      if (valid_function(temp)) {
        current = temp;
        return true;
      } else {
        return false;
      }
    } else {
      current = temp;
      return true;
    }
  }

  void set_prev_function(std::function<double(double)> prev_function) {
    this->prev_function = prev_function;
  }

  virtual bool prev() override {
    double temp = this->prev_function(current);
    if (valid_function) {
      if (valid_function(temp)) {
        current = temp;
        return true;
      } else {
        return false;
      }
    } else {
      current = temp;
      return true;
    }
  }

  void set_valid_function(std::function<double(double)> valid_function) {
    this->valid_function = valid_function;
  }

  // virtual size_t count_values() const override { throw; }

  virtual void reset() override {
    // TODO: should be extended, so that an initial guess can be supplied
    current = initial;
  }

  virtual std::string to_parameter_source_line() override {
    return "#define " + name + " " + std::to_string(current) + "\n";
  }

  virtual std::shared_ptr<abstract_parameter> clone() override {
    std::shared_ptr<continuous_parameter> new_instance =
        std::make_shared<continuous_parameter>(this->name, initial);
    new_instance->current = this->current;
    return std::dynamic_pointer_cast<abstract_parameter>(new_instance);
  };
};

namespace factory {

std::shared_ptr<continuous_parameter>
make_continuous_parameter(const std::string &name, double initial, double min,
                          double max, double step) {
  std::shared_ptr<continuous_parameter> p =
      std::make_shared<continuous_parameter>(name, initial);
  p->set_next_function(stepper(step));
  p->set_prev_function(stepper(-1.0 * step));
  p->set_valid_function([=](double value) {
    if (value >= min && value <= max) {
      return true;
    } else {
      return false;
    }
  });

  return p;
}

std::shared_ptr<continuous_parameter>
make_continuous_parameter(const std::string &name, double initial,
                          double step) {
  std::shared_ptr<continuous_parameter> p =
      std::make_shared<continuous_parameter>(name, initial);
  p->set_next_function(stepper(step));
  p->set_prev_function(stepper(-1.0 * step));

  return p;
}

std::shared_ptr<continuous_parameter>
make_continuous_parameter(const std::string &name, double initial) {
  std::shared_ptr<continuous_parameter> p =
      std::make_shared<continuous_parameter>(name, initial);
  p->set_next_function(stepper(1.0));
  p->set_prev_function(stepper(-1.0));

  return p;
}

std::shared_ptr<continuous_parameter>
make_continuous_parameter(const std::string &name, double initial, double min,
                          double max) {
  std::shared_ptr<continuous_parameter> p =
      std::make_shared<continuous_parameter>(name, initial);
  p->set_next_function(stepper(1.0));
  p->set_prev_function(stepper(-1.0));
  p->set_valid_function([=](double value) {
    if (value >= min && value <= max) {
      return true;
    } else {
      return false;
    }
  });

  return p;
}

} // namespace factory
} // namespace autotune