#ifndef PETER_RESULT_H
#define PETER_RESULT_H

#include "peterson.h"
#include "result.h"

#include <optional>
#include <ostream>
#include <tabulate/tabulate.hpp>

namespace pdr::peterson
{
  namespace result
  {
  } // namespace result

  // processes | max_processes | invariant | trace | time
  class PetersonResult final : public IpdrResult
  {
   public:
    inline static const tabulate::Table::Row_t peterson_summary_header = {
      "processes", "max_processes", "invariant index", "trace_length", "time"
    };
    inline static const tabulate::Table::Row_t peterson_total_header = {
      "runtime", "proven for p=", "maximum p"
    };

    PetersonResult(const PetersonModel& m, Tactic t);

    double get_total_time() const;
    bool all_holds() const;
    std::string end_result() const override;
    tabulate::Table::Row_t total_row() const override;

   private:
    const PetersonModel& model;
    const Tactic tactic;
    bool holds{ true };
    unsigned last_proof_procs{ 0 };

    const tabulate::Table::Row_t summary_header() const override;
    const tabulate::Table::Row_t total_header() const override;
    const tabulate::Table::Row_t process_row(const PdrResult& r) override;
    std::string process_trace(const PdrResult& res) const override;
  }; // class PeterState
} // namespace pdr::peterson

#endif // PETER_RESULT_H
