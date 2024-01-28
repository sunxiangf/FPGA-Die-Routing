#pragma once

#include "database.hpp"
#include "dijkstra.hpp"

void THTrouting(Database& db);

class THTGraph {
public:
  THTGraph(Database &db);  // 构造函数，使用数据库初始化图

  void dijkstra_die();    // 运行Dijkstra算法

  void init2(Database &db);     //第二步初始化

  void dijkstra2(Database &db,Net& net);  //第二步dijkstra

  int dijkstra3(Database &db,Net& net);   //第三部dijkstra

  double add_rw(Database &db,int start, int end);

  bool dis_rerouting(Database db,Database &db_old,Net &net,vector<Net*>net_max_rw);//重跑dijkstra重分配函数

  bool FPGAwire_rerouting(Database db,Database &db_old, Net &net,vector<Net*>net_max_rw);//调整FPGA线路重分配函数

  void removeNetFromWire(Wire *wire, int net_id, int fpga_num);

  void clearOldPaths(Database &db, int net_id);

  void find_wire_from_die(Database &db,int start,int end,Wire* &wire,int &fpga_num);

  void adjustTDMRadio(Database &db);
  
  void adjustnetrw(Database &db);

  // vector<int> shortest_path(int start, int end);  // 计算从起点到终点的最短路径

  vector<vector<double>> dists;  // 存储所有节点对之间的最短距离  

  map<pair<int,int>, int> more_node_map; // 记录关联源点之间的关系。记录die1和die2之间的wire线路，后面存储初始节点V
  map<int, pair<int, int>> more_node_map_V;  // 记录关联源点之间的关系。反过来存一遍

  double max_max_rw = -1;
  vector<Net*>net_max_rw;

private:
  int V; // 总节点数量
  vector<vector<int>> predecessors; // 存储前驱节点信息 
  vector<list<pair<int, double>>> adjList; // 邻接表表示图，代表第i个die有k个分别通往不同pair1的、权重为pair2的路径
  vector<pair<int, int>> net_need_to_move; // 所有可以移动的线路，前权值后netid
  map<int, int> net_map;                   // 存储从id->编号的map
  
  // 添加边到图中
  void add_edge(int src, int dest, double weight);
  bool add_net_path_dis3(int start, int end, int net_num, Database &db);
  void add_net_path(int start, int end, int net_num, Database &db);
};

void export_routing_result(const Database& db, const std::string& filename);
void export_tdm_result(const Database& db, const std::string& filename);