#pragma once

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// 表示一个网络节点
class net {
public:
  // 构造函数
  explicit net(int fpga_id, int die_id, int net_id, string net_name) {
    fpga_num = fpga_id;
    die_num = die_id;
    net_num = net_id;
    name = net_name;
  }

  string name; // 网络节点名称
  int fpga_num;     // FPGA 编号
  int die_num;      // Die 编号
  int net_num;      // 网络编号
  vector<string> net_connected; // 连接到该网络的其他网络节点
};

// 表示一个 Die
class die {
public:
  // 构造函数
  explicit die(int fpga_id, int die_id) {
    fpga_num = fpga_id;
    id = die_id;
  }
  int id;          // Die 编号
  int fpga_num;    // 归属的 FPGA 编号
  vector<string> nets; // 与此 Die 相连的网络节点
};

// 表示一个 FPGA
class fpga {
public:
  // 构造函数
  explicit fpga(int fpga_id) { id = fpga_id; }
  int id;                      // FPGA 编号
  vector<int> dies;       // 属于此 FPGA 的所有 Die 编号
};

// 数据库类，用于存储和处理数据
class database {
public:
  // 构造函数，读取并处理四个不同的输入文件
  explicit database(const string &file1, const string &file2,
                    const string &file3, const string &file4) {
    read1(file1); // 读取 FPGA 与 Die 的关系文件
    read2(file2); // 读取节点 Die 级分割位置文件
    read3(file3); // 读取连线定义文件
    read4(file4); // 读取 Die 级组网描述文件
  }
  explicit database() {}

  map<string, int> net_map; // 映射网络节点名称到网络编号

  vector<fpga> fpgas; // 存储 FPGA 信息
  vector<die> dies;   // 存储 Die 信息
  vector<net> nets;   // 存储网络节点信息

  map<pair<int, int>, int> die_network;      // 存储 Die 之间的连接关系和连线数量
  map<pair<int, int>, int> fpga_connection;  // 存储 FPGA 之间的连接关系和连线数量
  net &get_net(string net_name); // 获取指定名称的网络节点
  void test();
private:
  void add_fpga_connection(int fpga_num1, string net2); // 添加 FPGA 之间的连接
  void read1(const string &file); // 读取 design.fpga.die 文件
  void read2(const string &file); // 读取 design.die.position 文件
  void read3(const string &file); // 读取 design.net 文件
  void read4(const string &file); // 读取 design.die.network 文件
  void test1();
  void test2();
  void test3();
  void test4();
  void test5();
  void test6();
};

/*以下部分为THT数据结构*/

//节点的集合：节点包含节点编号（在db线性表中的序号）和所属的die。
class Node
{
public:
  Node(){}
  Node(int i,int d) : id(i), die_id(d){}
  int id;
  int die_id;
};

//die的集合，die包含编号和所属的FPGA。
class Die
{
public:
  Die(int i,int F) : id(i), FPGA_id(F){}
  int id;
  int FPGA_id;
};

//接收节点具有节点编号，到接收节点的路径，到接受节点的权值。
class Input_node
{
public:
  Input_node(int i,int di) : id(i),die_id(di), rw(0){}
  int id;
  int die_id;
  double rw;
  vector<int> path;
};

//net的集合：一个net包含该net最大rw，最大rw对应的接受节点的编号，一个发射节点，多个接收节点，
class Net
{
public:
  Net(int oi,int i){id = i;max_rw = 0;max_rw_node_id = 0;output_node_id = oi;}
  int id;
  double max_rw;
  int max_rw_node_id;
  int output_node_id;
  vector<Input_node> input_node;
};

//FPGAwire：net的编号数组，方向(在这个版本用不上)，时分复用值，更新net函数。
class FPGA_wire
{
public:
  FPGA_wire(){TDM_Ratio = 0;used = 1;}
  vector<int> net_id;
  int TDM_Ratio;
  bool used;
};

//wire的集合：wire包含一对die编号（小到大），自己的编号，跨FPGA与否，一个FPGAwire数组。
class Wire
{
public:
  Wire(int i,int dbi,int dei,int i_f,int wn) : id(i), die_begin_id(dbi), die_end_id(dei), inter_fpga(i_f), wire_num(wn)
  {
    if(i_f == 1)fpga_wire.resize(wn);
  }
  int id;
  int die_begin_id;
  int die_end_id;
  int inter_fpga;
  int wire_num;
  vector<FPGA_wire> fpga_wire;
};

class Database 
{
public:
  // 构造函数，读取并处理四个不同的输入文件
  Database(const database& db); 

  vector<Node> node;   // 存储 node 信息
  vector<Net> net;     // 存储 net 信息
  vector<Die> die;     // 存储 die 信息
  vector<Wire> wire;   // 存储 wire 信息

  map<string, int> net_map;       // 映射网络节点名称到网络编号
  map<pair<int,int>,int> wire_map;   // 映射die节点对到wire编号

};

