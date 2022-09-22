#ifndef PDR_ALG
#define PDR_ALG

#include "_logging.h"
#include "frames.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "result.h"
#include "stats.h"
#include "z3-ext.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <queue>
#include <spdlog/stopwatch.h>
#include <string>
#include <vector>
#include <z3++.h>
#include <z3_fpa.h>

namespace pdr
{
  namespace pebbling
  {
    class IPDR;
  }

  class PDR
  {
    friend class pebbling::IPDR;

   private:
    Context& ctx;
    IModel& model;

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;
    Logger& logger;

    Frames frames; // sequence of candidates
    std::set<Obligation, std::less<Obligation>> obligations;

    // if mic fails to reduce a clause c this many times, take c
    const unsigned mic_retries = UINT_MAX;

    void print_model(const z3::model& m);
    // main algorithm
    PdrResult run();
    PdrResult init();
    PdrResult iterate();
    PdrResult block(z3::expr_vector cti, unsigned n);
    PdrResult iterate_short();
    PdrResult block_short(z3::expr_vector&& counter, unsigned n);
    // generalization
    // todo return [n, cti ptr]
    int hif_(const z3::expr_vector& cube, int min);
    std::tuple<int, z3::expr_vector> highest_inductive_frame(
        const z3::expr_vector& cube, int min);
    z3::expr_vector generalize(const z3::expr_vector& cube, int level);
    z3::expr_vector MIC(const z3::expr_vector& cube, int level);
    bool down(std::vector<z3::expr>& cube, int level);
    // results
    void make_result(PdrResult& result);
    // to replace return value in run()
    // stores final logs, stats and result and returns its argument
    PdrResult finish(PdrResult&& rv);
    void store_frame_strings();
    std::string constraint_str() const;

    // logging shorthands
    void log_start() const;
    void log_iteration();
    void log_cti(const z3::expr_vector& cti, unsigned level);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(
        size_t queue_size, unsigned top_level, const z3::expr_vector& top);
    void log_pred(const z3::expr_vector& p);
    void log_state_push(unsigned frame);
    void log_finish(const z3::expr_vector& s);
    void log_obligation_done(std::string_view type, unsigned l, double time);

   public:
    PDR(Context& c, IModel& m, Logger& l);
    PDR(Context& c, IModel& m, Logger& l, std::optional<unsigned> constraint);
    // prepare PDR for new run. discards old trace
    void reset();
    const Context& get_ctx() const;
    Context& get_ctx();

    Statistics& stats();
    void show_solver(std::ostream& out) const;
    std::vector<std::string> trace_row(const z3::expr_vector& v);
    int length_shortest_strategy() const;
  };

  namespace pebbling
  {
    class IPDR
    {
     private:
      PDR alg;
      PebblingModel& model; // same instance as the IModel in alg
      Tactic tactic{ Tactic::undef };
      std::optional<unsigned> starting_value;

      void basic_reset(unsigned pebbles);
      void increment_reset(unsigned pebbles);
      std::optional<size_t> decrement_reset(unsigned pebbles);

     public:
      IPDR(
          Context& c, PebblingModel& m, my::cli::ArgumentList args, Logger& l);

      // runs the optimizer as dictated by the argument
      PebblingResult run(bool control);
      // runs the optimizer as dictated by the argument but with forced
      // experiment_control
      PebblingResult control_run();
      PebblingResult increment(bool control);
      PebblingResult decrement(bool control);
      void inc_jump_test(unsigned start, int step);

      void dump_solver(std::ofstream& out) const;
    }; // class Optimizer
  }    // namespace pebbling
} // namespace pdr
#endif // PDR_ALG
