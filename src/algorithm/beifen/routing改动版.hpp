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

using namespace std;

// net_weight 类型定义为一个包含整数对的向量
typedef vector<pair<int, int>> net_weight;

// routing_result 结构体，用于存储路径和权重信息
struct routing_result {
  vector<vector<int>> path; // 存储路径
  vector<int> weight;            // 存储路径权重
};

// wire 结构体，用于存储线路信息
struct wire {
  unsigned int wire_num;             // 线路编号
  bool inter_fpga;                   // 是否跨越FPGA
  pair<int, int> interval;      // 连接的端点
  set<int> wire1;               // low -high 线路集合
  set<int> wire2;               // high -low 线路集合
  vector<set<int>> wires;  // 存储线路集合
  vector<int> tdm;              // 存储时间分配多路复用（TDM）信息
};

// router 类
class router {
public:
  router(database &db); // 构造函数

  vector<routing_result> result1;        // 存储路径规划结果
  vector<wire> result2;                  // 存储线路信息
  map<pair<int, int>, int> result_map; // 结果映射

  void route();   // 线路分配函数
  void test();    // 测试函数

private:
  database &db;    // 数据库引用
  net_weight weights; // 网络权重
  int get_weight1(int, int); // 获取权重的函数
  void net_order();         // 网络排序函数
  void routing();           // 路由函数
  void tdm_assignment();    // TDM分配函数
  void reroute();           // 重新路由函数
  void update_result2(vector<int> &path, int net); // 更新result2函数
};
