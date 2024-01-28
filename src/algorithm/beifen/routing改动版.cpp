#include "routing.hpp"
#include <chrono>

using namespace std;

router::router(database &db) : db(db)
{
  result1.resize(db.nets.size());
  for (const auto &connect : db.die_network)
  {
    result2.emplace_back();
    result2.back().inter_fpga = connect.second > 1000 ? false : true;
    result2.back().interval = connect.first;
    result2.back().wire_num = connect.second;

    // result_map = db.die_network;
    // cout<<"size: "<<result_map.size()<<endl;
    result_map.emplace(connect.first, result2.size() - 1);
    //
  }
}

int router::get_weight1(int fpga_num1, int fpga_num2)
{
  if (fpga_num1 == fpga_num2)
  {
    return 0;
  }
  else
  {
    return 100;
  }
  // auto the_pair = fpga_num1 < fpga_num2 ? make_pair(fpga_num1,
  // fpga_num2)
  //                                       : make_pair(fpga_num2,
  //                                       fpga_num1);
  //   if (db.fpga_connection.find(the_pair) == db.fpga_connection.end()) {
  //     return 0;
  //   }
  //   int source = db.fpga_connection.at(the_pair);
  //   return source*10;
}
// 对net的重要性根据是否跨FPGA和跨越Die的数量来判断
void router::net_order()
{
  for (const auto &net : db.nets)
  {
    int weight = 0;
    int fpga_num1 = net.fpga_num;
    for (const auto &net2 : net.net_connected)
    {
      int fpga_num2 = db.get_net(net2).fpga_num;
      int weight1 = get_weight1(fpga_num1, fpga_num2);
      weight += weight1;
    }
    int weight2 = net.net_connected.size();
    weight += weight2;
    weights.emplace_back(make_pair(weight, net.net_num));
  }
  sort(weights.begin(), weights.end(),
            [](const auto &a, const auto &b)
            { return a.first > b.first; });
  // for (auto weight : weights) {
  //   cout << weight.first << " " << weight.second << endl;
  // }
}

// 调用dijkstra算法，计算每个net的最短路径进行布线
void router::routing()
{
  Graph graph(db);
  graph.dijkstra(*this);
  // graph.test();
  for (const auto &weight : weights)
  {
    int net1 = weight.second;
    for (const auto &net2 : db.nets.at(net1).net_connected)
    {
      int start_die = db.nets.at(net1).die_num;
      int end_die = db.get_net(net2).die_num;
      if (start_die == end_die)
      {
        cout << db.nets.at(net1).name << " " << db.get_net(net2).name
                  << endl;
      }
      if (start_die != end_die)
      {
        // cout << start_die << " " << end_die << endl;
        // cout << "最短距离： " << graph.dists[start_die][end_die]
        //           << endl;
        auto path = graph.shortest_path(start_die, end_die);
        result1.at(net1).path.emplace_back(path);
        update_result2(path, net1);
      }
    }
  }
}

// 更新布线结果
void router::update_result2(vector<int> &path, int net)
{
  for (unsigned int i = 0; i < path.size() - 1; i++)
  {
    if (path[i] < path[i + 1])
    {
      auto pair = make_pair(path[i], path[i + 1]);
      auto result_num = result_map.at(pair);
      result2.at(result_num).wire1.emplace(net);
    }
    else
    {
      auto pair = make_pair(path[i + 1], path[i]);
      auto result_num = result_map.at(pair);
      result2.at(result_num).wire2.emplace(net);
    }
  }
}

// 对于跨越Die的net，进行TDM分配，求其TDM radio（4的倍数）
void assignment(wire &the_wire)
{
  int k1 = 4;
  int k2 = 4;

  // Calculate wire sizes and line numbers
  double wire1Size = the_wire.wire1.size();
  double wire2Size = the_wire.wire2.size();
  unsigned int line_num = ceil(wire1Size / k1) + ceil(wire2Size / k2);

  // Find the optimal k1 and k2 values
  while (line_num > the_wire.wire_num)
  {
    if (k1 == k2)
    {
      k1 += 4;
    }
    else
    {
      k2 += 4;
    }
    line_num = ceil(wire1Size / k1) + ceil(wire2Size / k2);
  }

  // Resize wires and tdm vectors
  the_wire.wires.resize(line_num);
  the_wire.tdm.reserve(line_num);

  // Initialize wire index variables for both wire1 and wire2
  unsigned wireIndex1 = 0;
  unsigned wireIndex2 = wire1Size / k1;

  // Fill the wires vectors
  for (auto start = the_wire.wire1.begin(); start != the_wire.wire1.end();
       ++start)
  {
    the_wire.wires[wireIndex1].emplace(*start);
    if (++wireIndex1 == (unsigned int)(wire1Size / k1))
    {
      wireIndex1 = 0;
    }
  }

  for (auto start = the_wire.wire2.begin(); start != the_wire.wire2.end();
       ++start)
  {
    the_wire.wires[wireIndex2].emplace(*start);
    if (++wireIndex2 == line_num)
    {
      wireIndex2 = wire1Size / k1;
    }
  }

  // Fill the tdm vector
  for (int ii = 0; ii < ceil(wire1Size / k1) + ceil(wire2Size / k2); ++ii)
  {
    if (ii < ceil(wire1Size / k1))
    {
      the_wire.tdm.emplace_back(k1);
    }
    else
    {
      the_wire.tdm.emplace_back(k2);
    }
  }
}
// 判断一个Die是否跨FPGA，若跨则调用assignment函数进行TDM分配
void router::tdm_assignment()
{
  for (auto &the_wire : result2)
  {
    if (the_wire.inter_fpga == false)
    {
      //
      auto is_enough =
          the_wire.wire_num > the_wire.wire1.size() + the_wire.wire2.size();
      if (!is_enough)
      {
        cout << "resource is not enough" << endl;
      }
    }
    else
    {
      assignment(the_wire);
    }
  }
}

// 测试函数，打印中间结果
void router::test()
{
  for (auto &rslt : result2)
  {
    cout << rslt.interval.first << " " << rslt.interval.second << " "
              << rslt.wire_num << " " << rslt.inter_fpga << endl;
    for (auto &wire : rslt.wire1)
    {
      cout << "Wire1: " << wire << endl;
    }
    for (auto &wire : rslt.wire2)
    {
      cout << "Wire2: " << wire << endl;
    }
    for (auto &wire : rslt.wires)
    {
      cout << "Nets:";
      for (auto &net : wire)
      {
        cout << net << " ";
      }
      cout << endl;
    }
    for (auto &wire_tdm : rslt.tdm)
    {
      cout << wire_tdm << endl;
    }
  }
  for (auto &map : result_map)
  {
    cout << map.first.first << " " << map.first.second << " " << map.second
              << endl;
  }
}

void router::route()
{

  net_order();
  routing();

  tdm_assignment();

  test();
}
