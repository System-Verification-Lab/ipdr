#ifndef Z3PDR_H
#define Z3PDR_H

#include "dag.h"
#include "expr.h"
#include "logger.h"
#include "pdr-context.h"
#include "pebbling-result.h"
#include "result.h"
#include "vpdr.h"
#include "z3-pebbling-model.h"

#include <tabulate/table.hpp>
#include <z3++.h>

namespace pdr::test
{
  class z3PebblingIPDR;

  class z3PDR : public vPDR
  {
    friend class z3PebblingIPDR;

   public:
    z3PDR(Context c, Logger& l, Z3Model& m);

    PdrResult run() override;
    void reset() override;
    void show_solver(std::ostream& out) const override;

   private:
    struct Rule
    {
      z3::expr expr;
      z3::symbol name;

      Rule(z3::context& ctx) : expr(ctx), name(ctx.str_symbol("empty str")) {}
      Rule(z3::expr const& e, z3::symbol const& n) : expr(e), name(n) {}
    };

    Z3Model& ts;
    z3::check_result last_result = z3::check_result::unknown;
    std::string cover_string{ "" };

    std::vector<std::string> get_trace(z3::fixedpoint& engine);
    PdrResult::Trace::TraceVec get_trace_states(z3::fixedpoint& engine);

    z3::fixedpoint mk_prepare_fixedpoint();
  };

  // class to verify against z3's pdr implementation
  // runs pdr multiple times, but without incremental optimizations
  class z3PebblingIPDR
  {
   public:
    z3PebblingIPDR(my::cli::ArgumentList const& args, Context& c, Logger& l,
        Z3PebblingModel& m);

    // run ipdr without any incremental funtionality
    pebbling::IpdrPebblingResult control_run(Tactic tactic);
    pebbling::IpdrPebblingResult relax(bool control);
    pebbling::IpdrPebblingResult constrain(bool control);

    pebbling::IpdrPebblingResult new_total(Tactic t) const;
    z3PDR const& internal_alg() const;

   private:
    z3PDR alg;
    Z3PebblingModel& ts;
    std::optional<unsigned> starting_pebbles;

    void basic_reset(unsigned pebbles);
  }; // class Optimizer
} // namespace pdr::test

#endif // Z3PDR_H