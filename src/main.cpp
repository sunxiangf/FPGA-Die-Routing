#include "database/routing.hpp"
#include "database/database.hpp"
#include "database/THTrouting.hpp"

#include <iostream>
#include <map>
#include <string>
using namespace std;

int main(int argc, char *argv[]) {
  if (argc == 1) {
    string file1 = "../TestCase/testcase4/design.fpga.die";     // 1
    string file2 = "../TestCase/testcase4/design.die.position"; // 2
    string file3 = "../TestCase/testcase4/design.net";          // 3
    string file4 = "../TestCase/testcase4/design.die.network";  // 4
    // cout<<file1<<endl;  //Debug Info

    database db(file1, file2, file3, file4);
    db.test();

    Database THT_db(db);

    router router1(db);

    //router1.test();
    //cout << endl;

    router1.route();
    //router1.test();
    //cout << endl;

    // cout<<file1<<endl;  //Debug Info

    THTrouting(THT_db);

  } else if (argc == 5) {
    database db(argv[1], argv[2], argv[3], argv[4]);
    cout<<argv[1]<<endl;  //Debug Info
    db.test();
    router router1(db);
    router1.test();
    router1.route();
    cout<<argv[1]<<endl;  //Debug Info
  }
  return 0;
}



/*
思路内容：
将多个节点构建net，net包含一个发射节点，多个接收节点和当前net最大rw和最大rw对应的节点编号。
每个接收节点上保存有节点编号，名称（或许有查询表？），节点归属die编号，该节点到发射节点的路径。

将每个die作为一个节点，根据两个die之间通行的时间消耗为权值，构建图，构建dijkstra算法。

第一步，权值计算：
用dis算法跑一遍所有die到其他die的路径最短消耗，将其保存在2维数组中（vector）。【目前先将FPGA看做5，die看做1，后续再进行优化】
将所有节点对映射到上面每个net为一个单元进行运算。运算完之后，以rw为判断依据排序，更新net的rw。
完成所有net内部的rw运算后，将所有net按照net的rw排序。

将所有需要处理的net进行遍历：
从rw最大的节点对开始，我们根据其所在的die，映射为用dijkstra算法查询这两个die之间的最短路径。
每当得到一个节点对的最短路径后，将其存储，并根据最短路径上经过的线路，更新跨了FPGA的die之间的通讯延时（权值），并计算该节点对通讯总延时，维护节点对通讯延时的排序数列。
将当前通过的所有节点路径改为0，方向不变，其他的重新复位，在本net中，之后的路径更新到节点列表中的节点时即停止更新（但为了路径还是要继续递归）。具体方法为在线路本身上进行存储，路径更新调用线路函数。
对于之后的节点，当前图中所经过的所有节点均为原点，即在这些点之间的路径临时长度更改为0。
当得到所有节点对路径后，重复以下过程：
重新对当前通讯延时最大的net里最大的节点对进行上面的dijkstra算法重新运算最短路径，如果存在一条更短的路径，将其路径改为更短的路径并重复该循环；
如果不存在更短的路径了，查找当前节点对经过的路径上的所有跨FPGA线路，尝试更改在该FPGA线路上的一条其他节点对路径，
具体更改方式为临时将该FPGA线路权值设定为无穷大，用上方dijkstra算法重跑最短路径。如果该最短路径小于当前所有节点对的通讯延时最大值，则更改路径成功。
否则，尝试更改该FPGA线路上的另一节点对的路径。
如果尝试了完所有其他节点对仍然无法降低通讯延时最大值，停止循环。
*/
/*
数据结构：
db数据库：
die的集合，die包含编号和所属的FPGA。
节点的集合：节点包含节点编号（在db线性表中的序号）和所属的die。
为了节点和名字的映射，我们做一个名字与节点的对应表。
net的集合：一个net包含该net最大rw，最大rw对应的接受节点的编号，一个发射节点，多个接收节点，接收节点具有节点编号，到接收节点的路径，到接受节点的权值。
wire的集合：wire包含一对die编号（小到大），自己的编号，跨FPGA与否，一个FPGAwire数组。
FPGAwire：net的编号数组，方向，时分复用值，更新net函数。
dis图：die节点
result1：关于net路径和rw，自己从db的net网络里找；
result2：任务完成后，将扩张的节点根据之前保存的对应关系记录整理。
找到是FPGA的线路，根据其中数据输出即可。
*/