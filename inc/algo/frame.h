#ifndef FRAME
#define FRAME

#include "logger.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <algorithm>
#include <fmt/format.h>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
  class Frame
  {
   private:
    // the arguments of the clause are sorted by mic, use id to search
    z3ext::CubeSet blocked_cubes;
    const unsigned level;

   public:
    Frame(unsigned i);

    void clear();

    // remove any weaker cubes in the frame
    unsigned remove_subsumed(
        const std::vector<z3::expr>& cube, bool remove_equal);
    // remove any weaker cubes in the frame, specialized for constrained cubes.
    // slower than regular, used during relaxation phase
    unsigned remove_subsumed_constrained(
        std::map<unsigned, size_t> const& order,
        const std::vector<z3::expr>& cube,
        bool remove_equal);

    // check if a stronger cube has already been blocked
    bool is_subsumed(std::vector<z3::expr> const& cube) const;
    bool block(std::vector<z3::expr> const& cube);

    // Frame comparisons
    bool equals(const Frame& f) const;
    std::vector<std::vector<z3::expr>> diff(const Frame& f) const;

    // getters
    z3ext::CubeSet const& get() const;
    bool empty() const;

    // string representations
    std::string blocked_str() const;
  };
} // namespace pdr

#endif // FRAME
