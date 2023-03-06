#ifndef CLI_H
#define CLI_H

#include "dag.h"
#include "io.h"
#include "logger.h"
#include "tactic.h"

#include <cxxopts.hpp>
#include <fmt/core.h>
// #include <filesystem>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace my::cli
{
  using namespace cxxopts;
  using namespace my::io;

  class ArgumentList;

  namespace graph_src
  {
    struct benchFile
    {
      std::string name;
      fs::path file;
    };

    struct tfcFile
    {
      std::string name;
      fs::path file;
    };

    struct Hop
    {
      unsigned bits;
      unsigned mod;
      Hop(unsigned b, unsigned m) : bits(b), mod(m) {}
    };

    using Graph_var = std::variant<benchFile, tfcFile, Hop>;

    std::string get_name(Graph_var const& g);
    dag::Graph make_graph(Graph_var const& g);
  } // namespace graph_src

  namespace model_t
  {
    using namespace graph_src;

    struct Pebbling
    {
      std::optional<unsigned> max_pebbles; // starting value for constraint
      Graph_var src;
    };

    struct Peterson
    {
      unsigned start;
      unsigned max;
    };

    using Model_var = std::variant<Pebbling, Peterson>;

    std::string src_name(Model_var const& m);
    std::string describe(Model_var const& m);
    std::string get_name(Model_var const& m);
    std::string filetag(Model_var const& m);
  } // namespace model_t

  namespace algo
  {
    using namespace model_t;

    struct t_PDR
    {
    };

    struct t_IPDR
    {
      pdr::Tactic type;
      t_IPDR(pdr::Tactic t) : type(t) {}
    };

    struct t_Bounded
    {
    };

    using Algo_var = std::variant<t_PDR, t_IPDR, t_Bounded>;

    std::string get_name(Algo_var const& a);
    std::string filetag(Algo_var const& a);
    Model_var const& get_model(Algo_var const& a);
  } // namespace algo

  struct Experiment
  {
    unsigned repetitions;
    bool control_only;
  };

  class ArgumentList
  {
   public:
    FolderStructure folders;
    OutLvl verbosity{ OutLvl::whisper };
    std::optional<std::string> out; // filename to redirect logging out messages

    algo::Algo_var algorithm;
    model_t::Model_var model;

    std::optional<Experiment> experiment;
    std::variant<bool, unsigned> r_seed;
    std::optional<unsigned> mic_retries;
    bool tseytin;  // encode pebbling::Model transition using tseyting enconding
    bool onlyshow; // only read in and produce the model image and description

    bool z3pdr;

    bool _failed = false;

    ArgumentList(int argc, char* argv[]);
    void show_header(std::ostream& out) const;
    const FolderStructure make_folders() const;

   private:
    // constructor helpers
    Options make_options(std::string name);
    void parse_verbosity(ParseResult const& clresult);
    void parse_alg(ParseResult const& clresult);
    void parse_model(ParseResult const& clresult);
    void parse_run(ParseResult const& clresult);
    graph_src::Graph_var parse_graph_src(ParseResult const& clresult);

    // cli names
    inline static const std::string o_alg     = "algo";
    inline static const std::string s_pdr     = "pdr";
    inline static const std::string s_ipdr    = "ipdr";
    inline static const std::string s_bounded = "bounded";

    inline static const std::string s_pebbling = "pebbling";
    inline static const std::string s_peter    = "peterson";

    inline static const std::string s_z3pdr = "z3pdr";

    inline static const std::string s_exp     = "experiment";
    inline static const std::string s_control = "control";

    inline static const std::string o_inc       = "inc";
    inline static const std::string s_constrain = "constrain";
    inline static const std::string s_relax     = "relax";

    inline static const std::string s_pebbles = "pebbles";
    inline static const std::string s_procs   = "processes";

    inline static const std::string s_dir   = "dir";
    inline static const std::string s_bench = "bench";
    inline static const std::string s_tfc   = "tfc";
    inline static const std::string s_hop   = "hop";

    inline static const std::string s_rand    = "rand";
    inline static const std::string s_seed    = "seed";
    inline static const std::string s_tseytin = "tseytin";
    inline static const std::string s_show    = "show-only";

    inline static const std::string s_verbose = "verbose";
    inline static const std::string s_whisper = "whisper";
    inline static const std::string s_silent  = "silent";

    inline static const std::string s_mic = "mic-attempts";
  };
} // namespace my::cli
#endif // CLI_H
