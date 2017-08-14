#include "graph.h"
#include <sstream>
#include <cmath>
#include <functional>
cMap::cMap(int i, int j):first(i), second(j){

}

Graph::Graph(): offset(0){
    mustNum = 0;
    mincost = 0;
    code = 7;
}

/*************************************************************************
*  函数名称：Graph::initGraph
*  功能说明：初始化数据读入，构造网络图
*  参数说明：topo为输入字符指针数组，line_num为字符数组行数
*  函数返回：无
*  修改时间：2017-3-6
*************************************************************************/
void Graph::initGraph(char *topo[], int line_num){
    sscanf(topo[0],"%d %d %d",&verNum, &edgeNum, &conNum);
    sscanf(topo[2], "%d", &serverCost);

    memset(edge,0,sizeof(edge));
    costTable.resize(conNum,vector <int>(verNum, INF));
    sumCap.resize(verNum, 0);
    priSet.resize(verNum, 0);
    priIndex.resize(verNum, 0);

    //init edge table and calculate total bandwidth of every node
    for(unsigned int i(4),j(4 + edgeNum);i != j; ++i){
        int head, tail, cap, cost;
        sscanf(topo[i], "%d %d %d %d", &head, &tail, &cap, &cost);

        edgeTable[head].emplace_back(tail, cost, cap);
        sumCap[head] += cap; addEdge(head, tail, cap, cost);
        edgeTable[tail].emplace_back(head, cost, cap);
        sumCap[tail] += cap; addEdge(tail, head, cap, cost);

    }
    //calculate consumer node total demand.
    totalDemand = 0;
    // initial super source node and super destination node
    src = verNum + conNum; des = src + 1;

    for(int i(5 + edgeNum);i < line_num; i++){
        int id,netId,need;
        sscanf(topo[i], "%d %d %d", &id, &netId, &need);
        conList.emplace_back(netId, need);
        addEdge(netId,id + verNum,need,0);
        addEdge(id + verNum,des,need,0);
        totalDemand += need;
    }
    curOff=offset;
}

//initial consume node
ConsumeNode::ConsumeNode( int v, int need)
    :netId(v), demand(need)
{
    limit = (Graph::serverCost) / demand;
}

/*************************************************************************
*  函数名称：Graph::calCost
*  功能说明：使用spfa算法计算每个消费节点到每个网络节点的最短路径距离
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-8
*************************************************************************/
void Graph::calCost(){
    for(int i = 0;i < conNum; ++i){
        int s = conList[i].netId;
        costTable[i][s] = 0;
        memset(visit, 0, sizeof(visit));
        visit[s] = true;
        deque <int> que;
        que.push_back(s);

        while(!que.empty()){
            int u = que.front();
            visit[u] = false;
            que.pop_front();
            for(unsigned int k = 0;k < edgeTable[u].size(); ++k){

                int cost = costTable[i][u] + edgeTable[u][k].cost;
                int v = edgeTable[u][k].to;
                if(cost < costTable[i][v]){
                    costTable[i][v] = cost;
                    if(!visit[v]){
                        visit[v] = true;
                        if(!que.empty() && costTable[i][v] < costTable[i][que[0]])
                            que.push_back(v);
                        else
                            que.emplace_front(v);
                    }
                }
            }
        }
    }
}

/*************************************************************************
*  函数名称：Graph::resetServer
*  功能说明：求得server解改变时，重新将server连接至超级源点
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-8
*************************************************************************/
void Graph::resetServer(){
    if(offset != curOff){
        offset = curOff;
        for(Link * p = edge[src]; p; p = p->next)
            edge[p->to] = edge[p->to]->next;

        edge[src]=0;
        Link * pLink = buffer + offset;
        for(Link * e = buffer; e < pLink; ++e){
            e->flow = e->cap;
            e->cost = e->exes;
        }
    }
    for(size_t i = 0;i < server.size(); ++i)
        addEdge(src, server[i], INF, 0);
}

/*************************************************************************
*  函数名称：Graph::addEdge
*  功能说明：在网络流中的添加两条对偶边
*  参数说明：u，v：对偶顶点，flow：流量，cost：费用
*  函数返回：无
*  修改时间：2017-3-8
*************************************************************************/
void Graph::addEdge(int from, int to, int cap, int cost){

    Link * p1 = buffer + offset++;
    Link * p2 = buffer + offset++;

    *p1 = (Link){to, cap, cost, cap, cost, edge[from], p2};
    edge[from] = p1;
    *p2=(Link){from, 0, -cost, 0, -cost, edge[to], p1};
    edge[to] = p2;

}

/*************************************************************************
*  函数名称：Graph::augment
*  功能说明：寻找增广路
*  参数说明：u:起点，c:流量
*  函数返回：增广流量
*  修改时间：2017-3-9
*************************************************************************/
int Graph::augment(int u, int c){
    if(u == des)
        return flowCost += dist * c, c;
    int t = c;
    visit[u] = true;
    for(Link * e = edge[u]; e; e = e->next){
        if(e->flow && !e->cost && !visit[e->to]){
            int f = augment(e->to, min(t, e->flow));
            e->flow -= f;
            e->pair->flow += f;
            t -= f;
            if(!t) return c;
        }
    }
    return c - t;
}

/*************************************************************************
*  函数名称：Graph::modLabel
*  功能说明：顶标修改
*  参数说明：无
*  函数返回：bool
*  修改时间：2017-3-9
*************************************************************************/
bool Graph::modLabel(){
    memset(visit, 0, sizeof(visit));
    memset(dis, 0x3f, sizeof(dis));

    deque <int> deq;
    deq.push_back(src);
    dis[src] = 0;
    visit[src] = true;

    while(!deq.empty()){
        int u = deq.front();
        deq.pop_front();
        visit[u] = false;
        for(Link *e = edge[u]; e; e = e->next){
            int v = e->to;
            if(e->flow && dis[u] + e->cost < dis[v]){
                dis[v] = dis[u] + e->cost;
                if(visit[v]) continue;
                visit[v] = true;
                if(!deq.empty() && dis[v] < dis[deq[0]])
                    deq.emplace_front(v);
                else
                    deq.push_back(v);
            }
        }
    }

    for(Link *e = buffer;e < buffer + offset; ++e)
        e->cost -= dis[e->to] - dis[e->pair->to];
    dist += dis[des];

    return dis[des]<INF;
}

/*************************************************************************
*  函数名称：Graph::minCostFlow
*  功能说明：求解最小费用流
*  参数说明：无
*  函数返回：路径费用
*  修改时间：2017-3-11
*************************************************************************/
int Graph::minCostFlow(){
    //reset server node to super source
    resetServer();
    int flow = 0;
    dist = flowCost = 0;
    while(modLabel()){
        int tmp;
        do{
            memset(visit, 0, sizeof(visit));
            tmp = augment(src, INT_MAX);
            flow += tmp;
        }while(tmp);
    }
    if(flow !=totalDemand)
        return INF;
    return flowCost;
}

/*************************************************************************
*  函数名称：Graph::getFlow
*  功能说明：获得最小费用路径及路径上的流量
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-14
*************************************************************************/
void Graph::getFlow(){
    pathSet.clear();
    flow.clear();
    while (true) {
        vector <int> path;
        int s = src, traffic = INF;
        while (src != des) {
            bool flag = false;
            for (Link *e = edge[s]; e; e = e->next) {
                if (e->cap > e->flow) {
                    traffic = min(traffic, e->cap - e->flow);
                    s = e->to;
                    flag = true; break;
                }
            }
            if (!flag) break;
        }
        if (s != des) break;
        flow.push_back(traffic);
        s = src;
        while (s != des) {
            for (Link *e = edge[s]; e; e = e->next) {
                int v = e->to;
                if (e->cap > e->flow) {
                    e->flow += traffic;
                    s = v;  break;
                }
            }
            if (s != des) path.push_back(s);
        }
        pathSet.push_back( move(path));
    }
}

/*************************************************************************
*  函数名称：Graph::getRes
*  功能说明：将路径和流量转化为输出结果的字符串
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-16
*************************************************************************/
void Graph::getRes(){
    stringstream txt;
    txt<<(int)pathSet.size()<<"\n\n";
    for(unsigned int i=0;i<pathSet.size();++i){
        for(unsigned int j=0;j<pathSet[i].size()-1;j++){
            txt<<pathSet[i][j]<<" ";
        }
        int conId=pathSet[i][pathSet[i].size()-1]-verNum;
        txt<<conId<<" ";
        txt<<flow[i];
        if(i!=pathSet.size()-1)
           txt<<"\n";
    }
    string res_str=txt.str();
    res = new char[res_str.size()];
    strcpy(res,res_str.c_str());
}

/*************************************************************************
*  函数名称：Graph::calPriority
*  功能说明：根据每个节点能对网络做出的贡献，计算每个网络节点的优先级
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-20
*************************************************************************/
void Graph::calPriority(){
    server.clear();
    memset(isDeploy, 0, sizeof(isDeploy));

    //对每个消费节点,判断是否必须在相连网络节点部署一台服务器
     for(int i = 0;i < conNum; ++i){
         int id = conList[i].netId;
         server.push_back(id);
         isDeploy[id] = true;
         if(sumCap[id ] < conList[i].demand){
             priSet[id] = INF;
             mustNum++;
         } else{
             sort(edgeTable[id].begin(), edgeTable[id].end());
             int needFlow = conList[i].demand, totalCost = 0;
             for(size_t k = 0;k < edgeTable[id].size(); ++k){
                 int useBandwidth = MIN(needFlow, edgeTable[id][k].bandwidth);
                 totalCost += useBandwidth * edgeTable[id][k].cost;
                 needFlow -= useBandwidth;
                 if(needFlow == 0) break;
             }
             if(totalCost >= serverCost){
                 priSet[id] = INF;
                 mustNum++;
             }
         }
     }
     //按照一定规则求每个网络节点的优先级
     for(int j = 0; j < verNum; ++j){
         priIndex[j] = j;
         if(priSet[j] == INF)  continue;
         double need = 0.0;
         bool flag = 0;
         for(int i = 0; i < conNum; ++i){
             if(priSet[conList[i].netId] == INF)
                 continue;
             if(costTable[i][j] <= conList[i].limit){
               need += costTable[i][j] == 0 ? conList[i].demand:conList[i].demand / sqrt(costTable[i][j]<<1);
               flag = true;
             }
         }
         if(flag) {
             int pri =  MIN((int)floor(need+0.3), sumCap[j]);
             if(priSet[j] != INF)  priSet[j] = move(pri);
         }
     }
     sort(priIndex.begin(), priIndex.end(), [](const int &a,const int &b){return priSet[a] > priSet[b];});
}

/*************************************************************************
*  函数名称：Graph::initSolution
*  功能说明：初始化解决方案，从直连删除一些非必需节点
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-26
*************************************************************************/
void Graph::initSolution(){
    sort(server.begin(), server.end(), [](const int &a,const int &b){return priSet[a] > priSet[b];});

    for(int i = conNum - 1;i >= mustNum; --i){
        int id = server[i];
        server.erase(server.begin() + i);
        int cost = minCostFlow();
        if(mincost + serverCost > cost){
            isDeploy[id] = false;
            mincost = cost;
        }else
            server.push_back(id);
    }
}

/*************************************************************************
*  函数名称：Graph::updateAddSet
*  功能说明：更新可增加节点
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-28
*************************************************************************/
void Graph::updateAddSet(){
    addSet.clear();
    if(verNum>500){
        for(int i = mustNum; i < conNum && !Timer::timeout() ;++i){
            if(isDeploy[priIndex[i]]) continue;
            server.push_back(priIndex[i]);
            int cost = minCostFlow();
            server.pop_back();
            if(mincost > cost + serverCost)
                addSet.emplace_back(priIndex[i],cost);
        }
    }else{
        for(int i = mustNum ;i  <verNum; ++i){
            if(isDeploy[i]) continue;
            server.push_back(i);
            int cost = minCostFlow();
            server.pop_back();
            if(mincost > cost + serverCost)
                addSet.emplace_back(i, cost);
        }
    }
    //return !addSet.empty();
    if(addSet.empty())
        code &= (~1);
    else
        code |= 1;
}

/*************************************************************************
*  函数名称：Graph::updateDelSet
*  功能说明：更新可删除服务器节点集合
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-28
*************************************************************************/
void Graph::updateDelSet(){
    deleteSet.clear();
    for(int i = server.size()-1;i >= mustNum && !Timer::timeout(); --i){
        int id = server[i];
        if(i == (int)server.size() -  1){
            server.pop_back();
        }else{
            iter_swap(server.begin() + i, server.end() - 1);
            server.pop_back();
        }
        int cost = minCostFlow();
        server.push_back(id);
        if(mincost + serverCost > cost)
            deleteSet.emplace_back(id,cost);
    }
    //return !deleteSet.empty();
    if(deleteSet.empty())
        code &= (~2);
    else
        code |= 2;
}

/*************************************************************************
*  函数名称：Graph::updateMoveSet
*  功能说明：更新可移动服务器节点集合
*  参数说明：无
*  函数返回：无
*  修改时间：2017-3-28
*************************************************************************/
void Graph::updateMoveSet(){
    moveSet.clear();
    int idx1 = mustNum,idx2 = mustNum, mcost = INF;
    for(size_t s = mustNum; s < server.size(); ++s){
        for(size_t i = 0;i < edgeTable[server[s]].size() && !Timer::timeout(); ++i){
            int id = edgeTable[server[s]][i].to;
            if(verNum > 500 && priSet[id] < priSet[priIndex[verNum>>1]]) continue;
            int old = server[s];
            server[s] = id;
            int cost = minCostFlow();
            server[s] = old;
            if(cost < mincost){
                moveSet.emplace_back(old,id);
                if(cost < mcost){
                    mcost = cost;
                    idx1 = old;
                    idx2 = id;
                }
            }
        }
    }
 /*   if(moveSet.empty())
    return false;
    else{
        moveSet.emplace_back(idx1,idx2);
    }
    return true;*/
    if(moveSet.empty())
        code &= (~4);
    else{
        code |= 4;
        moveSet.emplace_back(idx1,idx2);
    }

}

/*************************************************************************
*  函数名称：Graph::deleteServer
*  功能说明：从待删除节点集合中删除服务器节点，以减小总代价
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-1
*************************************************************************/
void Graph::deleteServer(){
    updateDelSet();
    if((code>>1) % 2 == 0) return;
    if((int)deleteSet.size() > 5){
        sort(deleteSet.begin(), deleteSet.end());
        for(size_t i = 0;i < deleteSet.size(); ++i){
            server.erase(remove(server.begin(), server.end(), deleteSet[i].first), server.end());
            int cost = minCostFlow();
            if(cost < serverCost + mincost){
                mincost = cost;
                isDeploy[deleteSet[i].first] = false;
            }else
				server.push_back(deleteSet[i].first);
        }
        return;
    }
    int id = 0,micost = INF;
    for(unsigned int i = 0;i < deleteSet.size(); ++i){
        if(deleteSet[i].second < micost){
            micost = deleteSet[i].second;
            id = i;
        }
    }
    isDeploy[deleteSet[id].first] = false;
    server.erase(remove(server.begin(), server.end() ,deleteSet[id].first), server.end());
    mincost = minCostFlow();

}

/*************************************************************************
*  函数名称：Graph::addServer
*  功能说明：从可增加集合中增加服务器节点，以减小总代价
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-1
*************************************************************************/
void Graph::addServer(){

    updateAddSet();
    if(code % 2 == 0) return ;//false;
    if((int)addSet.size() > 5){
        sort(addSet.begin(), addSet.end());
        for(size_t i = 0;i < addSet.size(); ++i){
            server.push_back(addSet[i].first);
            int cost = minCostFlow();
            if(cost + serverCost < mincost){
                mincost = cost;
                isDeploy[addSet[i].first] = true;
                addSet.erase(addSet.begin() + i);
            }else
                server.pop_back();
        }
        return ;
    }
    int id = 0,micost = INF;
    for(unsigned int i = 0;i < addSet.size(); ++i){
        if(addSet[i].second < micost){
            micost = addSet[i].second;
            id = i;
        }
    }
    isDeploy[addSet[id].first] = true;
    server.push_back(addSet[id].first);
    mincost = minCostFlow();

}

/*************************************************************************
*  函数名称：Graph::addServer
*  功能说明：从可移动节点集合中移动服务器节点，以减小总代价
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-1
*************************************************************************/
void Graph::moveServer(){
    updateMoveSet();
    if((code>>2) % 2 == 0) return ;
    if(moveSet.size() - 1 > 5){
        sort(moveSet.begin(), moveSet.end(), [](const cMap &a,const cMap &b){return priSet[a.second]>priSet[b.second];});
        for(unsigned int i = 0;i < moveSet.size() && !Timer::timeout(); ++i){
            if(!isDeploy[moveSet[i].first] || isDeploy[moveSet[i].second]) continue;
            vector <int> ::iterator it = find(server.begin(), server.end(), moveSet[i].first);
            *it = moveSet[i].second;
            int cost = minCostFlow();
            if(cost < mincost){
                mincost = cost;
                isDeploy[moveSet[i].first ] = false;
                isDeploy[moveSet[i].second] = true;
            }else{
                *it = moveSet[i].first;
            }
        }
        return ;
    }
    int size = moveSet.size() - 1;
    vector <int> ::iterator it = find(server.begin(), server.end(), moveSet[size].first);
    *it = moveSet[size].second;
    isDeploy[moveSet[size].first] = false;
    isDeploy[moveSet[size].second] = true;
    mincost = minCostFlow();

}

/*************************************************************************
*  函数名称：Graph::getNewServer
*  功能说明：获得新的服务器状态
*  参数说明：flag: 传入参数，指导当前选择增加，删除或移动服务器
*  函数返回：无
*  修改时间：2017-4-2
*************************************************************************/
void Graph::getNewServer(int flag){
    switch(flag % 3){
    case 0:
        addServer();
        break;
    case 1:
        moveServer();
        break;
    case 2:
        deleteServer();
        break;
    }
}

/*************************************************************************
*  函数名称：Graph::solver
*  功能说明：迭代求解近似最优解
*  参数说明：flag: 传入参数，指导当前选择增加，删除或移动服务器
*  函数返回：无
*  修改时间：2017-4-4
*************************************************************************/
void Graph::solver(){
    int loop = 0;
    while(!Timer::timeout() && code != 0){
        getNewServer(loop);
        printf("cost :%lu\n",mincost + server.size() * serverCost);
        loop++;
    }
    minCostFlow();
    getFlow();
    getRes();
}
