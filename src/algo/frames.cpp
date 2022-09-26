#include "frames.h"
#include "_logging.h"
#include "frame.h"
#include "output.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
  using std::make_unique;
  using std::optional;
  using z3::expr;
  using z3::expr_vector;

  Frames::Frames(Context& c, IModel& m, Logger& l)
      : ctx(c), model(m), logger(l), frame_base(ctx), init_solver(ctx)
  {
    // the initial states always remain the same
    init_solver.reset();
    init_solver.add(model.get_initial());

    reset();
  }

  void Frames::reset()
  {
    pdr::IModel& m = model;
    frame_base = m.property; // all frames are initialized to P

    if (ctx.delta)
    {
      const expr_vector& T = m.get_transition();
      expr_vector constraint = m.get_constraint();
      delta_solver = make_unique<Solver>(ctx, model, frame_base, T, constraint);
    }

    frames.resize(0);
    while (ctx.delta && act.size() > 0)
      act.pop_back();

    init_frame_I();
    extend();
    assert(frontier() == 0);
  }

  optional<size_t> Frames::reuse()
  {
    assert(frames.size() > 0);
    assert(model.diff == IModel::Diff_t::constrained);

    delta_solver->reconstrain(model.get_constraint());

    // with fewer transitions, new cubes may be propagated
    logger.and_show("Redoing last propagation: {}", frontier() - 1);

    model.diff = IModel::Diff_t::none;
    return propagate(frontier() - 1);
  }

  void Frames::reset_to_F1()
  {
#warning increment_reset is delta only
    assert(frames.size() > 0);
    assert(model.diff == IModel::Diff_t::relaxed);
    logger("Reset frames to F1");

    delta_solver->reconstrain(model.get_constraint());
    z3ext::CubeSet old = get_blocked(1); // store all cubes in F_1
    clear_until(0);                      // reset sequence to { F_0 }
    extend();                            // reinstate level 1

    unsigned count = 0;
    for (const z3::expr_vector& cube : old)
    {
      if (SAT(0, cube))
      {
#warning todo/future work regeneralize cti from this cube (must be possible or counter)
        // TODO regeneralize cti from this cube (must be possible or counter)
        // else it will be reconsidered next iteration
      }
      else
      {
        count++;
        remove_state(cube, 1);
      }
    }
    logger.stats.copied_cubes.count += count;
    logger.stats.copied_cubes.total += old.size();
    logger.and_show(
        "pre-INC: {} cubes carried over, out of {}", count, old.size());
    logger("Repopulated delta solver: \n{}", delta_solver->as_str("", false));

    model.diff = IModel::Diff_t::none;
  }

  void Frames::init_frame_I()
  {
    const expr_vector& I   = model.get_initial();
    const expr_vector& T   = model.get_transition();
    expr_vector constraint = model.get_constraint();

    if (ctx.delta)
    {
      assert(act.size() == 0);
      act.push_back(ctx().bool_const("__actI__")); // unused
    }

    // make F_0 = I transition solver
    {
      auto frame_solver = make_unique<Solver>(ctx, model, I, T, constraint);
      auto new_frame = make_unique<Frame>(0, std::move(frame_solver), logger);
      frames.push_back(std::move(new_frame));
    }
    logger("solver after init {}", delta_solver->as_str("", false));
  }

  // frame interface
  //
  // removes frames until the frontier is the given argument
  void Frames::clear_until(size_t frontier_index)
  {
    if (ctx.delta)
      assert(frames.size() == act.size());

    // pop until given index is the highest
    while (frontier() > frontier_index)
    {
      frames.pop_back();
      if (ctx.delta)
        act.pop_back();
    }
  }

  void Frames::extend()
  {
    assert(frames.size() > 0);
    if (ctx.delta)
    { // universal delta_solver is used for this frame
      std::string acti = fmt::format("__act{}__", frames.size());
      act.push_back(ctx().bool_const(acti.c_str()));
      frames.push_back(make_unique<Frame>(frames.size(), logger));
    }
    else
    { // frame with its own solver
      const z3::expr_vector& t = model.get_transition();
      expr_vector constr       = model.get_constraint();

      auto frame_solver = make_unique<Solver>(ctx, model, frame_base, t, constr);
      auto new_frame =
          make_unique<Frame>(frames.size(), std::move(frame_solver), logger);
      frames.push_back(std::move(new_frame));
    }
  }

  void Frames::repopulate_solvers()
  {
    if (ctx.delta)
    {
      delta_solver->reset();
      for (size_t i = 1; i < frames.size(); i++)
        for (const z3::expr_vector& cube : frames[i]->get_blocked())
          delta_solver->block(cube, act.at(i));
    }
    else
    {
      for (size_t i = 1; i < frames.size(); i++)
        get_solver(i).reset(frames[i]->get_blocked());
    }
  }

  z3ext::CubeSet Frames::get_blocked(size_t i) const
  {
    assert(i < frames.size());
    z3ext::CubeSet blocked;

    if (ctx.delta)
    {
      delta_solver->reset();
      // all cubes in all delta-level belong to F_1
      for (; i < frames.size(); i++)
      {
        // TODO non-const getter allows std::move
        const z3ext::CubeSet& Fi = frames[i]->get_blocked();
        blocked.insert(Fi.begin(), Fi.end());
      }
    }
    else
      blocked = frames[1]->get_blocked();

    return blocked;
  }

  bool Frames::remove_state(const z3::expr_vector& cube, size_t level)
  {
    assert(level < frames.size());
    // level = std::min(level, frames.size() - 1);
    logger.tabbed("removing cube from level [1..{}]: [{}]", level,
        str::extend::join(cube));
    logger.indent++;

    bool result;
    if (ctx.delta) // a cube is only stored in the last frame it holds
      result = delta_remove_state(cube, level);
    else
      result = fat_remove_state(cube, level);
    logger.indent--;
    return result;
  }

  bool Frames::delta_remove_state(const z3::expr_vector& cube, size_t level)
  {
    for (unsigned i = 1; i <= level; i++)
    {
      // remove all blocked cubes that are equal or weaker than cube
      unsigned n_removed = frames.at(i)->remove_subsumed(cube, i < level);
      logger.stats.subsumed_cubes.add(level, n_removed);
    }

    assert(level > 0 && level < frames.size());
    if (frames[level]->block(cube))
    {
      delta_solver->block(cube, act.at(level));
      logger.tabbed("blocked in {}", level);
      return true;
    }
    else
      logger.tabbed("was already blocked in {}", level);

    return false;
  }

  bool Frames::fat_remove_state(const z3::expr_vector& cube, size_t level)
  {
    assert(level > 0 && level < frames.size());
    for (unsigned i = 1; i <= level; i++)
    {
      // remove all blocked cubes that are equal or weaker than cube
      unsigned n_removed = frames.at(i)->remove_subsumed(cube, i < level);
      logger.stats.subsumed_cubes.add(level, n_removed);

#warning subsumes is now not automatic
      if (frames[i]->block(cube))
      {
        frames[i]->block_in_solver(cube);
        logger.tabbed("blocked in {}", i);
      }
      else
        return false;
    }
    return true;
  }

  std::optional<size_t> Frames::propagate() { return propagate(frontier()); }
  std::optional<size_t> Frames::propagate(size_t k)
  {
    assert(k <= frontier());
    logger.tabbed_and_whisper("propagate levels {} - {}", 1, k);
    logger.indent++;
    bool repeat = (k < frontier());

    for (size_t i = 1; i <= k; i++)
    {
      if (ctx.delta)
        push_forward_delta(i, repeat);
      else if (push_forward_fat(i, repeat))
        return i;
    }

    if (ctx.delta) // check for empty level
      for (size_t i = 1; i <= k; i++)
        if (frames.at(i)->empty())
        {
          logger.and_whisper("F[{}] \\ F[{}] == 0", i, i + 1);
          return i;
        }

    repopulate_solvers();
    logger.indent--;

    return {};
  }

  void Frames::push_forward_delta(size_t level, bool repeat)
  {
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    unsigned count         = 0;
    z3ext::CubeSet blocked = frames.at(level)->get_blocked();
    for (const z3::expr_vector& cube : blocked)
    {
      if (!trans_source(level, cube))
      {
        if (remove_state(cube, level + 1))
          if (repeat)
            count++;
      }
    }
    if (repeat)
      logger.and_show("{} blocked in repeat", count);

    std::chrono::duration<double> dt(steady_clock::now() - start);
    logger.stats.propagation_level.add_timed(level, dt.count());
  }

  std::optional<size_t> Frames::push_forward_fat(size_t level, bool repeat)
  {
    std::optional<size_t> rv = {};
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    unsigned count = 0;
    std::vector<z3::expr_vector> diff =
        frames.at(level)->diff(*frames.at(level + 1));
    for (const z3::expr_vector& cube : diff)
    {
      if (!trans_source(level, cube))
      {
        if (remove_state(cube, level + 1))
          if (repeat)
            count++;
      }
    }
    if (repeat)
      logger.and_show("{} blocked in repeat", count);

    if (diff.size() == 0 || frames.at(level)->equals(*frames.at(level + 1)))
    {
      logger.and_whisper("F[{}] \\ F[{}] == 0", level, level + 1);
      rv = level;
    }

    std::chrono::duration<double> dt(steady_clock::now() - start);
    logger.stats.propagation_level.add_timed(level, dt.count());
    return rv;
  }
  //
  // end frame interface

  // queries
  //
  bool Frames::inductive(const std::vector<z3::expr>& cube, size_t frame) const
  {
    return inductive(z3ext::convert(cube), frame);
  }

  // verifies if !cube is inductive relative to F_[frame]
  // query: Fi & !s & T /=> !s'
  bool Frames::inductive(const z3::expr_vector& cube, size_t frame) const
  {
    if (LOG_SAT_CALLS && ctx.delta)
      logger.tabbed("check relative inductiveness, frame {}", frame);

    z3::expr clause =
        z3::mk_or(z3ext::negate(cube)); // negate cube via demorgan
    z3::expr_vector assumptions = model.vars.p(cube); // cube in next state
    assumptions.push_back(clause);

    if (SAT(frame, std::move(assumptions)))
    { // there is a transition from !s to s'
      return false;
    }

    return true;
  }

  std::optional<z3::expr_vector> Frames::counter_to_inductiveness(
      const std::vector<z3::expr>& cube, size_t frame) const
  {
    if (LOG_SAT_CALLS && ctx.delta)
      logger.tabbed("counter to relative inductiveness, frame {}", frame);

    if (!inductive(cube, frame))
      return get_solver(frame).witness_current();

    return {};
  }

  std::optional<z3::expr_vector> Frames::counter_to_inductiveness(
      const z3::expr_vector& cube, size_t frame) const
  {
    if (!inductive(cube, frame))
      return get_solver(frame).witness_current();

    return {};
  }

  // if primed: cube is already in next state, else first convert it
  bool Frames::trans_source(
      size_t frame, const z3::expr_vector& dest_cube, bool primed) const
  {
    if (LOG_SAT_CALLS)
      logger.tabbed("transition check, frame {}", frame);
    if (!primed) // cube is in current, bring to next
      return SAT(frame, model.vars.p(dest_cube));

    return SAT(frame, dest_cube); // there is a transition from Fi to s'
  }

  std::optional<z3::expr_vector> Frames::get_trans_source(
      size_t frame, const z3::expr_vector& dest_cube, bool primed) const
  {
    if (LOG_SAT_CALLS)
      logger.tabbed("transition query, frame {}", frame);

    if (!primed) // cube is in current, bring to next
      if (!SAT(frame, model.vars.p(dest_cube)))
        return {};

    if (!SAT(frame, dest_cube))
      return {};
    // else there exists a source -T-> dest'
    return get_solver(frame).witness_current();
  }

  //
  // end queries

  // SAT interface
  //
  bool Frames::SAT(size_t frame, const z3::expr_vector& assumptions) const
  {
    return SAT(frame, z3ext::copy(assumptions));
  }

  // the expr_vector assumptions are modified by acts,
  // and should be considered usable afterwards
  bool Frames::SAT(size_t frame, z3::expr_vector&& assumptions) const
  {
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    Solver& solver = get_solver(frame);
    if (ctx.delta && frame > 0)
    {
      assert(frames.size() == act.size());
      for (unsigned i = frame; i < act.size(); i++)
        assumptions.push_back(act[i]);

      if (LOG_SAT_CALLS)
        logger.tabbed("Delta check");
    }
    else if (LOG_SAT_CALLS)
      logger.tabbed("Fat check");

    if (LOG_SAT_CALLS)
    {
      logger.indent++;
      logger.tabbed(
          "assumps: [ {} ]", z3ext::join_expr_vec(assumptions, false));
      logger.indent--;
    }

    bool result = solver.SAT(assumptions);
    std::chrono::duration<double> diff(steady_clock::now() - start);
    logger.stats.solver_calls.add_timed(frontier(), diff.count());

    return result;
  }

  const z3::model Frames::get_model(size_t frame) const
  {
    if (ctx.delta && frame > 0)
      return delta_solver->get_model();
    else
      return frames.at(frame)->get_solver().get_model();
  }

  //
  // end SAT interface

  // getters
  //
  // the index 'k' to the second to last frame
  size_t Frames::frontier() const
  {
    assert(frames.size() > 1); // 0 is the minimal frontier (series F_0, F_1)
    return frames.size() - 2;
  }

  Solver& Frames::get_solver(size_t frame) const
  {
    assert(frame < frames.size());
    if (ctx.delta && frame > 0)
      return *delta_solver;
    return frames[frame]->get_solver();
  }

  const Solver& Frames::get_const_solver(size_t frame) const
  {
    assert(frame < frames.size());
    if (ctx.delta && frame > 0)
      return *delta_solver;
    return frames[frame]->get_const_solver();
  }

  const Frame& Frames::operator[](size_t i)
  {
    assert(i < frames.size());
    return *frames[i];
  }

  //
  // end getters

  void Frames::log_solvers(bool only_clauses) const
  {
    logger(SEP3);
    if (ctx.delta)
    {
      logger(delta_solver->as_str("", only_clauses));
    }
    else
      for (const std::unique_ptr<Frame>& f : frames)
      {
        logger(f->get_solver().as_str("", only_clauses));
      }
    logger(SEP3);
  }

  std::string Frames::blocked_str() const
  {
    std::string str;
    for (auto& f : frames)
    {
      str += f->blocked_str();
      str += '\n';
    }
    return str;
  }
  std::string Frames::solvers_str(bool only_clauses) const
  {
    std::string str;
    if (ctx.delta)
      str += delta_solver->as_str("", only_clauses);
    else
    {
      for (auto& f : frames)
      {
        str += f->get_solver().as_str("", only_clauses);
        str += '\n';
      }
    }
    return str;
  }

} // namespace pdr
