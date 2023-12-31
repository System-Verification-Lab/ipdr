#ifndef PETERSON_H
#define PETERSON_H

#include <z3++.h>

#include "expr.h"
#include "pdr-model.h"

namespace pdr::peterson
{
  struct PetersonState;
  struct Vars;

  // encode the peterson algorithm for mutual exclusion as a cnf formula
  // for p processes, to a maximum of N.
  // The algorithm can be constrained to only allow a maximum number of context
  // switches: a firing of one process after another has just fired
  //
  // initial: pc[i] <- 0, level[i] <- 0, free[i] <- true, last[i] <- 0
  //
  // transition:
  //  0: idle
  //    level[i] <- 0
  //  1: boundcheck
  //    if level[i] < N-1 then pc[i] <- 2.
  //    if level[i] >= N-1 then pc[i] <- 4.
  //  2: set last
  //    last[level[i]] <- i
  //  3: wait
  //    if last[level[i]] == i && exists k != i: level[k] >= level[i] then
  //      pc[i] <- 3.
  //    else
  //      pc[i] <- 1
  //      level[i] <- level[i] + 1
  //  4: critical section (fire to release)
  //    pc[i] <- 0
  //    level[i] <- 0
  //    free[i] <- true
  //
  // property: only one process in pc[4] at the same time
  //
  // constraint: switch_count <= max_switches
  class PetersonModel : public pdr::IModel
  {
   public:
    using IStays   = mysat::primed::IStays;
    using Lit      = mysat::primed::Lit;
    using BitVec   = mysat::primed::BitVec;
    using numrep_t = BitVec::numrep_t;

    // the maximum amount of switches that can be tracked
    static constexpr size_t SWITCH_COUNT_MAX = 31; // 5 bits

    // enum Internals
    // {
    //   undef,
    //   mine,
    //   z3_fp,
    // };

    friend PetersonState;

    // from IModel
    // std::string name;
    // z3::context ctx;
    // ExpressionCache lits;
    // ExpressionCache property;
    // ExpressionCache n_property;

    // const z3::expr_vector& get_transition() const;
    // const z3::expr_vector& get_initial() const;

    // void show(std::ostream& out) const;

    PetersonModel(z3::context& c,
        numrep_t n_procs,
        numrep_t m_procs,
        std::optional<numrep_t> m_switches);

    static PetersonModel constrained_switches(
        z3::context& c, numrep_t n_procs, numrep_t m_switches);
    static PetersonModel constrained_procs(
        z3::context& c, numrep_t n_procs, numrep_t max_procs);

    // void load_initial(z3::fixedpoint& engine) override;
    // void load_transition(z3::fixedpoint& engine) override;
    // z3::expr create_fp_target() override;
    // z3::func_decl& fp_query_ref() override;
    // PdrResult::Trace::TraceVec fp_trace_states(z3::fixedpoint& engine)
    // override;

    const z3::expr get_constraint_current() const override;
    unsigned state_size() const override;
    const std::string constraint_str() const override;
    unsigned constraint_num() const override;
    unsigned n_processes() const;
    std::optional<unsigned> get_switch_bound() const;

    // Configure IModel
    void constrain_switches(std::optional<numrep_t> m);

    // Convert a cube (typically a witness from a SAT call) to a state
    PetersonState extract_state(const z3::expr_vector& witness,
        mysat::primed::lit_type t = mysat::primed::lit_type::base) const;
    PetersonState extract_state_p(const z3::expr_vector& witness) const;

    void test_room();
    void test_wait(numrep_t i);
    void test_property();
    void test_p_pred();

   private:
    // inherited from IModel
    // z3::expr_vector initial;    // each array index to '-1;. pc to 0
    // z3::expr_vector transition; // converted into cnf via tseytin

    // z3::fixedpoint support
    // z3::func_decl step; // B^N B^N |-> B
    // Rule reach_rule;

    // std::vector<z3::expr> fp_pc, fp_level, fp_last;
    // z3::expr fp_proc_last, fp_switch_count;
    // std::vector<z3::expr> fp_pc_p, fp_level_p, fp_last_p;
    // z3::expr fp_proc_last_p, fp_switch_count_p;

    // z3::expr_vector fp_vars0, fp_vars1, fp_vars01;
    // z3::func_decl fp_state, fp_step;
    // z3::sort_vector fp_state_sorts;

    // max no. processes. the size of the waiting queue
    const numrep_t N;
    // no. processes that can fire
    numrep_t p;
    //  constraint on number of allowed context-switches per run
    std::optional<numrep_t> max_switches;

    BitVec proc_last;    // last active process
    BitVec switch_count; // no. context switches performed
    // program counter for each process.
    std::vector<BitVec> pc; // pc[0..N-1] -> {0, 1, 2, 3, 4}
    const static numrep_t pc_num = 5;
    // level of each process. 
    // level[i] = 0 is a idle process. 
    // level[i] >= 1 is a waiting process.
    std::vector<BitVec> level; // level[0..N-1] -> {0..N-2}
    // last process to enter a level.
    std::vector<BitVec> last; // last[1..N-1] -> {0..N-1}
    // flag that denotes if process i has released the resource
    // alternatively viewed as a sign bit for l
    // std::vector<Lit> free;

    size_t n_lits() const;

    // fill the pc, level, free and last variables
    Vars mk_vars();
    void reset_initial();
    void reset_transition();

    // returns which variables are used in the cube. throws if more than one
    // type is used.
    std::set<PetersonState> successors(const z3::expr_vector& v);
    std::set<PetersonState> successors(const PetersonState& s);

    // z3::fixedpoint support
    // Internals verify_cube(z3::expr_vector const& cube) const;

    z3::expr T_start(numrep_t i);
    z3::expr T_boundcheck(numrep_t i);
    z3::expr T_setlast(numrep_t i);
    z3::expr T_await(numrep_t i);
    z3::expr T_release(numrep_t i);

    void fp_declarations(z3::fixedpoint& engine);

  }; // class PetersonModel

  struct PetersonState
  {
    std::vector<int> pc;
    std::vector<int> level;
    // std::vector<bool> free;
    std::vector<int> last;
    std::optional<int> proc_last;
    std::optional<int> switch_count;

    // TODO use model vector sizes
    PetersonState()
        : pc(0),
          level(0),
          // free(0),
          last(0)
    {
    }
    PetersonState(PetersonModel::numrep_t N)
        : pc(N),
          level(N),
          // free(N),
          last(N)
    {
    }

    z3::expr_vector cube(PetersonModel& m) const;
    z3::expr_vector cube_p(PetersonModel& m) const;
    std::string to_string(bool inl = false) const;
    std::string inline_string() const;

    bool operator<(const PetersonState&) const;
    bool operator==(const PetersonState&) const;
    bool operator!=(const PetersonState&) const;
  };

  struct Vars
  {
    std::vector<std::string> curr;
    std::vector<std::string> next;
  };

} // namespace pdr::peterson

#endif // PETERSON_H
