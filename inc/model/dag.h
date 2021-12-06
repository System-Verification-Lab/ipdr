#ifndef DAG_H
#define DAG_H

#include "graphvizgraph.h"
#include "string-ext.h"

#include <cassert>
#include <fmt/format.h>
#include <ghc/filesystem.hpp>
#include <graphviz/gvc.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace dag
{
  struct Edge
  {
    std::string from;
    std::string to;

    Edge(const std::string& f, const std::string& t) : from(f), to(t) {}

    friend bool operator<(const Edge& lhs, const Edge& rhs)
    {
      if (lhs.from < rhs.from)
        return true;
      if (rhs.from < lhs.from)
        return false;

      if (lhs.to < rhs.to)
        return true;
      return false;
    }

    friend std::stringstream& operator<<(std::stringstream& stream,
                                         Edge const& e)
    {
      stream << "(" << e.from << ", " << e.to << ")";
      return stream;
    }
  };

  class Graph
  {
   private:
    std::map<std::string, std::vector<std::string>> children; // nodes X nodes
    std::set<Edge> input_edges;                               // nodes X nodes
    std::vector<std::string> empty_vec;
    std::unique_ptr<graphviz::Graph> image;

   public:
    std::string name;
    std::string prefix = "";

    std::set<std::string> input;
    std::set<std::string> nodes;
    std::set<std::string> output; // subet of nodes
    std::set<Edge> edges;         // nodes X nodes

    std::string node(std::string name) { return prefix + name; }

    Graph(const std::string& s) : name(s) {}
    Graph(const std::string& s, const std::string& dot)
        : name(s), image(std::make_unique<graphviz::Graph>(dot))
    {
    }

    void add_input(std::string name) { input.insert(node(name)); }

    void add_node(std::string name) { nodes.insert(node(name)); }

    void add_output(std::string name)
    {
      nodes.insert(node(name));
      output.insert(node(name));
    }

    void add_edges_to(std::vector<std::string> from, std::string to)
    {
      if (from.empty())
        return;

      to = node(to);
      assert(nodes.find(to) != nodes.end());

      std::vector<std::string> to_children;
      to_children.reserve(from.size());

      for (std::string i : from)
      {
        std::string n = node(i);
        if (input.find(n) != input.end())
        {
          input_edges.emplace(n, to);
          continue;
        }

        assert(nodes.find(n) != nodes.end());
        edges.emplace(n, to);
        to_children.push_back(n);
      }

      children.emplace(to, std::move(to_children));
    }

    std::string summary() const
    {
      return fmt::format("Graph {{ In: {}, Out {}, Nodes {} }}", input.size(),
                         output.size(), nodes.size());
    }

    friend std::ostream& operator<<(std::ostream& stream, Graph const& g)
    {
      stream << "DAG \{" << std::endl
             << "\tinput { " << str::extend::join(g.input) << " }" << std::endl
             << "\toutput { " << str::extend::join(g.output) << " }"
             << std::endl
             << "\tnodes { " << str::extend::join(g.nodes) << " }" << std::endl
             << "\tedges { " << str::extend::join(g.edges) << " }" << std::endl
             << "}" << std::endl;
      return stream;
    }

    void show_image(const std::string& destination)
    {
      if (!image)
        image = std::make_unique<graphviz::Graph>(dot());
      image->render(destination);
    }

    std::string dot()
    {
      std::stringstream ss;
      ss << "digraph G {" << std::endl;

      for (const Edge& e : input_edges)
        ss << fmt::format("{} -> {};", e.from, e.to) << std::endl;
      for (const Edge& e : edges)
        ss << fmt::format("{} -> {};", e.from, e.to) << std::endl;

      for (const std::string& o : input)
        ss << fmt::format("{} [shape=plain];", o) << std::endl;
      for (const std::string& o : output)
        ss << fmt::format("{} [shape=doublecircle];", o) << std::endl;

      ss << "}" << std::endl;
      return ss.str();
    }

    bool is_output(const std::string& name) const
    {
      return output.find(name) != output.end();
    }

    const std::vector<std::string>& get_children(const std::string& key) const
    {
      auto result = children.find(key);

      if (result == children.end())
        return empty_vec;

      return result->second;
    }
  };
} // namespace dag

#endif // DAG_H
