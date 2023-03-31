#include <algorithm>
#include <cstddef>
#include <dbg.h>
#include <fmt/core.h>
#include <spdlog/stopwatch.h>
#include <vector>
#include <z3++.h>

#include "logger.h"
#include "pdr.h"
#include "string-ext.h"
#include "z3-ext.h"

namespace pdr
{
  using std::optional;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  //! s is inductive up until min-1. !s is included up until min
  PDR::HIFresult PDR::hif_(vector<expr> const& cube, int min)
  {
    int max = frames.frontier();
    if (min <= 0 && !frames.inductive(cube, 0))
    {
      MYLOG_DEBUG(logger, "Intersects I");
      return { -1, {} };
    }

    // F_result & !cube & T & cube' = UNSAT
    // => F_result & !cube & T & core' = UNSAT
    optional<vector<expr>> raw_core;

    int highest = max;
    for (int i = std::max(1, min); i <= max; i++)
    {
      // clause was inductive up to this iteration
      if (!frames.inductive(cube, i))
      {
        highest = i - 1; // previous was greatest inductive frame
        break;
      }
      raw_core = frames.get_solver(i).unsat_core();
    }

    MYLOG_DEBUG(logger, "highest inductive frame is {} / {}", highest,
        frames.frontier());
    return { highest, raw_core };
  }

  PDR::HIFresult PDR::highest_inductive_frame(vector<expr> const& cube, int min)
  {
    vector<expr> rv_core;
    HIFresult result = hif_(cube, min);

    if (result.level >= 0 && result.level >= min && result.core)
    { // if unsat result occurs
      // extract destination lits and convert to current state literals
      for (expr const& e : *result.core)
        if (ts.vars.lit_is_p(e))
          rv_core.push_back(ts.vars(e));

      MYLOG_DEBUG(logger, "core @{}: [{}]", result.level,
          rv_core ? z3ext::join_ev(rv_core) : "none");

      // if I => !core, the subclause survives initiation and is inductive
      if (frames.init_solver.check(rv_core.size(), rv_core.data()) == z3::sat)
      {
        MYLOG_DEBUG(logger, "unsat core is invalid. no reduction.");
        rv_core = cube; /// I /=> !core, use original
      }
      else
      {
        MYLOG_DEBUG(logger, "unsat core reduction: {} -> {}", cube.size(),
            rv_core.size());
      }
    }
    else
      rv_core = cube; // no core produced

    MYLOG_TRACE(logger, "new cube: [{}]", join_expr_vec(rv_core, false));
    return { result.level, rv_core };
  }

  void PDR::generalize(vector<expr>& state, int level)
  {
    MYLOG_DEBUG(logger, "generalize cube");
    MYLOG_TRACE(logger, "[{}]", join_expr_vec(state, false));

    logger.indent++;
    spdlog::stopwatch timer;
    IF_STATS(double s0 = state.size());
    unsigned pre_size = state.size();

    MIC(state, level);

    IF_STATS({
      logger.stats.generalization.add(level, timer.elapsed().count());
      double reduction = (s0 - smaller_cube.size()) / s0;
      logger.stats.generalization_reduction.add(reduction);
    });
    logger.indent--;

    MYLOG_DEBUG(
        logger, "generalization: {} -> {}", pre_size, state.size());
    MYLOG_TRACE(logger, "final reduced cube = [{}]",
        join_expr_vec(smaller_cube, false));
  }

// #define ctgmic true
#define ctgmic false

  void PDR::MIC(vector<expr>& cube, int level)
  {
    if (ctgmic)
    {
      // MICctg(rv, level, 1);
      // return rv;
    }

    assert(level <= (int)frames.frontier());
    // used for sorting

    unsigned attempts{ 0u };
    for (unsigned i{ 0 }; i < cube.size();)
    {
      assert(z3ext::lits_ordered(cube));
      vector<expr> new_cube(cube.begin(), cube.begin() + i);
      new_cube.reserve(cube.size() - 1);
      new_cube.insert(new_cube.end(), cube.begin() + i + 1, cube.end());

      MYLOG_TRACE(
          logger, "verifying subcube [{}]", join_expr_vec(new_cube, false));

      logger.indent++;
      if (down(new_cube, level))
      {
        MYLOG_TRACE(logger, "sub-cube survived");
        MYLOG_TRACE(logger, "down-reduced cube ({} -> {}): [{}]", cube.size(),
            new_cube.size(), join_expr_vec(new_cube));
        // current literal was dropped, i now points to the next
        cube = std::move(new_cube);
#warning try difference between tracking attempts per clause or no. times in a row
        // attempts = 0;
      }
      else
      {
        MYLOG_TRACE(logger, "sub-cube failed");
        i++;
      }
      logger.indent--;

      attempts++;
      if (attempts >= ctx.mic_retries)
      {
        IF_STATS(logger.stats.mic_limit++;);
        MYLOG_WARN(logger, "MIC exceeded {} attempts", ctx.mic_retries);
        break;
      }
    }
    IF_STATS(logger.stats.mic_attempts.add(attempts));
  }

  // @state is sorted
  bool PDR::down(vector<expr>& state, int level)
  {

    while (true)
    {
      assert(z3ext::lits_ordered(state));
      expr* const raw_state = state.data();
      if (frames.init_solver.check(state.size(), raw_state) == z3::sat)
      {
        MYLOG_TRACE(logger, "state includes I");
        return false;
      }

      if (!frames.inductive(state, level))
      {
        MYLOG_TRACE(logger, "state is not inductive");
        logger.indent++;
        state = frames.get_solver(level).witness_current_intersect(state);
        logger.indent--;
        MYLOG_TRACE(logger, "intersection witness and state: [{}]",
            z3ext::join_ev(state));
      }
      else
        return true;
    }
    return false;
  }

  void PDR::MICctg(vector<expr>& cube, int level, unsigned depth)
  {
    assert(level <= (int)frames.frontier());

    unsigned attempts{ 0u };
    for (unsigned i{ 0 }; i < cube.size();)
    {
      assert(z3ext::lits_ordered(cube));
      vector<expr> new_cube(cube.begin(), cube.begin() + i);
      new_cube.reserve(cube.size() - 1);
      new_cube.insert(new_cube.end(), cube.begin() + i + 1, cube.end());

      MYLOG_TRACE(
          logger, "verifying subcube [{}]", join_expr_vec(new_cube, false));

      logger.indent++;
      if (ctgdown(new_cube, level, depth))
      {
        MYLOG_TRACE(logger, "sub-cube survived");
        MYLOG_TRACE(logger, "down-reduced cube ({} -> {}): [{}]", cube.size(),
            new_cube.size(), join_expr_vec(new_cube));
        // current literal was dropped, i now points to the next
        cube = std::move(new_cube);
      }
      else
      {
        MYLOG_TRACE(logger, "sub-cube failed");
        i++;
      }
      logger.indent--;

      attempts++;
      if (attempts >= ctx.mic_retries)
      {
        IF_STATS(logger.stats.mic_limit++;);
        MYLOG_WARN(logger, "MIC exceeded {} attempts", ctx.mic_retries);
        break;
      }
    }
    IF_STATS(logger.stats.mic_attempts.add(attempts));
  }

  // @state is sorted
  bool PDR::ctgdown(vector<expr>& state, int level, unsigned depth)
  {
    unsigned ctgs = 0;

    while (true)
    {
      assert(z3ext::lits_ordered(state));
      expr* const raw_state = state.data();
      if (frames.init_solver.check(state.size(), raw_state) == z3::sat)
      {
        MYLOG_TRACE(logger, "state includes I");
        return false;
      }

      if (frames.inductive(state, level))
        return true;
      else
      {
        if (depth > ctx.ctg_max_depth)
          return false;

        vector<expr> ctg = frames.get_solver(level).std_witness_current();

        if (ctgs < ctx.ctg_max_counters && level > 0 &&
            frames.init_solver.check(ctg.size(), ctg.data()) &&
            frames.inductive(ctg, level - 1))
        {
          ctgs++;
          assert(level >= 0);
          size_t i;
          for (i = level; i < frames.frontier(); i++)
            if (!frames.inductive(ctg, i))
              break;
          MICctg(ctg, i - 1, depth + 1);
          frames.remove_state(ctg, i);
        }
        else
        {
          ctgs = 0;
          // TODO state = 
        }

        MYLOG_TRACE(logger, "state is not inductive");
        MYLOG_TRACE(logger, "intersection witness and state: [{}]",
            z3ext::join_ev(state));
      }
    }
    return false;
  }
} // namespace pdr
