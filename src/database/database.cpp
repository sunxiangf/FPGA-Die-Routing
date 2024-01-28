#include "database.hpp"
#include <type_traits>

using namespace std;

void database::read1(const string &filename) {
  cout << "read 1" << endl;
  ifstream input_file(filename);
  if (!input_file) {
    cout << "Can't open file!" << endl;
    return;
  }
  string line;
  while (getline(input_file, line)) {
    istringstream line_stream(line);
    string fpga_part;
    string dies_part;
    if (getline(line_stream, fpga_part, ':') &&
        getline(line_stream, dies_part)) {
      int fpga_id = stoi(fpga_part.substr(4));
      fpgas.emplace_back(fpga(fpga_id));
      // fpga_map.emplace(fpga_id, fpgas.size() - 1);
      istringstream dies_stream(dies_part);
      string the_die;
      while (dies_stream >> the_die) {
        int die_id = stoi(the_die.substr(3));
        fpgas.back().dies.emplace_back(die_id);
        dies.emplace_back(die(fpga_id, die_id));
        // die_map.emplace(die_id, dies.size()-1);
      }
    }
  }
}

void database::read2(const string &filename) {
  cout << "read 2" << endl;
  ifstream input_file(filename);
  if (!input_file) {
    cout << "Can't open file!" << endl;
    return;
  }
  string line;
  while (getline(input_file, line)) {
    istringstream line_stream(line);
    string die_part;
    string nets_part;
    getline(line_stream, die_part, ':');
    getline(line_stream, nets_part);
    int die_id = stoi(die_part.substr(3));
    int fpga_id = dies.at(die_id).fpga_num;
    istringstream nets_stream(nets_part);
    string the_net;
    while (nets_stream >> the_net) {
      nets.emplace_back(net(fpga_id, die_id, nets.size(), the_net));
      dies.at(die_id).nets.emplace_back(the_net);
      net_map.emplace(the_net, nets.size() - 1);
    }
  }
}


void database::read3(const string &filename) {
  cout << "read 3" << endl;
  ifstream input_file(filename);
  if (!input_file) {
    cout << "Can't open file!" << endl;
    return;
  }

  string line;
  int net_num = 0;

  while (getline(input_file, line)) {
    stringstream ss(line);
    string word;
    vector<string> words;

    while (getline(ss, word, ' ')) {
      word.erase(
          remove_if(word.begin(), word.end(),
                         [](unsigned char x) { return isspace(x); }),
          word.end());
      words.emplace_back(word);
    }
    
    if (!words.empty()) {
      if (words.at(1) == "s") {
        net_num = net_map.at(words.at(0));
      } else if (words.at(1) == "l") {
        nets.at(net_num).net_connected.emplace_back(words.at(0));
        add_fpga_connection(nets.at(net_num).fpga_num, words.at(0));
      }
    }
  }
}

void database::read4(const string &filename) {
  cout << "read 4" << endl;
  ifstream input_file(filename);
  if (!input_file) {
    cout << "Can't open file!" << endl;
    return;
  }
  string line;
  unsigned  row = 0;
  while (getline(input_file, line)) {
    stringstream ss(line);
    string word;
    vector<int> row_data;
    int num;
    while(ss >> num){
        //cout<<num<<endl;
        row_data.push_back(num);
    }

    if (!row_data.empty()) {
      for (unsigned  i = 0; i < row_data.size(); i++) {
        if (row_data.at(i) != 0 && row<i) {
          die_network.emplace(make_pair(row, i), row_data.at(i));
        }
      }
    }
    row++;
  }
}

net &database::get_net(string net_name) {
  int net_num = net_map.at(net_name);
  return nets.at(net_num);
}

void database::add_fpga_connection(int fpga_num1, string net2) {
  int fpga_num2 = get_net(net2).fpga_num;
  if (fpga_num1 == fpga_num2) {
    return;
  }
  auto position = fpga_num1 < fpga_num2 ? make_pair(fpga_num1, fpga_num2)
                                        : make_pair(fpga_num2, fpga_num1);
  if (fpga_connection.find(position) == fpga_connection.end()) {
    fpga_connection.emplace(position, 1);
  } else {
    fpga_connection.at(position)++;
  }
}

void database::test() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
}
void database::test1() {
  cout << "Test 1" << endl;
  for (const auto &fpga : fpgas) {
    cout << "FPGA" << fpga.id << ": ";
    for (const auto &die : fpga.dies) {
      cout << "Die" << dies.at(die).id << " ";
    }
    cout << endl;
  }
}
void database::test2() {
  cout << "Test 2" << endl;
  for (const auto &die : dies) {
    cout << "die" << die.id << ": ";
    for (const auto &net : die.nets) {
      cout << net << " ";
    }
    cout << endl;
  }
}
void database::test3() {
  cout << "Test 3" << endl;
  for (const auto &net : nets) {
    cout << net.name << ":";
    for (const auto &the_net : net.net_connected) {
      cout << the_net << " ";
    }
    cout << endl;
  }
}
void database::test4() {
  cout << "Test 4" << endl;
  for (const auto &die : die_network) {
    cout << "(" << die.first.first << "," << die.first.second << ") ";
    cout << die.second << endl;
  }
}

void database::test5() {
  cout << "Test 5" << endl;
  for (const auto &die : dies) {
    cout << "die" << die.id << ":" << die.fpga_num;
    cout << endl;
  }

  for (const auto &net : nets) {
    cout << net.name << ":" << net.fpga_num << " " << net.die_num << " "
              << net.net_num;
    cout << endl;
  }
}

void database::test6() {
  cout << "Test 6" << endl;
  for (const auto &con : fpga_connection) {
    cout << "(" << con.first.first << "," << con.first.second << ") ";
    cout << con.second << endl;
  }
}

Database::Database(const database& db)
{

  for (const auto &fpga : db.fpgas)
    for (auto &die1 : fpga.dies)
      this->die.push_back(Die(db.dies.at(die1).id,fpga.id));
  
  net_map = db.net_map;

  for(auto node : db.nets)
  {
    this->node.push_back(Node(net_map.at(node.name),node.die_num));
    if (node.net_connected.size() != 0)
    {
      Net r(net.size(),net_map.at(node.name));
      for (string input_node : node.net_connected)
      {
        r.input_node.push_back(Input_node(net_map.at(input_node),db.nets.at(net_map.at(input_node)).die_num));
      }
      this->net.push_back(r);
    }
  }

  for (auto w : db.die_network)
  {
    if(db.dies.at(w.first.first).fpga_num !=  db.dies.at(w.first.second).fpga_num)
      this->wire.push_back(Wire(this->wire.size(), w.first.first, w.first.second, 1, w.second));
    else
      this->wire.push_back(Wire(this->wire.size(), w.first.first, w.first.second, 0, w.second));
  }
  
  for(auto w : wire)
  {
    pair<int, int> a(w.die_begin_id, w.die_end_id);
    wire_map[a] = w.id;
  }
}