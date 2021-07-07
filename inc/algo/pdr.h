#ifndef PDR_ALG
#define PDR_ALG

#include <queue>
#include <z3++.h>
#include <memory>
#include <vector>

#include "frame.h"
#include "pdr-model.h"

using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using z3::context;
using z3::solver;

struct Obligation
{
	int level;
	expr_vector cube;

	bool operator<(const Obligation& o) { return this->level < o.level; }
};

class PDR 
{
	private:
		shared_ptr<context> ctx;
		const PDRModel& model;

		vector<unique_ptr<Frame>> frames;
		solver init_solver;

		Frame* make_frame(int level);
		void print_model(const z3::model& m);
		bool init();
		bool iterate();
		bool block(std::priority_queue<Obligation> obligations, unsigned level);
		void remove_state(expr_vector& smaller_cti, int level);
		bool propagate(unsigned level);

		int highest_inductive_frame(const expr_vector& cube, int min, int max);
		expr_vector generalize(const expr_vector& cube, int level);

	public:
		PDR(shared_ptr<context> c, const PDRModel& m);
		void run();
};

#endif //PDR_ALG