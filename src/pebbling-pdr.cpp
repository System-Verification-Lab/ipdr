﻿#include "bounded.h"
#include "cli-parse.h"
#include "dag.h"
#include "h-operator.h"
#include "io.h"
#include "logger.h"
#include "mockturtle/networks/klut.hpp"
#include "parse_bench.h"
#include "parse_tfc.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-experiments.h"
#include "pebbling-model.h"
#include "peterson-experiments.h"
#include "peterson-result.h"
#include "peterson.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <cxxopts.hpp>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <ghc/filesystem.hpp>
#include <iostream>
#include <lorina/bench.hpp>
#include <memory>
#include <ostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>
#include <vector>
#include <z3++.h>

using namespace my::cli;
using namespace my::io;

template <typename T> using cc_ptr = T const* const;

void show_files(std::ostream& os, std::map<std::string, fs::path> paths)
{
  // show used paths
  TextTable output_files;
  for (auto kv : paths)
  {
    auto row = { kv.first, kv.second.string() };
    output_files.addRow(row);
  }
  os << output_files << std::endl;
}

//
// end OUTPUT

std::ostream& operator<<(std::ostream& o, std::exception const& e)
{
  o << fmt::format(
           "terminated after throwing an \'std::exception\', typeid: {}",
           typeid(e).name())
    << std::endl
    << fmt::format("  what():  {}", e.what()) << std::endl;
  return o;
}

////////////////////////////////////////////////////////////////////////////////

dag::Graph setup_graph(const ArgumentList& args)
{
  // create folders and files for I/O
  fs::path model_dir = create_model_dir(args);
  std::ofstream graph_descr =
      trunc_file(args.folders.model_dir, "graph", "txt");

  dag::Graph G = build_dag(args);
  {
    G.show_image(model_dir / "dag");
    std::cout << G.summary() << std::endl;
    graph_descr << G.summary() << std::endl << G;
  }

  return G;
}

void experiment(ArgumentList& clargs)
{
  using std::ofstream;
  using std::string;

  std::cerr << "experiment()" << std::endl;

  dag::Graph G = build_dag(clargs);

  pdr::pebbling::PebblingModel model(clargs, G);

  const fs::path model_dir = create_model_dir(clargs);
  const string filename    = file_name(clargs);
  const fs::path run_dir   = setup(model_dir / run_folder_name(clargs));

  string sub = "analysis";
  fs::create_directory(run_dir / sub);
  ofstream stat_file = trunc_file(run_dir / sub, filename, "stats");
  ofstream strategy  = trunc_file(run_dir / sub, filename, "strategy");
  fs::path log_file  = run_dir / sub / fmt::format("{}.log", filename);

  pdr::Statistics stats =
      pdr::Statistics::PebblingStatistics(std::move(stat_file), G);
  pdr::Logger logger =
      pdr::Logger(log_file.string(), clargs.verbosity, std::move(stats));

  pdr::pebbling::experiments::pebbling_run(model, logger, clargs);
  std::cout << "experiment done" << std::endl;
}

void peter_experiment(ArgumentList& args)
{
  using std::ofstream;
  using std::string;

  std::cerr << "peter_experiment()" << std::endl;

  unsigned p = 3, N = 3;
  pdr::peterson::PetersonModel model(p, N);
  args.model_name = fmt::format("peter-p{}-N{}-relax", p, N);

  {
    auto model_logger = spdlog::basic_logger_st("model_dump",
        args.folders.file_in_model(args.model_name, "model"), true);
    model_logger->set_level(spdlog::level::trace);
    spdlog::set_pattern("");

    tabulate::Table t;
    t.add_row({ "I", model.get_initial().to_string() });
    t.add_row(
        { "P", model.property().to_string(), model.property.p().to_string() });
    t.add_row({ "!P", model.n_property().to_string(),
        model.n_property.p().to_string() });
    // t.add_row({ "T", model.get_constraint().to_string() });
    SPDLOG_LOGGER_TRACE(model_logger, t.str());
    model_logger->flush();
  }

  ofstream stat_file =
      trunc_file(args.folders.analysis, args.folders.file_base, "stats");
  ofstream trace_file =
      trunc_file(args.folders.analysis, args.folders.file_base, "trace");
  fs::path log_file = args.folders.analysis / args.folders.file("log");

  args.folders.show(std::cerr);

  pdr::Statistics stats =
      pdr::Statistics::PeterStatistics(std::move(stat_file), p, N);
  pdr::Logger logger =
      pdr::Logger(log_file.string(), args.verbosity, std::move(stats));

  // return;
  pdr::peterson::experiments::peterson_run(model, logger, args);
  std::cout << "experiment done" << std::endl;
}

#warning dont cares (?) in trace for non-tseytin. dont always make sense? mainly in high constraints
int main(int argc, char* argv[])
{
  using model_type::t_Pebbling;
  using model_type::t_Peterson;
  using std::ofstream;

  ArgumentList args(argc, argv);

  ofstream model_descr = trunc_file(args.folders.model_dir, "model", "txt");
  ofstream strat_file  = args.folders.file_in_run("trace");
  ofstream solver_dump = args.folders.file_in_run("solver_dump", "solver");

  pdr::Statistics stats(args.folders.file_in_run("stats"));
  pdr::Logger logger = pdr::Logger(args.folders.file_in_run("log"), *args.out,
      args.verbosity, std::move(stats));

  if (cc_ptr<t_Pebbling> peb_descr = std::get_if<t_Pebbling>(&args.model))
  {
    using namespace pdr::pebbling;

    dag::Graph G = model_type::make_graph(peb_descr->src);
    G.show(args.folders.model_dir / "dag", true);
    stats.is_pebbling(G);

    PebblingModel pebbling(args, G);
    pebbling.show(model_descr);
      pdr::Context context =
          std::holds_alternative<bool>(args.r_seed)
              ? pdr::Context(pebbling, std::get<bool>(args.r_seed))
              : pdr::Context(pebbling, std::get<unsigned>(args.r_seed));

    pdr::PDR pdr_algo(context, pebbling, logger);
    pebbling.constrain(peb_descr->max_pebbles.value());
    pdr::IpdrResult rs(pebbling);
    pdr::PdrResult r = pdr_algo.run();
    rs.add(r).show(strat_file);
    rs.show_traces(strat_file);
    pdr_algo.show_solver(solver_dump);
  }
  else if (cc_ptr<t_Peterson> peter_descr =
               std::get_if<t_Peterson>(&args.model))
  {
    using namespace pdr::peterson;

    PetersonModel peter(peter_descr->start, peter_descr->max);
    stats.is_peter(peter_descr->start, peter_descr->max);
    peter.show(model_descr);

    assert(false && "TODO: peterson control flow");
  }

  if (args.onlyshow)
    return 0;

  args.show_header(std::cerr);

  // if (std::holds_alternative<algo::PDR>(args.algorithm))
  // {
  //   pdr::PDR pdr_algo(context, pebbling, logger);
  //   pdr::IpdrResult rs(pebbling);
  //   model.constrain(args.);
  //   pdr::PdrResult r = algorithm.run();
  //   rs.add(r).show(strat_file);
  //   rs.show_traces(strat_file);
  //   algorithm.show_solver(solver_dump);

  //   return 0;
  // }
  // else if (cc_ptr<algo::IPDR> algo = std::get_if<algo::IPDR>(&args.algorithm))
  // {
  //   using namespace model_type;

  //   if (args.experiment)
  //     experiment(args);

  //   if (cc_ptr<model_type::Pebbling> m = std::get_if<Pebbling>(&args.model))
  //   {
  //     pdr::pebbling::PebblingModel pebbling(args, make_graph(m->model));
  //     pdr::Context context =
  //         std::holds_alternative<bool>(args.r_seed)
  //             ? pdr::Context(pebbling, std::get<bool>(args.r_seed))
  //             : pdr::Context(pebbling, std::get<unsigned>(args.r_seed));

  //     pdr::pebbling::IPDR optimize(context, pebbling, args, logger);
  //     pdr::pebbling::PebblingResult result = optimize.run(algo->type, false);
  //     result.show(strat_file);
  //     result.show_raw(strat_file);
  //     result.show_traces(strat_file);
  //     optimize.dump_solver(solver_dump);
  //   }
  //   else // Peterson
  //   {
  //     using namespace pdr::peterson;

  //     const Peterson& m = std::get<Pebbling>(args.model);
  //     PetersonModel peter(m.start, m.max);
  //     IPDR incremental_prove(context, peter, args, logger);
  //     PetersonResult result = incremental_prove.run(pdr::Tactic::relax, false);
  //     result.show(strat_file);
  //     result.show_raw(strat_file);
  //     result.show_traces(strat_file);
  //     optimize.dump_solver(solver_dump);
  //   }
  // }
  // else if (std::holds_alternative<Bounded>(args.algorithm))
  // {
  //   dag::Graph G = get_graph(model.model);
  //   bounded::BoundedPebbling obj(G, args);
  //   obj.find_for(G.nodes.size());
  // }

  std::cout << "goodbye :)" << std::endl;
  return 0;
}
