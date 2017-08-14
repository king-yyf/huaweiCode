#ifndef GRAPH_H
#define GRAPH_H
#include <stdio.h>
#include <queue>
#include <vector>
#include <climits>
#include <algorithm>
#include <deque>
#include <numeric>
#include <cstring>
//#include <bitset>
#include <chrono>
#include <map>
//#include <unordered_map>
#include "lib_io.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ENDTIME 88.3
using namespace std;
const int verMax = 1205;
const int conMax = 512;
const int INF = 0x3F3F3F3F;


typedef pair<int, int> PAIR;


//class Edge{
//public:
//    int to, cost, bandwidth;
//    Edge(int t, int c, int cap):to(t), cost(c), bandwidth(cap){}
//    bool operator <(const Edge & other)const{
//        return this->cost < other.cost;
//    }
//};


struct Edge{
    int to, cost;
    Edge(int _to, int _cost)
        :to(_to), cost(_cost){}
};

struct Link{
    int to, availCap, cost, cap, tempCost;
    Link* next, *reve;
};

class ConsumeNode{
public:
    int netId, demand;
    ConsumeNode(int v, int need);
};


class Graph
{
public:
    Graph();
    char * res;
    void solver();
    void calCost();   //求费用矩阵
    void calPriority();
    void initSolution();
    void initGraph(char * topo[MAX_EDGE_NUM], int line_num);
private:
    int dist,dis[verMax + conMax];
    int offset,curOff,superSource,superTarget,mincost,maxCap;
    int totalDemand,verNum,conNum,edgeNum,flowCost;
    short code;  //状态编码 0000 分别表示up,down,move,delete
    Link buffer[MAX_EDGE_NUM<<2],*edge[verMax + conMax];
    static vector<int> priSet;
    //保存每条路径发出的流量
    vector <int> flowSet;
    //保存节点优先级的索引
    vector <int> priIndex;
    //保存每个网络节点发出的总流量
    vector <int> sumCap;
    //保存服务器部署的网络节点,按照优先级排序
//    vector <int> server;
    //保存网络节点的部署成本<netId：deploy cost>
    vector <int> netCost;

    vector<ConsumeNode> conList;
    vector <vector<int> > costTable;
    vector <vector<int> > pathSet;
    vector <Edge> edgeTable[verMax];

    //保存容量和硬件成本的映射关系<cap, hardware cost>
    map<int, int> capCost;
    //保存服务器容量和档次的映射关系<cap, rank>
    map<int, int> capRank;
    //服务器档次和提供流量的映射关系<rank, cap>
    map <int, int> rankCap;
    //可删除集合 <cost,netId>，
    map<int, int> deleteSet;
    //可移动集合<old,new>
    map<int, int> moveSet;
    //可增加集合<cost,netid>
    map<int, int> addSet;
    //可降级集合<>
    map<int, int> downSet;
    //可升级集合<>
    map<int, int> upSet;
    // 部署成本
    map<int, int> costSet;
//    // 服务器提供流量
//    map<int, int> flowMap;
    //保存服务器的部署位置和服务器容量的对应关系
    map<int, int> server;

    bool isDeploy[verMax];
    bool modLabel();
//    bool neightbor(int id);
    bool markSet[verMax + conMax];

    void resetServer();
    void setServer();
    void getNewServer(int loop);
    void getPath();
    void getRes();
    void getFlow();
    void dealSource();

    void addEdge(int from,int to,int flow,int cost);
    void updateDeleteSet();
    void updateMoveSet(int &mcost, PAIR &point);
    void updateAddSet();
    void updateDownSet();
    void upRankSet();
    void deleteServer();
    void moveServer();
    void addServer();
    void downServer();
    void upServer();

    int augment(int u,int c);
//    int calServerCost();
    int calRightCap(int flow);
//    int getHardwareCost(int flow);
    int MCMF();
    int minCostFlow();
};

class Timer{
public:
    static void tic(void){
        beginTime = chrono::system_clock::now();
    }
    static double toc(void){
        return chrono::duration_cast<chrono::duration<double>>(chrono::system_clock::now() - beginTime).count();
    }
    static bool timeout(void){
        return chrono::duration_cast<chrono::duration<double>>(chrono::system_clock::now() - beginTime).count() > ENDTIME;
    }
private:
    static chrono::time_point<chrono::system_clock> beginTime;
};
#endif // GRAPH_H


