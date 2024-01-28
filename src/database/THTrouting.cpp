#include <iostream>
#include <vector>
#include "THTrouting.hpp"
#include <algorithm>

using namespace std;

bool cmp1(Input_node a, Input_node b)
{
    return a.rw > b.rw ? 1 : 0;
}

bool cmp2(Net a, Net b)
{
    return a.max_rw > b.max_rw ? 1 : 0;
}

bool cmp3(pair<int, Net *> net1, pair<int, Net *> net2)
{
    return net1.first > net2.first ? 1 : 0;
}

bool cmp4(const pair<int, int>& a, const pair<int, int>& b) {
    return a.first < b.first;
}

void THTrouting(Database &db)
{
    THTGraph graph(db);
    graph.dijkstra_die();
    for (auto &output_node : db.net)
    {
        for (auto &input_node : output_node.input_node)
        {
            input_node.rw = graph.dists[db.node[output_node.output_node_id].die_id][db.node[input_node.id].die_id];
        }
        sort(output_node.input_node.begin(), output_node.input_node.end(), cmp1);
        output_node.max_rw = output_node.input_node[0].rw;
    }
    sort(db.net.begin(), db.net.end(), cmp2);

    for (auto &net : db.net)
    {
        graph.init2(db);
        graph.dijkstra2(db, net);
    }

    for (auto &net : db.net)
    {
        net.max_rw = -1;
        for (auto &input_node : net.input_node)
        {
            input_node.rw = 0;
            for (int i = 0; i < input_node.path.size() - 1; i++)
            {
                input_node.rw += graph.add_rw(db, input_node.path[i], input_node.path[i + 1]);
            }
            if (net.max_rw < input_node.rw)
                net.max_rw = input_node.rw;
        }
    }

    int finish = 1;
    // rerouting部分

    while (finish)
    {
        vector<pair<double, Net *>> net_rw;
        for (auto &net : db.net)
            net_rw.push_back(make_pair(net.max_rw, &net));

        sort(net_rw.begin(), net_rw.end(), cmp3);
        cout << "最大权值为：" << net_rw[0].first << endl;

        graph.max_max_rw = net_rw[0].first;
        graph.net_max_rw.clear();
        for (int i = 0; net_rw[i].first == graph.max_max_rw; i++)
            graph.net_max_rw.push_back(net_rw[i].second);

        if (graph.dis_rerouting(db, db, *graph.net_max_rw[graph.net_max_rw.size() - 1], graph.net_max_rw) == false)
        {
            if (graph.FPGAwire_rerouting(db, db, *graph.net_max_rw[graph.net_max_rw.size() - 1], graph.net_max_rw) == false)
            {
                finish = 0;
            }
        }
    }
    vector<pair<double, Net *>> net_rw;
    for (auto &net : db.net)
        net_rw.push_back(make_pair(net.max_rw, &net));

    sort(net_rw.begin(), net_rw.end(), cmp3);
    cout << "最大权值为：" << net_rw[0].first << endl;

    cout << "开始检验 Wire 和 Net 的对应关系...(无输出代表正确)" << endl;
    // 用来测试检验程序问题的特例注入
    // db.net[46].input_node[0].path.push_back(3);
    // db.net[46].input_node[0].path.push_back(2);
    // db.net[46].input_node[0].path.push_back(1);
    // db.net[46].input_node[0].path.push_back(0);

    graph.adjustnetrw(db);

    for (auto &net : db.net)
    { // 遍历所有的 Net
        for (auto &input_node : net.input_node)
        { // 遍历每个 Net 的输入节点
            for (size_t i = 0; i < input_node.path.size() - 1; ++i)
            {
                int start = input_node.path[i];
                int end = input_node.path[i + 1];

                Wire *wire = nullptr;
                int fpga_num = 0;
                graph.find_wire_from_die(db, start, end, wire, fpga_num); // 查找经过的 Wire

                if (wire != nullptr && wire->inter_fpga == 1)
                {
                    // 检查 Wire 是否正确记录了该 Net
                    if (std::find(wire->fpga_wire[fpga_num].net_id.begin(), wire->fpga_wire[fpga_num].net_id.end(), net.id) == wire->fpga_wire[fpga_num].net_id.end())
                    {
                        cout << "不一致发现: Net " << net.id << endl;
                        cout << "  Net 路径: ";
                        for (auto n : input_node.path)
                        {
                            cout << n << " ";
                        }
                        cout << "\n  Net rw: " << input_node.rw << endl;
                        cout << "  出错路径对: " << start << " -> " << end << endl;
                        cout << "  Wire ID: " << wire->id << ", FPGA Num: " << fpga_num << endl;
                        cout << "  Wire 上的 Net ID 列表: ";
                        for (auto n : wire->fpga_wire[fpga_num].net_id)
                        {
                            cout << n << " ";
                        }
                        cout << endl;
                    }
                }
            }
        }
    }

    export_routing_result(db,"design.route.out");
    export_tdm_result(db,"design.tdm.out");

    return;
}

/************************************************************************************************/
/************************************************************************************************/
/************************************************************************************************/

THTGraph::THTGraph(Database &db)
{
    V = db.die.size();
    adjList.resize(V);
    for (const auto &wire : db.wire)
    {
        add_edge(wire.die_begin_id, wire.die_end_id, wire.inter_fpga == 1 ? 5 : 1);
        add_edge(wire.die_end_id, wire.die_begin_id, wire.inter_fpga == 1 ? 5 : 1);
    }
    dists.resize(V);
    predecessors.resize(V);
}

void THTGraph::add_edge(int src, int dest, double weight)
{
    if (weight < 0)
    {
        return;
    }

    pair<int, double> a(dest, weight);
    adjList.at(src).push_back(a);
}

// 实现Dijkstra算法
void THTGraph::dijkstra_die()
{
    for (int start = 0; start < V; start++)
    {
        dists[start].resize(V, numeric_limits<int>::max());
        dists[start][start] = 0;
        predecessors[start].resize(V, -1);

        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
        pq.push(make_pair(0, start));

        while (!pq.empty())
        {
            int u = pq.top().second;
            pq.pop();

            for (const auto &neighbor : adjList[u])
            {
                int v = neighbor.first;       // 邻接节点编号
                int weight = neighbor.second; // 边的权重

                if (dists[start][v] > dists[start][u] + weight)
                {
                    dists[start][v] = dists[start][u] + weight;
                    predecessors[start][v] = u;
                    pq.push(make_pair(dists[start][v], v));
                }
            }
        }
    }
}

void THTGraph::init2(Database &db)
{
    adjList.clear();
    V = db.die.size();
    for (auto &wire : db.wire)
    {
        if (wire.inter_fpga == 1)
        {
            pair<int, int> a(wire.die_begin_id, wire.die_end_id);
            more_node_map[a] = V;
            for (int i = 0; i < wire.wire_num; i++)
            {
                more_node_map_V[V + i] = a;
            }

            this->V += wire.wire_num - 1;
            adjList.resize(V);
            if (wire.fpga_wire[0].used != 0)
            {
                add_edge(wire.die_begin_id, wire.die_end_id, (double)(wire.fpga_wire[0].TDM_Ratio * 2 * 4 + 1) / 2);
            }
            int num_node = more_node_map[a];
            int fpga_num = 1;
            for (int i = 1; i < ((wire.wire_num + 1) / 2); i++)
            {
                if (wire.fpga_wire[i].used == 0)
                {
                    num_node++;
                    fpga_num++;
                    continue;
                }
                add_edge(wire.die_begin_id, num_node, 0);
                add_edge(num_node, wire.die_begin_id, 0);
                // 路径左侧为经过线条的一半，右侧为0
                add_edge(num_node, wire.die_end_id, (double)(wire.fpga_wire[fpga_num].TDM_Ratio * 2 * 4 + 1) / 2);
                num_node++;
                fpga_num++;
            }

            for (int i = 0; i < wire.wire_num / 2; i++)
            {
                if (wire.fpga_wire[fpga_num].used == 0)
                {
                    num_node++;
                    fpga_num++;
                    continue;
                }
                // add_edge(more_node_map[a] + ((wire.wire_num - 1) / 2) + i,wire.die_end_id,0);
                add_edge(wire.die_end_id, num_node, 0);
                add_edge(num_node, wire.die_end_id, 0);
                add_edge(num_node, wire.die_begin_id, (double)(wire.fpga_wire[fpga_num].TDM_Ratio * 2 * 4 + 1) / 2);
                num_node++;
                fpga_num++;
            }
        }
        else
        {
            adjList.resize(V);
            add_edge(wire.die_begin_id, wire.die_end_id, 1);
            add_edge(wire.die_end_id, wire.die_begin_id, 1);
        }
    }
}

// 实现Dijkstra2算法
void THTGraph::dijkstra2(Database &db, Net &net)
{
    // 存储该net中那些节点已经被通过了

    if (net.id == 1331)
    {
        cout << "debug";
    }

    int out_die_id = db.node[net.output_node_id].die_id;
    map<int, int> node_have;
    for (auto &input_node : net.input_node)
    {
        int in_die_id = input_node.die_id;
        // 初始化节点到发射节点的最小距离和前置节点
        dists.clear();
        dists.resize(1);
        dists[0].resize(V, numeric_limits<int>::max());
        dists[0][out_die_id] = 0;
        predecessors.clear();
        predecessors.resize(1);
        predecessors[0].resize(V, -1);
        predecessors[0][out_die_id] = -2;

        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
        pq.push(make_pair(0, out_die_id));

        while (!pq.empty())
        {
            int u = pq.top().second;
            // 找到了终点就退出
            pq.pop();
            // cout << "当前节点为" << u << "，有邻接节点数：" << adjList[u].size() << endl;
            for (const auto &neighbor : adjList[u])
            {
                int v = neighbor.first;       // 邻接节点编号
                int weight = neighbor.second; // 边的权重
                // cout << "编号：" << v << "权重:" << weight << endl;

                if (dists[0][v] > dists[0][u] + weight)
                {
                    dists[0][v] = dists[0][u] + weight;
                    // cout << "修改节点"<< v <<"的路径长度为" << dists[0][v] << "经过节点" << u << endl;
                    predecessors[0][v] = u;
                    pq.push(make_pair(dists[0][v], v));
                }
            }
        }
        // 处理路径上的权值更新问题。
        int p = -1;           // 保留上一个节点编号
        bool if_add_path = 1; // 记录是否需要更新路径权值
        for (int it = in_die_id; it != -2; it = predecessors[0][it])
        {
            input_node.path.push_back(it); // 路径反着存，别忘了

            if (p != -1)
            {
                if (if_add_path == 1)
                {
                    add_net_path(it, p, net.id, db);
                    add_edge(it, p, 0); // 添加一条长度为0的边，相当于把边长度改成了0
                }
            }

            // 如果该节点已经经过过了，之后不再更新路径权值（net共用）
            if (node_have.find(it) != node_have.end()) // 找到了
                if_add_path = 0;
            else // 没找到
                node_have[it] = 1;

            p = it;
        }
    }
}

void THTGraph::add_net_path(int start, int end, int net_num, Database &db)
{
    Wire *wire = nullptr;
    int fpga_num = 0; // 确认FPGA编号
    find_wire_from_die(db, start, end, wire, fpga_num);
    if (wire == nullptr && fpga_num == -1)
        return;

    if (wire->inter_fpga == 0)
    {
        wire->wire_num -= 1;
        return;
    }
    else
    {
        wire->fpga_wire[fpga_num].net_id.push_back(net_num); // 修改对应wire的对应FPGA线路存储的net，并更新TDM值
        if (wire->fpga_wire[fpga_num].net_id.size() > wire->fpga_wire[fpga_num].TDM_Ratio * 4)
        {
            wire->fpga_wire[fpga_num].TDM_Ratio += 1;
        }
    }
}

double THTGraph::add_rw(Database &db, int start, int end)
{
    Wire *wire = nullptr;
    int fpga_num = 0; // 确认FPGA编号
    find_wire_from_die(db, start, end, wire, fpga_num);
    if (wire == nullptr && fpga_num == -1)
        return 0;

    if (wire->inter_fpga == 0)
        return 1;
    else
        return (double)(2 * 4 * wire->fpga_wire[fpga_num].TDM_Ratio + 1) / 2;
}

int THTGraph::dijkstra3(Database &db, Net &net)
{
    // 存储该net中那些节点已经被通过了

    int out_die_id = db.node[net.output_node_id].die_id;
    map<int, int> node_have;
    for (auto &input_node : net.input_node)
    {
        int in_die_id = input_node.die_id;
        // 初始化节点到发射节点的最小距离和前置节点
        dists.clear();
        dists.resize(1);
        dists[0].resize(V, numeric_limits<int>::max());
        dists[0][out_die_id] = 0;
        predecessors.clear();
        predecessors.resize(1);
        predecessors[0].resize(V, -1);
        predecessors[0][out_die_id] = -2;

        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
        pq.push(make_pair(0, out_die_id));

        while (!pq.empty())
        {
            int u = pq.top().second;
            // 找到了终点就退出
            pq.pop();
            for (const auto &neighbor : adjList[u])
            {
                int v = neighbor.first;       // 邻接节点编号
                int weight = neighbor.second; // 边的权重

                if (dists[0][v] > dists[0][u] + weight)
                {
                    dists[0][v] = dists[0][u] + weight;

                    predecessors[0][v] = u;
                    pq.push(make_pair(dists[0][v], v));
                }
            }
        }
        // 处理路径上的权值更新问题。
        int p = -1;           // 保留上一个节点编号
        bool if_add_path = 1; // 记录是否需要更新路径权值
        // 找不到路径了捏
        if (predecessors[0][in_die_id] == -1)
        {
            return 0;
        }

        for (int it = in_die_id; it != -2; it = predecessors[0][it])
        {

            if (p != -1)
            {
                if (if_add_path == 1)
                {
                    if (add_net_path_dis3(it, p, net.id, db) == 0)
                    {
                        return 1;
                    }

                    add_edge(it, p, 0); // 添加一条长度为0的边，相当于把边长度改成了0
                }
            }

            // 如果该节点已经经过过了，之后不再更新路径权值（net共用）
            if (node_have.find(it) != node_have.end()) // 找到了
                if_add_path = 0;
            else // 没找到
                node_have[it] = 1;

            // 先判断这路径能不能加，再加路径
            input_node.path.push_back(it); // 路径反着存，别忘了

            p = it;
        }
    }

    return 2;
}

/*
需求文档
给定一个net，要求给出net的可能更优路径，使得net的max_rw小于最优路径，
跑该net的最短路径，重新计算经过的FPGA线路上的net的rw，如果他们当中有人rw大于等于了最大rw，将该FPGA线路标记为不可通行线路，重跑。
如果最后无法通行，报告找不到这样的路径(0)。
如果有，得出rw，和存储的rw进行比较，如果更小，保留当前的结果，求下一轮的数。否则，报告找不到这样的路径（原路径做不到更小）。
*/
bool THTGraph::dis_rerouting(Database db, Database &db_old, Net &net, vector<Net *> net_max_rw)
{
    // 重新初始化图，为新的路由尝试准备。
    init2(db);

    // 在数据库中查找当前 net 的索引。
    int net_id = -1;
    for (int i = 0; i < db.net.size(); i++)
    {
        if (db.net[i].id == net.id)
        {
            net_id = i;
            break;
        }
    }
    if (net_id == -1)
    {
        // 如果找不到 net，返回 false。
        return false;
    }

    // 重路由的主循环。
    while (true)
    {
        // 清除旧路径，为新路径的建立做准备。
        clearOldPaths(db, net_id);
        // 尝试找到新的路径。
        int pathFindingResult = dijkstra3(db, db.net[net_id]);

        switch (pathFindingResult)
        {
        case 0: // 没有路径能够成功建立。
            cout << "无路可选了……" << endl;
            return false;

        case 1:        // 找到的新路径导致 rw 过大。
            init2(db); // 重新初始化，为下一次尝试做准备。
            continue;

        case 2: // 找到了一条可能的新路径。
            adjustTDMRadio(db);
            adjustnetrw(db);
            if (db.net[net_id].max_rw < net.max_rw)
            {
                // 如果新路径的 rw 小于原路径，表示找到了更优的路径。
                cout << net.id << "已找到更优路径" << endl;
                cout << "原路径(" << net.max_rw << "): ";
                for (auto it = db_old.net[net_id].input_node[0].path.rbegin(); it != db_old.net[net_id].input_node[0].path.rend(); ++it)
                {
                    cout << *it << " ";
                }
                cout << endl;
                cout << "新路径(" << db.net[net_id].max_rw << "): ";
                for (auto it = db.net[net_id].input_node[0].path.rbegin(); it != db.net[net_id].input_node[0].path.rend(); ++it)
                {
                    cout << *it << " ";
                }
                cout << endl;
                for (auto &wire : db.wire)
                    if (wire.inter_fpga == 1)
                    {
                        for (auto &fpgawire : wire.fpga_wire)
                            fpgawire.used = 1;
                    }

                db_old = db;
                return true;
            }
            else
            {
                return false;
            }

        default:
            // 其他情况，表示出现了意料之外的结果。
            cout << "dis3返回值意外" << endl;
            return false;
        }
    }

    // 循环结束，返回 false。
    return false;
}

bool THTGraph::add_net_path_dis3(int start, int end, int net_num, Database &db) // 添加没问题就返回1，否则返回0
{
    Wire *wire = nullptr;
    int fpga_num = 0; // 确认FPGA编号
    find_wire_from_die(db, start, end, wire, fpga_num);
    if (wire == nullptr && fpga_num == -1)
        return 1;

    if (wire->inter_fpga == 0)
    {
        wire->wire_num -= 1;
        return 1;
    }
    else
    {

        wire->fpga_wire[fpga_num].net_id.push_back(net_num); // 修改对应wire的对应FPGA线路存储的net，并更新TDM值
        wire->fpga_wire[fpga_num].TDM_Ratio = (wire->fpga_wire[fpga_num].net_id.size() + 3) / 4;

        adjustnetrw(db);

        for (auto &net : db.net)
        {
            if (net.max_rw > max_max_rw)
            {
                wire->fpga_wire[fpga_num].net_id.erase(wire->fpga_wire[fpga_num].net_id.end() - 1);
                wire->fpga_wire[fpga_num].TDM_Ratio = (wire->fpga_wire[fpga_num].net_id.size() + 3) / 4;
                wire->fpga_wire[fpga_num].used = 0;
                cout << "装路啦1"
                     << "在尝试连接线路：" << start << " 到 " << end << " 时,由于net： " << net.id << " 换线失败！ " << net.max_rw << endl;
                return 0;
            }
            bool t = 0;
            for (auto net_max : net_max_rw)
                if (net_max->id == net.id)
                    t = 1;

            if (net.max_rw == max_max_rw && !t)
            {
                wire->fpga_wire[fpga_num].net_id.erase(wire->fpga_wire[fpga_num].net_id.end() - 1);
                wire->fpga_wire[fpga_num].TDM_Ratio = (wire->fpga_wire[fpga_num].net_id.size() + 3) / 4;
                wire->fpga_wire[fpga_num].used = 0;
                cout << "装路啦2"
                     << "在尝试连接线路：" << start << " 到 " << end << " 时,由于net： " << net.id << " 换线失败！ " << net.max_rw << endl;
                return 0;
            }
        }
    }
    return 1;
}

// 清除旧路径的函数：为新路径的建立清理旧的路径信息。
void THTGraph::clearOldPaths(Database &db, int net_id)
{
    // 存储原来的路径。
    map<pair<int, int>, int> old_path;
    for (auto input_node : db.net[net_id].input_node)
    {

        if (input_node.path.size() == 0) // 虽然感觉多余了，但是删了就是会越界……
            continue;

        for (int i = 0; i < input_node.path.size() - 1; i++)
        {
            old_path[make_pair(input_node.path[i], input_node.path[i + 1])] = 1;
        }
    }

    // 删除原来的线路。
    for (auto path : old_path)
    {
        Wire *wire = nullptr;
        int fpga_num = 0;
        find_wire_from_die(db, path.first.first, path.first.second, wire, fpga_num);
        if (wire != nullptr && fpga_num != -1)
        {
            removeNetFromWire(wire, db.net[net_id].id, fpga_num);
        }
    }

    for (auto &input_node : db.net[net_id].input_node)
    {
        // 清除路径
        input_node.path.clear();
    }
}

// 从 wire 中移除 net 的函数。
void THTGraph::removeNetFromWire(Wire *wire, int net_id, int fpga_num)
{
    if (wire->inter_fpga == 1)
    {
        auto &net_ids = wire->fpga_wire[fpga_num].net_id;
        net_ids.erase(std::remove(net_ids.begin(), net_ids.end(), net_id), net_ids.end());
        wire->fpga_wire[fpga_num].TDM_Ratio = (wire->fpga_wire[fpga_num].net_id.size() + 3) / 4;
    }
    else
    {
        wire->wire_num += 1;
    }
}

/*
需求文档
给定一个net，尝试修改该net经过的FPGA路径上的其他路径，最少需要能减少到4的整数倍（如果已经是，那就是下一个最小4整数倍）条路径，使得该net的rw变小。
每跑一次就需要重新计算经过的FPGA线路上的net的rw，如果他们当中有人rw大于等于了最大rw，将该FPGA线路也标记为不可通行线路，重跑。
如果最后无法通行，尝试更改另一条。
如果最后数量满足不了，报告无法施行(0)。
如果最后得到了路径结果，报告(1)。
*/
bool THTGraph::FPGAwire_rerouting(Database db, Database &db_old, Net &net, vector<Net *> net_max_rw)
{
    cout << "开始跑FPGAwire——rerouting" << endl;
    init2(db);
    int net_id = -1;
    for (int i = 0; i < db.net.size(); i++)
    {
        if (db.net[i].id == net.id)
            net_id = i;
    }
    if (net_id == 4)
    {
        cout << "debug jieogu" << endl;
    }
    
    net_need_to_move.clear();
    net_map.clear();
    // 第一步，找到所有经过的fpga线路
    map<pair<int, int>, int> old_path;
    for (auto input_node : db.net[net_id].input_node)
    {
        for (int i = 0; i < input_node.path.size() - 1; i++)
        {
            old_path[make_pair(input_node.path[i], input_node.path[i + 1])] = 1;
        }
    }
    for (auto path : old_path)
    {
        int start = path.first.first;
        int end = path.first.second;
        Wire *wire = nullptr;
        int fpga_num = 0;
        find_wire_from_die(db, start, end, wire, fpga_num);
        if (wire != nullptr && wire->inter_fpga == 1)
        {
            for (const auto &net : wire->fpga_wire[fpga_num].net_id)
                net_need_to_move.push_back(make_pair(0, net));
            wire->fpga_wire[fpga_num].used = 0; // 换线的时候不允许使用这条路
        }
    }
    for (int i = 0; i < db.net.size();i++) // 记录所有可以移动的线路及其权重
    {
        for (auto &p_net_id : net_need_to_move)
        {
            if (db.net[i].id == p_net_id.second)
            {
                net_map[p_net_id.second] = i;
                p_net_id.first = db.net[i].max_rw;
            }
        }
    }
    
    sort(net_need_to_move.begin(), net_need_to_move.end(), cmp4);
    int condition = 0;
    for (int i = 0; i < net_need_to_move.size() && condition == 0;)
    {
        cout << "跑" << net_need_to_move[i].second << "net(id):" << endl;
        if (net_need_to_move[i].second <= 0 || net_need_to_move[i].second >= db.net.size())
        {
            i++;
            continue;
        }
        
        int move_net_id = net_map[net_need_to_move[i].second];
        // 清除旧路径，为新路径的建立做准备。
        clearOldPaths(db, move_net_id);
        // 尝试找到新的路径。
        int pathFindingResult = dijkstra3(db, db.net[move_net_id]); // 一模一样跑一遍，只要变线不超过当前最大值，不产生新最大值就算成功。
        switch (pathFindingResult)
        {
        case 0:
            cout << "net " << move_net_id << "换线失败！" << endl;
            i++; // 尝试下一条线路
            break;
        case 1:
            init2(db);
            break;
        case 2:
            adjustTDMRadio(db);
            adjustnetrw(db);
            cout << "net " << move_net_id << "换线成功！" << endl;
            if (db.net[net_id].max_rw < max_max_rw)
            {
                cout << "net " << net_id << "当前权值已下降为：" << db.net[net_id].max_rw << endl;
                condition = 1;
            }
            else
            {
                i++;
                for (auto &wire : db.wire)
                    if (wire.inter_fpga == 1)
                    {
                        for (auto &fpgawire : wire.fpga_wire)
                            fpgawire.used = 1;
                    }
                for (auto path : old_path)
                {
                    int start = path.first.first;
                    int end = path.first.second;
                    Wire *wire = nullptr;
                    int fpga_num = 0;
                    find_wire_from_die(db, start, end, wire, fpga_num);
                    if (wire != nullptr && wire->inter_fpga == 1)
                    {
                        wire->fpga_wire[fpga_num].used = 0; // 换线的时候不允许使用这条路
                    }
                }
            }
            break;
        default:
            cout << "debug1024" << endl;
            break;
        }
    }

    if (condition == 1)
    {
        adjustTDMRadio(db);
        adjustnetrw(db);
        for (auto &wire : db.wire)
            if (wire.inter_fpga == 1)
            {
                for (auto &fpgawire : wire.fpga_wire)
                    fpgawire.used = 1;
            }
        db_old = db;
        return true;
    }
    else
    {
        return false;
    }
}

void THTGraph::find_wire_from_die(Database &db, int start, int end, Wire *&wire, int &fpga_num)
{
    // 按照小到大的节点对
    pair<int, int> a(start <= end ? start : end, start < end ? end : start);
    // 如果这是一条权值为0的路，说明是同源节点，不需要处理
    if (start >= db.die.size() || end >= db.die.size())
    {
        int WireNum = db.wire[db.wire_map[more_node_map_V[a.second]]].wire_num;
        if ((a.second <= (WireNum + 1) / 2 + more_node_map[more_node_map_V[a.second]] && a.first == more_node_map_V[a.second].first) ||
            (a.second > (WireNum + 1) / 2 + more_node_map[more_node_map_V[a.second]] && a.first == more_node_map_V[a.second].second))
        {
            wire = nullptr;
            fpga_num = -1;
            return;
        }
    }

    if (start < db.die.size() && end < db.die.size()) // 找到对应的线路
    {
        wire = &db.wire[db.wire_map[a]]; // 定位wire指针
        fpga_num = 0;                    // 定位fpga编号（如果是跨FPGA才用得到）
    }
    else if (start < db.die.size())
    {
        wire = &db.wire[db.wire_map[more_node_map_V[end]]];
        fpga_num = end - more_node_map[more_node_map_V[end]] + 1;
    }
    else
    {
        wire = &db.wire[db.wire_map[more_node_map_V[start]]];
        fpga_num = start - more_node_map[more_node_map_V[start]] + 1;
    }

    return;
}

void THTGraph::adjustTDMRadio(Database &db)
{
    for (auto &wire : db.wire)
    { // 遍历所有的 wire
        for (auto &fpgaWire : wire.fpga_wire)
        { // 遍历每个 wire 的所有 fpga_wire
            fpgaWire.TDM_Ratio = (fpgaWire.net_id.size() + 3) / 4;
        }
    }
}

void THTGraph::adjustnetrw(Database &db)
{
    for (auto &net : db.net)
    {

        net.max_rw = -1;
        for (auto &input_node : net.input_node)
        {
            input_node.rw = 0;
            if (input_node.path.size() != 0)
            {
                for (int i = 0; i < input_node.path.size() - 1; i++)
                {
                    if (net.id == 199)
                    {
                        // cout << "从路径 "  << input_node.path[i] << "到" << input_node.path[i+1] << "权值为 " << add_rw(db,input_node.path[i],input_node.path[i+1]) << endl;
                    }
                    input_node.rw += add_rw(db, input_node.path[i], input_node.path[i + 1]);
                }
            }

            if (net.max_rw < input_node.rw)
                net.max_rw = input_node.rw;
        }
    }
}

void export_routing_result(const Database& db, const std::string& filename) {
    std::ofstream file(filename);

    // 首先，对net按照max_rw进行排序
    auto sorted_nets = db.net;
    std::sort(sorted_nets.begin(), sorted_nets.end(), [](const Net& a, const Net& b) {
        return a.max_rw > b.max_rw;
    });

    // 输出每个Net的信息
    for (const auto& net : sorted_nets) {
        file << "[" << net.id << "]\n";
        for (const auto& inode : net.input_node) {
            file << "[";
            for (size_t i = 0; i < inode.path.size(); ++i) {
                if (inode.path[i]<db.die.size())
                {
                    file << inode.path[i];
                    if (i < inode.path.size() - 1) file << ",";
                }
            }
            file << "][" << inode.rw << "]\n";
        }
    }

    file.close();
}

void export_tdm_result(const Database& db, const std::string& filename) {
    std::ofstream file(filename);

    // 遍历所有Wire
    for (const auto& w : db.wire) {
        // 输出Die编号
        file << "[" << "Die" << w.die_begin_id << ",Die" << w.die_end_id << "]\n";

        // 遍历该Wire的所有FPGA_wire
        for (const auto& fw : w.fpga_wire) {
            file << "[";
            for (size_t i = 0; i < fw.net_id.size(); ++i) {
                file << fw.net_id[i];
                if (i < fw.net_id.size() - 1) file << ",";
            }
            file << "] " << fw.TDM_Ratio << "\n";
        }
    }

    file.close();
}