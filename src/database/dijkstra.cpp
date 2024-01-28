#include "dijkstra.hpp"

using namespace std;

Graph::Graph(database &db) {
  V = db.dies.size();
  adjList.resize(V);
  for (const auto &pair : db.die_network) {
    auto [die1, die2] = pair.first;
    int weight = 20400 - pair.second;
    /*权值更改处*/
    add_edge(die1, die2, weight);
    add_edge(die2, die1, weight);
  }
  /*无用的总存*/
  dists.resize(V);

  predecessors.resize(V);
}

void Graph::add_edge(int src, int dest, int weight) {
  adjList[src].push_back(make_pair(dest, weight));
}

// Dijkstra algorithm
/*
void Graph::dijkstra() {
  for (int start = 0; start < V; start++) {
    dists[start].resize(V, numeric_limits<int>::max());
    predecessors[start].resize(V, -1);
    dists[start][start] = 0;
    priority_queue<pair<int, int>, vector<pair<int, int>>,
                        greater<pair<int, int>>>
        pq;
    pq.push(make_pair(0, start));

    while (!pq.empty()) {
      int u = pq.top().second;
      pq.pop();

      for (const auto &neighbor : adjList[u]) {
        int v = neighbor.first;
        int weight = neighbor.second;

        if (dists[start][v] > dists[start][u] + weight) {
          dists[start][v] = dists[start][u] + weight;
          predecessors[start][v] = u;
          pq.push(make_pair(dists[start][v], v));
        }
      }
    }
  }
}
*/



// 实现Dijkstra算法
void Graph::dijkstra() {
  // 遍历每个节点作为起点
  for (int start = 0; start < V; start++) {
    // 初始化所有节点到起点的距离为无限大
    dists[start].resize(V, numeric_limits<int>::max());
    // 初始化所有节点的前驱节点为-1（即不存在）
    predecessors[start].resize(V, -1);
    // 起点到自己的距离设置为0
    dists[start][start] = 0;

    // 使用优先队列优化，队列中存储（距离，节点编号）的对
    priority_queue<pair<int, int>, vector<pair<int, int>> , greater<pair<int, int>>>pq;
    // 将起点加入队列
    pq.push(make_pair(0, start));

    // 当队列非空时循环
    while (!pq.empty()) {
      // 取出队列头部元素（即当前距离最小的节点）
      int u = pq.top().second;
      pq.pop();

      // 遍历所有与节点u相连的节点
      for (const auto &neighbor : adjList[u]) {
        int v = neighbor.first; // 邻接节点编号
        int weight = neighbor.second; // 边的权重

        // 如果从起点通过u到v的距离小于已知的最短距离
        if (dists[start][v] > dists[start][u] + weight) {
          // 更新起点到v的最短距离
          dists[start][v] = dists[start][u] + weight;
          // 记录v的前驱节点为u
          predecessors[start][v] = u;
          // 将新的距离和节点v加入到队列中
          pq.push(make_pair(dists[start][v], v));
        }
      }
    }
  }
}

vector<int> Graph::shortest_path(int start, int end) {
  vector<int> path;
  if (predecessors[start][end] == -1) {
    cout << "No path!" << endl;
    return path;
  }

  if (predecessors[start][end] != -1) {
    for (int at = end; at != -1; at = predecessors[start][at]) {
      path.push_back(at);
    }
  }
  // if (predecessors[start][end] != -1) {
  //     cout << shortest path: ";
  //     for (auto it = path.rbegin(); it != path.rend(); ++it) {
  //       cout << *it;
  //       if (it != path.rend() - 1) {
  //         cout << " -> ";
  //       }
  //     }
  //     cout << endl;
  // }
  return path;
}

void Graph::test() {
  for (int i = 0; i < V; i++) {
    for (int j = 0; j < V; j++) {
      cout << i << " " << j << " " << dists[i][j] << endl;
    }
  }
}

/*
最短距离存储在 dists 数组中：dists 是一个二维向量（vector<vector<int>>），
其中 dists[start][end] 存储了从节点 start 到节点 end 的最短路径长度。

路径前驱节点存储在 predecessors 数组中：这同样是一个二维向量，
其中 predecessors[start][end] 存储了从节点 start 到节点 end 的最短路径上 end 的前驱节点。
通过这个信息，可以通过回溯的方式重建整个路径。

关于 priority_queue 的内部数据结构和存储的数据：

priority_queue 是一个标准库容器适配器，它提供了一个堆的实现。
在原的代码中，它被用作最小堆（min-heap）。

pair<int, int> 表示其中的元素是整数对。在这个上下文中，
每个对的第一个元素（first）代表从起点到当前节点的累积距离，第二个元素（second）代表该节点的编号。

vector<pair<int, int>> 是这个优先队列内部使用的容器类型，它存储了上述的整数对。

greater<pair<int, int>> 是一个比较函数对象，用于决定优先队列的排序顺序。
在这里，它确保队列按照距离的升序排列，这意味着距离最小的节点对总是位于队列的顶部。

综上所述，这个优先队列在每次迭代中存储了当前已知的最短路径候选节点，
以及对应的累积距离，从而保证了 Dijkstra 算法的高效执行。
*/