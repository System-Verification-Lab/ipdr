#include "cli-parse.h"
#include "pdr.h"
#include "peterson-result.h"
#include "peterson.h"
#include "types-ext.h"
#include <cassert>

namespace pdr::peterson
{
  IPDR::IPDR(
      my::cli::ArgumentList const& args, Context c, Logger& l, PetersonModel& m)
      : vIPDR(args, c, l, m), ts(m), control_setting(args.control_run)
  {
    auto ipdr = my::variant::get_cref<my::cli::algo::t_IPDR>(args.algorithm);
    assert(ipdr->get().type == Tactic::relax);
  }

  IpdrPetersonResult IPDR::control_run(Tactic tactic, unsigned processes)
  {
    switch (tactic)
    {
      case Tactic::constrain:
        throw std::invalid_argument("Decrement not implemented.");
        break;
      case Tactic::relax: return relax(processes, true);
      case Tactic::inc_jump_test: relax_jump_test(processes, 10); break;
      case Tactic::inc_one_test: relax_jump_test(processes, 1); break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  IpdrPetersonResult IPDR::run(Tactic tactic, unsigned max_bound)
  {
    switch (tactic)
    {
      case Tactic::constrain:
        throw std::invalid_argument("Constrain not implemented.");
        break;
      case Tactic::relax: return relax(max_bound, control_setting);
      case Tactic::inc_jump_test: relax_jump_test(max_bound, 10); break;
      case Tactic::inc_one_test: relax_jump_test(max_bound, 1); break;
      default: break;
    }
    throw std::invalid_argument(
        "No optimization pdr tactic has been selected.");
  }

  IpdrPetersonResult IPDR::relax(unsigned max_bound, bool control)
  {
    alg.logger.and_whisper(
        "! Proving peterson for {} processes.", ts.n_processes());

    unsigned bound = 0;
    basic_reset(bound);

    IpdrPetersonResult total(ts, Tactic::relax);

    pdr::PdrResult invariant = alg.run();
    total.add(invariant, ts.n_processes());

    for (bound = bound + 1; invariant && bound <= max_bound; bound++)
    {
      spdlog::stopwatch timer;
      if (control)
        basic_reset(bound);
      else
        relax_reset(bound);
      total.append_inc_time(timer.elapsed().count()); // adds to previous result

      invariant = alg.run();

      total.add(invariant, ts.n_processes());
    }

    if (invariant && bound > max_bound) // last run did not find a trace
    {
      alg.logger.and_whisper("! No trace exists.");
      return total;
    }
    // N is minimal
    alg.logger.and_whisper("! Counter for switches switch", bound);
    return total;
  }

  // Private members
  //
  void IPDR::basic_reset(unsigned switches)
  {
    assert(std::addressof(ts) == std::addressof(alg.ts));

    alg.logger.and_show("naive change from {} -> {} switches",
        ts.get_switch_bound().value(), switches);

    ts.constrain_switches(switches);
    alg.ctx.type = Tactic::basic;
    alg.reset();
  }

  void IPDR::relax_reset(unsigned switches)
  {
    assert(std::addressof(ts) == std::addressof(alg.ts));

    unsigned old                   = ts.get_switch_bound().value();
    z3::expr_vector old_constraint = z3ext::copy(ts.get_constraint());
    assert(switches > old);

    alg.logger.and_show("increment from {} -> {} switches", old, switches);

    ts.constrain_switches(switches);

    alg.ctx.type = Tactic::relax;
    alg.frames.copy_to_Fk_keep(old, old_constraint);
  }

  IpdrPetersonResult IPDR::relax_jump_test(unsigned start, int step)
  {
    std::vector<pdr::Statistics> statistics;
    alg.logger.and_show("NEW INC JUMP TEST RUN");
    alg.logger.and_show("start {}. step {}", start, step);

    IpdrPetersonResult total(ts, Tactic::relax);
    basic_reset(start);
    pdr::PdrResult invariant = alg.run();
    total.add(invariant, ts.n_processes());

    unsigned oldp = ts.n_processes();
    unsigned newp = oldp + step;
    assert(newp > 0);
    assert(oldp < newp);
    assert(newp <= ts.get_switch_bound());

    relax_reset(newp);
    invariant = alg.run();

    total.add(invariant, ts.n_processes());

    return total;
  }
} // namespace pdr::peterson
