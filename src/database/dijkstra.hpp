#pragma once

#include "database.hpp"
#include "routing.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <queue>
#include <vector>


// Input:  die_num1 die_num2
// Output: routing result

#include <iostream>
#include <limits>
#include <list>
#include <queue>
#include <vector>

using namespace std;

class Graph {
public:
  // 构造函数，使用数据库初始化图
  Graph(database &db);

  // 运行Dijkstra算法
  void dijkstra();

  // 测试函数
  void test();

  // 计算从起点到终点的最短路径
  vector<int> shortest_path(int start, int end);

  // 存储所有节点对之间的最短距离  /*无用的总存*/
  vector<vector<int>> dists;

private:
  int V; // 总节点数量
  vector<vector<int>> predecessors; // 存储前驱节点信息 
  vector<list<pair<int, int>>> adjList; // 邻接表表示图，代表第i个die有k个分别通往不同pair1的、权重为pair2的路径

  // 添加边到图中
  void add_edge(int src, int dest, int weight);

};



