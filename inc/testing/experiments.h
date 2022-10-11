#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "cli-parse.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr.h"
#include "result.h"
#include "pebbling-model.h"
#include "peterson.h"

#include <variant>

namespace pdr::pebbling::experiments
{
  enum output_format
  {
    string,
    latex,
    markdown
  };
  struct Run
  {
    using Row_t   = tabulate::Table::Row_t;
    using Table_t = std::array<Row_t, 7>;
    std::string_view model;
    Tactic tactic;

    double avg_time;
    double std_dev_time;
    std::optional<PebblingResult::PebblingInvariant> min_inv;
    std::optional<PebblingResult::PebblingTrace> min_strat;

    Run(const my::cli::ArgumentList& args,
        const std::vector<pebbling::PebblingResult>& r);
    std::string str(output_format fmt) const;
    std::string str_compared(const Run& other, output_format fmt) const;

   private:
    // latex export
    Table_t listing() const;
    Table_t combined_listing(const Run& other) const;
  };

  void pebbling_run(pebbling::PebblingModel& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);
} // namespace pdr::pebbling::experiments

#endif // EXPERIMENTS_H
