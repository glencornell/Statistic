#pragma once
//
//    FILE: Statistic.h
//  AUTHOR: Rob Tillaart
//          modified at 0.3 by Gil Ross at physics dot org
// VERSION: 0.4.4
// PURPOSE: Recursive Statistical library for Arduino
// HISTORY: See CHANGELOG.md
//
// NOTE: 2011-01-07 Gill Ross
// Rob Tillaart's Statistic library uses one-pass of the data (allowing
// each value to be discarded), but expands the Sum of Squares Differences to
// difference the Sum of Squares and the Average Squared. This is susceptible
// to bit length precision errors with the float type (only 5 or 6 digits
// absolute precision) so for long runs and high ratios of
// the average value to standard deviation the estimate of the
// standard error (deviation) becomes the difference of two large
// numbers and will tend to zero.
//
// For small numbers of iterations and small Average/SE the original code is
// likely to work fine.
// It should also be recognised that for very large samples, questions
// of stability of the sample assume greater importance than the
// correctness of the asymptotic estimators.
//
// This recursive algorithm, which takes slightly more computation per
// iteration is numerically stable.
// It updates the number, mean, max, min and SumOfSquaresDiff each step to
// deliver max min average, population standard error (standard deviation) and
// unbiased SE.

#include <limits>
#include <type_traits>
#include <cstdint>
#include <cmath>


#define STATISTIC_LIB_VERSION                     (F("0.4.4"))

namespace stat {

template <typename T = float, typename C = uint32_t, bool _useStdDev = true>
class Statistic
{
public:
  static_assert (std::is_floating_point<T>::value, "Statistic<T,C>: T must be a floating point type (float, double, etc)");
  static_assert (std::is_unsigned<C>::value, "Statistic<T,C>: C must be an unsigned type");

  typedef T value_type;
  typedef C count_type;
  static constexpr value_type NaN { std::numeric_limits<value_type>::quiet_NaN() };

  Statistic() = default;

  void clear() {
    _cnt = 0;
    _sum = 0;
    _min = 0;
    _max = 0;
    _extra.clear();
    // note not _ssq but sum of square differences
    // which is SUM(from i = 1 to N) of f(i)-_ave_N)**2
  }

  // returns value actually added
  value_type add(const value_type value) {
    value_type previousSum = _sum;
    if (_cnt == 0)
      {
        _min = value;
        _max = value;
      } else {
      if (value < _min) _min = value;
      else if (value > _max) _max = value;
    }
    _sum += value;
    _cnt++;

    if (_useStdDev && (_cnt > 1))
      {
        value_type _store = (_sum / _cnt - value);
        _extra.ssqdif(_extra.ssqdif() + _cnt * _store * _store / (_cnt - 1));

        // ~10% faster but limits the amount of samples to 65K as _cnt*_cnt overflows
        // value_type _store = _sum - _cnt * value;
        // _ssqdif = _ssqdif + _store * _store / (_cnt*_cnt - _cnt);
        //
        // solution:  TODO verify
        // _ssqdif = _ssqdif + (_store * _store / _cnt) / (_cnt - 1);
      }
    return _sum - previousSum;
  }

  // returns the number of values added
  count_type count() const   { return _cnt; };   // zero if count == zero
  value_type sum() const     { return _sum; };   // zero if count == zero
  value_type minimum() const { return _min; };   // zero if count == zero
  value_type maximum() const { return _max; };   // zero if count == zero

  // NAN if count == zero
  value_type average() const {
    if (_cnt == 0) return NaN; // prevent DIV0 error
    return _sum / _cnt;
  }

  // useStdDev must be true to use next three
  // all return NAN if count == zero
  value_type variance() const {
    if (!_useStdDev) return NaN;
    if (_cnt == 0) return NaN; // prevent DIV0 error
    return _extra.ssqdif() / _cnt;
  }

  // Population standard deviation
  value_type pop_stdev() const {
    if (!_useStdDev) return NaN;
    if (_cnt == 0) return NaN; // prevent DIV0 error
    return std::sqrt( _extra.ssqdif() / _cnt);
  }
  value_type unbiased_stdev() const {
    if (!_useStdDev) return NaN;
    if (_cnt < 2) return NaN; // prevent DIV0 error
    return std::sqrt( _extra.ssqdif() / (_cnt - 1));
  }

  // deprecated methods:
  Statistic(bool) {
  } __attribute__ ((deprecated ("use default constructor instead")));
  void clear(bool) {
    clear();
  } __attribute__ ((deprecated ("use Statistic::clear(void) instead")));

protected:
  count_type _cnt { 0 };
  value_type _sum { 0.0 };
  value_type _min { 0.0 };
  value_type _max { 0.0 };
  // Condtionally compile to reduce dead code if not used
  struct Empty {
    void clear() { }
    value_type ssqdif() const { return NaN; }
    value_type ssqdif(value_type v) { }
  };
  struct StdDev {
    value_type    _ssqdif { 0.0 };    // sum of squares difference
    void clear() { _ssqdif = 0.0; }
    value_type ssqdif() const { return _ssqdif; }
    value_type ssqdif(value_type v) { _ssqdif = v; }
  };
  typename std::conditional<_useStdDev, StdDev, Empty>::type _extra;
};

} // namespace stat

// This typedef maintains backwards API compatibility with library
// versions <= 0.4.4.
typedef stat::Statistic<float, uint32_t, true> Statistic;

// NOTE:
// Do not issue 'using stat;' in your code because the compiler will
// not be able to distinguish between '::Statistic' and
// 'stat::Statistic'

// -- END OF FILE --
