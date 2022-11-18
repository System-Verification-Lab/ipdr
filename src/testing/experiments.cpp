#include "experiments.h"
#include "cli-parse.h"
#include "pebbling-result.h"
#include "result.h"
#include "tactic.h"
#include "types-ext.h"

#include <fmt/format.h>
#include <numeric> // std::accumulate
#include <tabulate/exporter.hpp>
#include <tabulate/format.hpp>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include <variant>

namespace pdr::experiments
{
  using namespace my::cli;
  using std::vector;

  namespace math
  {
    std::string time_str(double x) { return fmt::format("{:.3f} s", x); };

    double std_dev(const vector<double>& v)
    {
      double mean = std::accumulate(v.cbegin(), v.cend(), 0.0) / v.size();
      return std_dev(v, mean);
    }

    double std_dev(const vector<double>& v, double mean)
    {
      double total_variance = std::accumulate(v.begin(), v.end(), 0.0,
          [mean](double a, double t) { return a + (t - mean) * (t - mean); });

      return std::sqrt(total_variance / v.size());
    }
  } // namespace math

  namespace tablef
  {
    tabulate::Format& format_base(tabulate::Format& f)
    {
      return f.font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
    }

    tabulate::Format& format_base(tabulate::Table& t)
    {
      return format_base(t.format());
    }

    tabulate::Table init_table()
    {
      tabulate::Table t;
      format_base(t);
      return t;
    }
  } // namespace tablef

  // RUN PUBLIC MEMBERS
  //
  Run::Run(std::string const& t, std::string const& m,
      std::vector<std::unique_ptr<IpdrResult>>&& r)
      : results(std::move(r)), model(m), tactic(t)
  {
    double time_sum{ 0.0 };
    vector<double> times;
    for (auto const& r : results)
    {
      time_sum += r->get_total_time();
      times.push_back(r->get_total_time());
    }

    assert(times.size() == results.size());
    avg_time     = time_sum / times.size();
    std_dev_time = math::std_dev(times, avg_time);
  }

  tabulate::Table::Row_t Run::tactic_row() const { return { "", tactic }; }

  tabulate::Table::Row_t Run::avg_time_row() const
  {
    return { "avg time", math::time_str(avg_time) };
  }

  tabulate::Table::Row_t Run::std_time_row() const
  {
    return { "std dev time", math::time_str(std_dev_time) };
  }

  std::string Run::str(output_format form) const
  {
    using std::endl;
    // tabulate::Table table = tablef::init_table();
    tabulate::Table table = make_table();
    tablef::format_base(table);

    std::stringstream ss;
    switch (form)
    {
      case output_format::string:
        ss << fmt::format("Experiment: {}", model) << endl << table;
        break;
      case output_format::latex:
        ss << tabulate::LatexExporter().dump(table) << endl;
        break;
      case output_format::markdown:
        ss << tabulate::MarkdownExporter().dump(table) << endl;
        break;
    }

    return ss.str();
  }

  std::string Run::str_compared(const Run& other, output_format form) const
  {
    using std::endl;
    tabulate::Table paired = make_combined_table(other);
    tablef::format_base(paired);

    std::stringstream ss;
    switch (form)
    {
      case output_format::string:
        ss << fmt::format("Experiment: {}", model) << endl << paired;
        break;
      case output_format::latex:
        ss << tabulate::LatexExporter().dump(paired) << endl;
        break;
      case output_format::markdown:
        ss << tabulate::MarkdownExporter().dump(paired) << endl;
        break;
    }
    return ss.str();
  }

  void Run::dump(tabulate::Exporter& exporter, std::ostream& out) const
  {
    using std::endl;

    for (size_t i{ 0 }; i < results.size(); i++)
    {
      out << fmt::format("### Sample {}", i) << endl;
      tabulate::Table t{ results[i]->summary_table() };
      tablef::format_base(t);
      out << exporter.dump(t) << endl << endl;

      out << "#### Traces" << endl;
      out << results[i]->all_traces() << endl;
    }
  }

  // EXPERIMENT PUBLIC MEMBERS
  //
  Experiment::Experiment(
      my::cli::ArgumentList const& a, Logger& l)
      : args(a), model(model_t::get_name(args.model)),
        type(algo::get_name(args.algorithm)), log(l),
        N_reps(args.experiment->repetitions), seeds(N_reps)
  {
    if (auto ipdr = my::variant::get_cref<algo::t_IPDR>(args.algorithm))
      tactic = ipdr->get().type;

    sample_table.add_row(pebbling::result::summary_header);
    sample_table.format()
        .font_align(tabulate::FontAlign::right)
        .hide_border_top()
        .hide_border_bottom();
    control_table.add_row(pebbling::result::summary_header);
    control_table.format() = sample_table.format();

    srand(time(0));
    std::generate(seeds.begin(), seeds.end(), rand);
  }

  void Experiment::run()
  {
    using fmt::format;
    using std::endl;

    // TODO: haal reps uit Run
    std::ofstream latex = args.folders.file_in_run("tex");
    std::ofstream raw   = args.folders.file_in_run("md");

    std::cout << type + " run." << endl;

    std::shared_ptr<Run> aggregate = single_run(false);
    latex << aggregate->str(output_format::latex);

    std::cout << "control run." << endl;
    std::shared_ptr<Run> control_aggregate = single_run(true);
    latex << control_aggregate->str(output_format::latex);

    // write raw run data as markdown
    tabulate::MarkdownExporter exporter;
    raw << format("# {}. {} samples. {} tactic.", model, N_reps,
               tactic::to_string(tactic))
        << endl;

    raw << "## Experiment run." << endl;
    aggregate->dump(exporter, raw);

    raw << "## Control run." << endl;
    control_aggregate->dump(exporter, raw);
  }
} // namespace pdr::experiments
