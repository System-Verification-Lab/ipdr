#include "pdr-context.h"
#include "tactic.h"
#include "types-ext.h"

#include <iostream>
#include <memory>
#include <variant>

#define MIC_RETRIES_DEFAULT UINT_MAX

namespace pdr
{

  Context::Context(z3::context& c, my::cli::ArgumentList const& args)
      : z3_ctx(c),
        type(Tactic::undef),
        mic_retries(args.mic_retries.value_or(MIC_RETRIES_DEFAULT))
  {
    using namespace my::variant;
    z3_ctx.set("unsat_core", true);
#warning TODO sat.core.minimize
    z3_ctx.set("sat.core.minimize", true);
    // z3_ctx.set("sat.core.minimize_partial", true);
    z3_ctx.set("model", true);

    // clang-format off
    seed = std::visit(visitor{ 
        [](bool r) -> unsigned
        {
          if (r)
            {
              srand(time(0));
              return rand();
            }
            return 0u;
        },
        [](unsigned s) -> unsigned { return s; } },
      args.r_seed);
    // clang-format on
    std::cout << "z3 random seed: " << seed << std::endl;
  }

  Context::Context(
      z3::context& c, my::cli::ArgumentList const& args, unsigned s)
      : z3_ctx(c),
        seed(s),
        type(Tactic::undef),
        mic_retries(args.mic_retries.value_or(MIC_RETRIES_DEFAULT))
  {
    std::cout << "z3 random seed: " << seed << std::endl;
  }

  Context::operator z3::context&() { return z3_ctx; }
  Context::operator const z3::context&() const { return z3_ctx; }

  z3::context& Context::operator()() { return z3_ctx; }
} // namespace pdr