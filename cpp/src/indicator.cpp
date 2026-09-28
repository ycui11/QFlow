#include "qflow/indicator.hpp"

namespace qflow {

Sma::Sma(std::size_t period) : period_(period == 0 ? 1 : period) {
  values_.reserve(period_);
}

void Sma::reset() {
  values_.clear();
  sum_ = 0.0;
  value_ = 0.0;
}

void Sma::update(double price) {
  values_.push_back(price);
  sum_ += price;
  if (values_.size() > period_) {
    sum_ -= values_.front();
    values_.erase(values_.begin());
  }
  if (ready()) {
    value_ = sum_ / static_cast<double>(period_);
  }
}

Ema::Ema(std::size_t period) {
  const std::size_t p = period == 0 ? 1 : period;
  alpha_ = 2.0 / (static_cast<double>(p) + 1.0);
}

void Ema::reset() {
  value_ = 0.0;
  primed_ = false;
}

void Ema::update(double price) {
  if (!primed_) {
    value_ = price;
    primed_ = true;
    return;
  }
  value_ = alpha_ * price + (1.0 - alpha_) * value_;
}

EmaCrossSignal::EmaCrossSignal(std::size_t fast_period, std::size_t slow_period)
    : fast_(fast_period), slow_(slow_period) {}

void EmaCrossSignal::reset() {
  fast_.reset();
  slow_.reset();
  have_prev_ = false;
  crossed_up_ = false;
  crossed_down_ = false;
}

void EmaCrossSignal::update(double price) {
  crossed_up_ = false;
  crossed_down_ = false;
  fast_.update(price);
  slow_.update(price);
  if (!ready()) {
    return;
  }
  const double f = fast_.value();
  const double s = slow_.value();
  if (have_prev_) {
    if (prev_fast_ <= prev_slow_ && f > s) {
      crossed_up_ = true;
    }
    if (prev_fast_ >= prev_slow_ && f < s) {
      crossed_down_ = true;
    }
  }
  prev_fast_ = f;
  prev_slow_ = s;
  have_prev_ = true;
}

}  // namespace qflow
