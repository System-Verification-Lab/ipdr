#ifndef PEBBLING_RESULT_H
#define PEBBLING_RESULT_H

#include "pebbling-model.h"
#include "result.h"

namespace pdr::pebbling
{
  namespace result
  {
    static tabulate::Table::Row_t result_header = { "constraint", "pebbled",
      "invariant index", "trace length", "time" };

    static tabulate::Table::Row_t summary_header = { "runtime",
      "max constraint with invariant", "level", "min constraint with strategy",
      "length" };
  } // namespace result

  // aggregates multiple pdr runs into a single ipdr result for pebbling
  // collects: total time spent, highest level invariant, and trace with the
  // lowest marking
  class PebblingResult final : public IpdrResult
  {
   public:
    struct PebblingInvariant
    {
      PdrResult::Invariant invariant;
      std::optional<unsigned> constraint;
    };
    struct PebblingTrace
    {
      PdrResult::Trace trace;
      unsigned pebbled{ 0 };
    };
    struct Data_t
    {
      double const& time; // refers to IpdrResult::total_time
      std::optional<PebblingInvariant> inv;
      std::optional<PebblingTrace> strategy;
    };

    PebblingResult(const PebblingModel& m, Tactic t);
    PebblingResult(const IpdrResult& r, const PebblingModel& m, Tactic t);

    void add_summary_to(tabulate::Table& t) const override;
    void show_raw(std::ostream& out) const;

    void show(std::ostream& out) const override;

    Data_t const& get_total() const;
    const std::optional<unsigned> min_pebbles() const;

   private:
    PebblingModel const& model;
    const Tactic tactic;
    Data_t total; // the latest invariant and trace, with the total time spent
    unsigned n_invariants{ 0 };
    unsigned n_traces{ 0 };

    const tabulate::Table::Row_t header() const override;
    // expand row with constraint and length, and store the latest in total
    const tabulate::Table::Row_t table_row(const PdrResult& r) override;
    std::string process_trace(const PdrResult& res) const override;
  };
} // namespace pdr::pebbling

#endif // PEBBLING_RESULT_H
