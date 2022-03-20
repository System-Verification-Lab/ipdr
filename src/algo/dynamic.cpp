#include "pdr-model.h"
#include "pdr.h"
#include <bits/types/FILE.h>
#include <cassert>
#include <cstddef>
#include <dbg.h>
#include <string>

namespace pdr
{
  /*
    returns true if the incremental algorithm must continue
    returns false if no strategy with fewer than max_pebbles exists.
  */
  bool PDR::decrement(bool reuse)
  {
    const Model& m = ctx.const_model();
    unsigned k     = frames.frontier();

    int max_pebbles = m.get_max_pebbles();
    int new_pebbles = shortest_strategy - 1;
    assert(new_pebbles > 0);
    assert(new_pebbles < max_pebbles);

    if (new_pebbles < m.get_f_pebbles()) // not enough to pebble final state
      return false;

    reset();
    results.extend();
    logger.whisper("retrying with {}", new_pebbles);
    if (!reuse)
      return true;

    // TODO separate statstics from dyn runs?
    frames.reset_constraint(logger.stats, new_pebbles);

    logger.and_show(fmt::format("Dynamic: skip initiation. k = {}", k));
    // if we are reusing frames, the last propagation was k-1, repeat this
    int invariant = frames.propagate(true);
    if (invariant >= 0)
    {
      results.current().invariant_index = invariant;
      return true;
    }
    return false;
  }

  bool PDR::dec_tactic(std::ofstream& strategy, std::ofstream& solver_dump)
  {
    logger.and_show("NEW DEC RUN");
    const Model& m = ctx.const_model();
    int N          = m.n_nodes(); // cannot pebble more than this
    ctx.model().set_max_pebbles(N);
    bool found_strategy = !run(Tactic::basic);
    while (found_strategy)
    {
      int maxp = m.get_max_pebbles();
      int newp = shortest_strategy - 1;
      assert(newp > 0);
      assert(newp < maxp);
      if (newp > (int)m.n_nodes())
        return false;
      reset();
      results.extend();
      logger.whisper("Decremental run {} -> {} pebbles", maxp, newp);

      frames.reset_constraint(stats(), newp);
      found_strategy = !run(Tactic::decrement);
    }
    // N is minimal
    show_results(strategy);
    solver_dump << SEP3 << " final iteration " << N << std::endl;
    show_solver(solver_dump);
    return true;
  }

  bool PDR::inc_tactic(std::ofstream& strategy, std::ofstream& solver_dump)
  {
    logger.and_show("NEW INC RUN");
    const Model& m = ctx.const_model();
    int N          = m.get_f_pebbles(); // need at least this many pebbles
    ctx.model().set_max_pebbles(N);
    bool found_strategy = !run(Tactic::basic);
    while (!found_strategy)
    {
      int maxp = m.get_max_pebbles();
      int newp = maxp + 1;
      assert(newp > 0);
      assert(maxp < newp);
      if (newp > (int)m.n_nodes())
        return false;
      reset();
      results.extend();

      frames.increment_reset(stats(), newp);
      found_strategy = !run(Tactic::increment);
    }
    // N is minimal
    show_results(strategy);
    solver_dump << SEP3 << " final iteration " << N << std::endl;
    show_solver(solver_dump);
    return true;
  }

  bool PDR::inc_jump_test(int start, std::ofstream& strategy,
                          std::ofstream& solver_dump)
  {
    logger.and_show("NEW INC JUMP TEST RUN");
    const Model& m = ctx.const_model();
    int N          = start;
    ctx.model().set_max_pebbles(N);
    bool found_strategy = !run(Tactic::basic);
    if (true)
    {
      int maxp = m.get_max_pebbles();
      int newp = maxp + 10;
      assert(newp > 0);
      assert(maxp < newp);
      if (newp > (int)m.n_nodes())
        return false;
      reset();
      results.extend();

      frames.increment_reset(stats(), newp);
      found_strategy = !run(Tactic::increment);
    }
    // N is minimal
    show_results(strategy);
    solver_dump << SEP3 << " final iteration " << N << std::endl;
    show_solver(solver_dump);
    return true;
  }
} // namespace pdr
