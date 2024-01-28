#include "routing.hpp"
#include <chrono>
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
    // std::cout<<"size: "<<result_map.size()<<std::endl;
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
  // auto the_pair = fpga_num1 < fpga_num2 ? std::make_pair(fpga_num1,
  // fpga_num2)
  //                                       : std::make_pair(fpga_num2,
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
    weights.emplace_back(std::make_pair(weight, net.net_num));
  }
  std::sort(weights.begin(), weights.end(),
            [](const auto &a, const auto &b)
            { return a.first > b.first; });
  // for (auto weight : weights) {
  //   std::cout << weight.first << " " << weight.second << std::endl;
  // }
}

// 调用dijkstra算法，计算每个net的最短路径进行布线
void router::routing()
{
  Graph graph(db);
  graph.dijkstra();
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
        std::cout << db.nets.at(net1).name << " " << db.get_net(net2).name
                  << std::endl;
      }
      if (start_die != end_die)
      {
        // std::cout << start_die << " " << end_die << std::endl;
        // std::cout << "最短距离： " << graph.dists[start_die][end_die]
        //           << std::endl;
        auto path = graph.shortest_path(start_die, end_die);
        result1.at(net1).path.emplace_back(path);
        update_result2(path, net1);
      }
    }
  }
}

// 更新布线结果
void router::update_result2(std::vector<int> &path, int net)
{
  for (unsigned int i = 0; i < path.size() - 1; i++)
  {
    if (path[i] < path[i + 1])
    {
      auto pair = std::make_pair(path[i], path[i + 1]);
      auto result_num = result_map.at(pair);
      result2.at(result_num).wire1.emplace(net);
    }
    else
    {
      auto pair = std::make_pair(path[i + 1], path[i]);
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
        std::cout << "resource is not enough" << std::endl;
      }
    }
    else
    {
      assignment(the_wire);
    }
  }
}
// 进行重布线优化布线的结果，目前主要思路为：
// 1.计算每个net的routeweight
// 2.找到最大和最小的routeweight，更新max_weight和min_weight
// 3.对于max_weight的net，进行重布线
// 4.重复1-3步骤，直到max_weight和min_weight的差值小于2（因为评价标准是所有net的routeweight最小，因此理论上上最大routeweight和最小routeweight近似相等时，最大routeweight相等）

void router::reroute()
{
  int max_weight = 10;
  int min_weight = 0;
  while (max_weight - min_weight > 2)
  {
    // 计算所有网络的routeweight
    for (auto &result : result1)
    {
      result.weight.resize(result.path.size());
      for (auto &path1 : result.path)
      {
        auto it = std::find(result.path.begin(), result.path.end(), path1);
        int index = std::distance(result.path.begin(), it);
        for (unsigned int i = 0; i < path1.size() - 1; i++)
        {
          int die1 = path1[i];
          int die2 = path1[i + 1];
          if (die1 < die2)
          {
            if (db.dies.at(die1).fpga_num == db.dies.at(die2).fpga_num)
            {
              result.weight[index] += 1;
            }
            else
            {
              auto key = std::make_pair(die1, die2);
              if (result_map.count(key) > 0)
              {
                int delay = result2.at(result_map.at(key)).tdm.front();
                result.weight[index] += 0.5 * (2 * delay + 1);
              }
              else
              {
                std::cout << "continue1" << std::endl;
                continue; // 跳过该次循环
              }
            }
          }
          else
          {
            if (db.dies.at(die1).fpga_num == db.dies.at(die2).fpga_num)
            {
              result.weight[index] += 1;
            }
            else
            {

              auto key = std::make_pair(die1, die2);
              if (result_map.count(key) > 0)
              {
                int delay = result2.at(result_map.at(key)).tdm.back();
                result.weight[index] += 0.5 * (2 * delay + 1);
              }
              else
              {
                std::cout << "continue2" << std::endl;
                for (auto it = result_map.begin(); it != result_map.end(); ++it)
                {
                  // 打印键和值

                  std::cout << "die1: " << die1 << ", die2: " << die2 << std::endl;
                  std::cout << "Key: " << it->first.first << "," << it->first.second << " "
                                                                                        ", Value: "
                            << it->second << std::endl;
                }

                continue;
              }
            }
          }
        }
      }
    }
    // 查找最大和最小的routeweight，更新max_weight和min_weight
    int max_weight_net = 0;
    int max_weight2 = std::numeric_limits<int>::min(); // 初始值设置为最小值
    for (unsigned int i = 0; i < result1.size(); i++)
    {
      auto maxElement = std::max_element(result1.at(i).weight.begin(), result1.at(i).weight.end());
      if (maxElement != result1.at(i).weight.end() && *maxElement > max_weight2)
      {
        max_weight2 = *maxElement;
        max_weight_net = i;
      }
    }

    int min_weight_net = 0;
    int min_weight2 = std::numeric_limits<int>::max(); // 初始值设置为最大值
    for (unsigned i = 0; i < result1.size(); i++)
    {
      auto minElement = std::min_element(result1.at(i).weight.begin(), result1.at(i).weight.end());
      if (minElement != result1.at(i).weight.end() && *minElement < min_weight2)
      {
        min_weight2 = *minElement;
        min_weight_net = i;
      }
    }

    auto it = std::stable_partition(weights.begin(), weights.end(), [max_weight_net](const std::pair<int, int> &p)
                                    { return p.second == max_weight_net; });
    routing();
    tdm_assignment();

    // 更新max_weight和min_weight
    max_weight = max_weight2;
    min_weight = min_weight2;
  }
}


// 测试函数，打印中间结果
void router::test()
{
  for (auto &rslt : result2)
  {
    std::cout << rslt.interval.first << " " << rslt.interval.second << " "
              << rslt.wire_num << " " << rslt.inter_fpga << std::endl;
    for (auto &wire : rslt.wire1)
    {
      std::cout << "Wire1: " << wire << std::endl;
    }
    for (auto &wire : rslt.wire2)
    {
      std::cout << "Wire2: " << wire << std::endl;
    }
    for (auto &wire : rslt.wires)
    {
      std::cout << "Nets:";
      for (auto &net : wire)
      {
        std::cout << net << " ";
      }
      std::cout << std::endl;
    }
    for (auto &wire_tdm : rslt.tdm)
    {
      std::cout << wire_tdm << std::endl;
    }
  }
  for (auto &map : result_map)
  {
    std::cout << map.first.first << " " << map.first.second << " " << map.second
              << std::endl;
  }
}

void router::route()
{

  net_order();
  routing();

  tdm_assignment();

  //reroute();
  test();
}
