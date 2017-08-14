#ifndef GRAPH_H
#define GRAPH_H
#include <stdio.h>
#include <queue>
#include <vector>
#include <climits>
#include <algorithm>
#include <deque>
#include <cstring>
#include <chrono>
#include "lib_io.h"
#define MIN(a,b) ((a)<(b)?(a):(b))
#define ENDTIME 88.5
using namespace std;
const int verMax = 2010;
const int INF = 0x3F3F3F3F;

class cMap{
public:
    int first, second;
    cMap(int i, int j);
    bool operator <(const cMap & other)const{
        return this->second < other.second;
    }
};

class Edge{
public:
    int to, cost, bandwidth;
    Edge(int t, int c, int cap):to(t), cost(c), bandwidth(cap){}
    bool operator <(const Edge & other)const{
        return this->cost < other.cost;
    }
};

struct Link{
    int to, flow, cost, cap, exes;
    Link* next, *pair;
};

class ConsumeNode{
public:
    int netId, demand, limit;
    ConsumeNode(int v, int need);
};

class Graph
{
public:
    Graph();
    char * res;
    int totalDemand,verNum,conNum,edgeNum,flowCost;
    static int serverCost;
    short code;  //状态编码
    vector <int > server;
    vector<ConsumeNode> conList;
    int minCostFlow();
    void solver();
    void calCost();   //求费用矩阵
    void calPriority();
    void initSolution();
    void initGraph(char * topo[MAX_EDGE_NUM], int line_num);
private:
    int dist,dis[verMax];
    int offset,curOff,src,des,mustNum,mincost;
    Link buffer[MAX_EDGE_NUM<<2],*edge[verMax];
    static vector<int> priSet;
    vector<int> flow;
    vector<int> priIndex;
    vector<int >sumCap;
    //vector<int >curBest;
    vector<cMap> moveSet;
    bool finalOpt();
    vector<cMap> addSet;
    vector<cMap> deleteSet;
    vector <vector<int> > costTable;
    vector <vector<int> > pathSet;
    vector<Edge> edgeTable[verMax>>1];
    bool isDeploy[verMax>>1];
    bool visit[verMax];
    void updateAddSet();
    void updateDelSet();
    void updateMoveSet();
    void moveServer();
    void addServer();
    void deleteServer();
    bool modLabel();
    void getNewServer( int flag);
    void resetServer();
    void getFlow();
    void getRes();
    void addEdge(int u,int v,int flow,int cost);
    int augment(int u,int c);

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


