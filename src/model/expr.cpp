#include "expr.h"
#include "z3-ext.h"
#include <cstdlib>
#include <optional>
#include <regex>

namespace mysat::primed
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::mk_and;

  vector<string> extract_names(const expr_vector& v)
  {
    vector<string> rv;
    rv.reserve(v.size());
    std::transform(v.begin(), v.end(), std::back_inserter(rv),
        [](expr l) { return l.to_string(); });
    return rv;
  }

  // Lit
  //
  Lit::Lit(z3::context& c, const string& n) : IPrimed<expr>(c, n)
  {
    current = ctx.bool_const(name.c_str());
    next    = ctx.bool_const(next_name.c_str());
  }

  Lit::operator const expr&() const { return current; }
  const expr& Lit::operator()() const { return current; }
  const expr& Lit::p() const { return next; }

  expr Lit::unchanged() const { return current == next; }

  vector<string> Lit::names() const { return { current.to_string() }; }

  bool Lit::extract_value(const z3::expr_vector& cube, lit_type t) const
  {
    // matches name[i], extracts name and i
    std::regex is_value;
    std::smatch match;
    string match_name;
    if (t == primed)
    {
      // is_value = (R"(([[:alnum:]_]+).p)"); // primed variable
      is_value   = next_name;
      match_name = next_name;
    }
    else
    {
      // is_value = (R"(([[:alnum:]_]+))");
      is_value   = fmt::format("{}(?!.p)", name); // name not followed by .p
      match_name = name;
    }

    std::optional<bool> v;

    for (const expr& l : cube)
    {
      assert(z3ext::is_lit(l));
      string l_str = l.to_string();
      // if (l_str.find(match_name) != string::npos)
      if (std::regex_search(l_str, match, is_value))
      {
        assert(match.size() == 1);
        // if (match[1] == match_name)
        // {
        // no contradicting literals
        assert(not v.has_value() || (v == not l.is_not()));
        v = not l.is_not();
        // }
      }
    }

    return v.value();
  }

  // VarVec
  // public
  VarVec::VarVec(z3::context& c, const std::set<string> names)
      : IPrimed<expr_vector>(c, "varvec")
  {
    for (const string& name : names)
    {
      const expr new_curr = ctx.bool_const(name.c_str());
      const expr new_next = ctx.bool_const(prime(name).c_str());

      current.push_back(new_curr);
      next.push_back(new_next);

      to_current.emplace(new_next, new_curr);
      to_next.emplace(new_curr, new_next);
    }
  }

  VarVec::operator const expr_vector&() const { return current; }
  const expr_vector& VarVec::operator()() const { return current; }
  const expr_vector& VarVec::p() const { return next; }
  vector<string> VarVec::names() const { return extract_names(current); }

  expr VarVec::operator()(size_t i) const
  {
    assert(i < current.size());
    return current[i];
  }
  expr VarVec::p(size_t i) const
  {
    assert(i < next.size());
    return next[i];
  }

  expr VarVec::operator()(const expr& e) const
  {
    if (e.is_not())
    {
      assert(e.arg(0).is_const());
      return !to_current.at(e.arg(0));
    }
    assert(e.arg(0).is_const());
    return to_current.at(e);
  }
  expr VarVec::p(const expr& e) const
  {
    if (e.is_not())
    {
      assert(e.arg(0).is_const());
      return !to_next.at(e.arg(0));
    }
    assert(e.arg(0).is_const());
    return to_next.at(e);
  }

  expr_vector VarVec::operator()(const expr_vector& ev) const
  {
    expr_vector rv(ctx);
    for (expr e : ev)
      rv.push_back(e.substitute(next, current));
    return rv;
  }
  expr_vector VarVec::p(const expr_vector& ev) const
  {
    expr_vector rv(ctx);
    for (expr e : ev)
      rv.push_back(e.substitute(current, next));
    return rv;
  }

  bool VarVec::lit_is_current(const z3::expr& e) const
  {
    return to_next.find(e) != to_next.end();
  }

  bool VarVec::lit_is_p(const z3::expr& e) const
  {
    return to_current.find(e) != to_current.end();
  }

  // ExpVec
  // public

  ExpVec::ExpVec(z3::context& c, const VarVec& v)
      : IPrimed<expr_vector>(c, "expvec"), vars(v)
  {
  }

  ExpVec::operator const expr_vector&() const { return current; }
  const expr_vector& ExpVec::operator()() const { return current; }
  const expr_vector& ExpVec::p() const { return next; }

  vector<string> ExpVec::names() const { return extract_names(current); }

  ExpVec& ExpVec::add(z3::expr e)
  {
    assert(!finished);

    current.push_back(e);
    z3::expr e_p = e.substitute(vars(), vars.p());
    next.push_back(e_p);

    return *this;
  }

  void ExpVec::finish() { finished = true; }

  // BitVec
  // public
  BitVec::BitVec(z3::context& c, const string& n, size_t Nbits)
      : IPrimed<expr_vector>(c, n), size(Nbits)
  {
    for (size_t i = 0; i < size; i++)
    {
      current.push_back(ctx.bool_const(index_str(name, i).c_str()));
      next.push_back(ctx.bool_const(index_str(next_name, i).c_str()));
    }
  }

  BitVec BitVec::holding(z3::context& c, const string& n, numrep_t max_val)
  {
    return BitVec(c, n, std::log2(max_val) + 1);
  }

  string BitVec::index_str(std::string_view n, size_t i) const
  {
    return fmt::format("{}[{}]", n, i);
  }

  BitVec::operator const expr_vector&() const { return current; }
  const expr_vector& BitVec::operator()() const { return current; }
  const expr_vector& BitVec::p() const { return next; }

  expr BitVec::operator()(size_t i) const { return current[i]; }
  expr BitVec::p(size_t i) const { return next[i]; }

  vector<string> BitVec::names() const { return extract_names(current); }

  expr_vector BitVec::uint(numrep_t n) const { return unint_to_lits(n, false); }
  expr_vector BitVec::uint_p(numrep_t n) const
  {
    return unint_to_lits(n, true);
  }

  BitVec::numrep_t BitVec::extract_value(
      const expr_vector& cube, lit_type t) const
  {
    std::bitset<MAX_BITS> n;
    // matches name[i], extracts name and i
    std::regex is_value;
    std::smatch match;
    string match_name;
    if (t == primed)
    {
      // is_value = R"(([[:alnum:]_]+).p\[([[:digit:]]+)\])"; // primed variable
      is_value   = fmt::format("{}\\[([[:digit:]]+)\\]", next_name);
      match_name = next_name;
    }
    else
    {
      // is_value = R"(([[:alnum:]_]+)\[([[:digit:]]+)\])";
      is_value   = fmt::format("{}\\[([[:digit:]]+)\\]", name);
      match_name = name;
    }

    for (const expr& l : cube)
    {
      assert(z3ext::is_lit(l));
      string l_str = l.to_string();
      if (std::regex_search(l_str, match, is_value))
      {
        assert(match.size() == 2);
        // if (match[1] != match_name)
        //   continue;

        numrep_t i = std::stoul(match[1]);

        n.set(i, !l.is_not());
      }
    }

    return n.to_ulong();
  }

  expr BitVec::equals(numrep_t n) const { return mk_and(uint(n)); }
  expr BitVec::p_equals(numrep_t n) const { return mk_and(uint_p(n)); }

  expr BitVec::unchanged() const
  {
    expr_vector conj(ctx);
    for (size_t i = 0; i < size; i++)
      conj.push_back(current[i] == next[i]);

    return mk_and(conj);
  }

  // private
  expr_vector BitVec::unint_to_lits(numrep_t n, bool primed) const
  {
    expr_vector rv(ctx), reversed(ctx);
    std::bitset<MAX_BITS> bits(n);
    assert(size <= bits.size());

    for (size_t i = 0; i < current.size(); i++)
    {
      if (n & 1) // first bit is 1
        reversed.push_back(primed ? next[i] : current[i]);
      else
        reversed.push_back(primed ? !next[i] : !current[i]);

      n >>= 1;
    }

    for (int i = reversed.size() - 1; i >= 0; i--)
      rv.push_back(reversed[i]);

    return rv;
  }

  expr BitVec::less_4b(numrep_t n, size_t msb) const
  {
    assert(msb < INT_MAX);
    assert((msb + 1) % 4 == 0);
    size_t lsb = msb - 3;

    const std::bitset<MAX_BITS> bits(n);
    assert(size <= bits.size());

    std::bitset<MAX_BITS> relevant(0);
    for (size_t i = lsb; i <= msb; i++)
      relevant.set(i, bits.test(i));

    if (relevant.to_ulong() == 0)
      return ctx.bool_val(false); // unsigned, so impossible

    auto set = [this, &bits](size_t i) { return ctx.bool_val(bits.test(i)); };

    expr_vector disj(ctx);
    for (int i = msb; i >= (int)lsb; i--)
    {
      expr bit = i < (int)size ? current[i] : ctx.bool_val(false);
      expr e   = !bit && set(i);
      for (size_t j = i + 1; j <= msb; j++)
      {
        // handle if BitVec does not store i+1 bits
        expr bit = j < size ? current[j] : ctx.bool_val(false);
        e        = !(bit ^ set(j)) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  expr BitVec::less_4b(const BitVec& cube, size_t msb) const
  {
    assert(size == cube.size);
    assert(msb < INT_MAX);
    assert((msb + 1) % 4 == 0);
    size_t lsb = msb - 3;

    expr_vector disj(ctx);
    for (int i = msb; i >= (int)lsb; i--)
    {
      expr bit      = i < (int)size ? current[i] : ctx.bool_val(false);
      expr cube_bit = i < (int)size ? cube(i) : ctx.bool_val(false);
      expr e        = !bit && cube_bit;
      for (size_t j = i + 1; j <= msb; j++)
      {
        // handle if BitVec does not store i+1 bits
        expr bit      = j < size ? current[j] : ctx.bool_val(false);
        expr cube_bit = j < size ? cube(j) : ctx.bool_val(false);
        e             = !(bit ^ cube_bit) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  z3::expr BitVec::eq(numrep_t n, size_t msb, size_t nbits) const
  {
    assert((msb + 1) % 4 == 0);
    assert((msb + 1) >= nbits);
    size_t lsb = msb - (nbits - 1);

    const std::bitset<MAX_BITS> bits(n);

    z3::expr_vector conj(ctx);
    for (size_t i = lsb; i <= msb; i++)
    {
      expr bit = i < size ? current[i] : ctx.bool_val(false);
      if (bits.test(i))
        conj.push_back(bit);
      else
        conj.push_back(!bit);
    }

    return z3::mk_and(conj);
  }

  z3::expr BitVec::eq(const BitVec& cube, size_t msb, size_t nbits) const
  {
    assert(size == cube.size);
    assert((msb + 1) % 4 == 0);
    assert((msb + 1) >= nbits);
    size_t lsb = msb - (nbits - 1);

    z3::expr_vector conj(ctx);
    for (size_t i = lsb; i <= msb; i++)
    {
      expr bit      = i < size ? current[i] : ctx.bool_val(false);
      expr cube_bit = i < size ? cube(i) : ctx.bool_val(false);
      conj.push_back(!(cube_bit ^ bit)); // n[i] <=> current[i]
    }

    return z3::mk_and(conj);
  }
} // namespace mysat::primed
