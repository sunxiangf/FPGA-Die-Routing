#pragma once

#include "../database/database.hpp"
#include "../database/dijkstra.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

typedef std::vector<std::pair<int, int>> net_weight;
// [3]
// [1,2,5,6][50]
// [1,2,3][48]
//表示id为3的net，从driver到达第一个loader的布线路径为Die1,2,5,6，routing weight为50; 从driver到达第二个loader的布线路径为Die1,2,3，routing weight为48。
struct routing_result {
  std::vector<std::vector<int>> path;
  std::vector<int> weight;
};

struct wire {
  unsigned int wire_num;
  bool inter_fpga;//是否跨越FPGA
  std::pair<int, int> interval;//连接的端点
  std::set<int> wire1; // low -high std::pair<int, int>
  std::set<int> wire2; // high -low std::pair<int, int>

  std::vector<std::set<int>> wires; //
  std::vector<int> tdm;
};

class router {
public:
  router(database &db);

  std::vector<routing_result> result1;
  std::vector<wire> result2;
  std::map<std::pair<int, int>, int> result_map;

  void route();
  void test();

private:
  database &db;
  net_weight weights;
  int get_weight1(int, int);
  void net_order();
  void routing();
  void tdm_assignment();
  void reroute();
  void update_result2(std::vector<int> &path, int net);
};
