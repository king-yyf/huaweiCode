#include "graph.h"
#include <sstream>
//#include <cmath>
#include <functional>

Graph::Graph(): offset(0){
    //编码初始值1111,表示均为true
    mincost = INF;
    code = 31;
}

/*************************************************************************
*  函数名称：Graph::initGraph
*  功能说明：初始化数据读入，构造网络图
*  参数说明：topo为输入字符指针数组，line_num为字符数组行数
*  函数返回：无
*  修改时间：2017-4-9
*************************************************************************/
void Graph::initGraph(char *topo[], int line_num){
    sscanf(topo[0],"%d %d %d",&verNum, &edgeNum, &conNum);

    memset(edge,0,sizeof(edge));
    costTable.resize(conNum,vector <int>(verNum, INF));
    netCost.resize(verNum);
    priSet.resize(verNum, 0);
    sumCap.resize(verNum, 0);
    priIndex.resize(verNum, 0);

    //init server leval and cap, hardware fee.
    for(unsigned int i(2); strlen(topo[i]) > 2; ++i){
        int level, cap, cost;
        sscanf(topo[i], "%d %d %d", &level, &cap, &cost);
        capCost.emplace(cap,cost);
        capRank.emplace(cap,level);
        rankCap.emplace(level,cap);
    }

    int line = capCost.size() + 3;
    for(unsigned int i(line), j(line + verNum); i != j; ++i){
        int netId , deployCost;
        sscanf(topo[i], "%d %d", &netId, &deployCost);
        netCost[netId] = deployCost;
        costSet.emplace(deployCost,0);
    }

    line += verNum +1;
    //init edge table and calculate total bandwidth of every node
    for(unsigned int i(line),j(line + edgeNum);i != j; ++i){
        int head, tail, cap, cost;
        sscanf(topo[i], "%d %d %d %d", &head, &tail, &cap, &cost);

        edgeTable[head].emplace_back(tail, cost);
        addEdge(head, tail, cap, cost); sumCap[head] += cap;
        edgeTable[tail].emplace_back(head, cost);
        addEdge(tail, head, cap, cost); sumCap[tail] += cap;

    }
    //calculate consumer node total demand.
    totalDemand = 0;
    // initial super source node and super destination node
    superSource = verNum + conNum; superTarget = superSource + 1;
    line += edgeNum +1;
    for(int i(line);i < line_num; i++){
        int id, netId, need;
        sscanf(topo[i], "%d %d %d", &id, &netId, &need);
        conList.emplace_back(netId, need);
        addEdge(netId, id + verNum, need, 0);
        addEdge(id + verNum, superTarget, need, 0);
        totalDemand += need;
    }
    curOff=offset;
    map<int, int>::reverse_iterator rit = capCost.rbegin();
    maxCap = rit->first;
}

//initial consume node
ConsumeNode::ConsumeNode( int v, int need)
    :netId(v), demand(need)
{
    //limit = (Graph::serverCost) / demand;
}

/*************************************************************************
*  函数名称：Graph::calCost
*  功能说明：使用spfa算法计算每个消费节点到每个网络节点的最短路径距离
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-9
*************************************************************************/
void Graph::calCost(){
    for(int i = 0;i < conNum; ++i){
        int s = conList[i].netId;
        costTable[i][s] = 0;
        memset(markSet,0,sizeof(markSet));
        markSet[s] = true;
        deque <int> que;
        que.push_back(s);

        while(!que.empty()){
            int u = que.front();
//            bitvec.reset(u);
            markSet[u] = false;
            que.pop_front();
            for(size_t  k = 0; k < edgeTable[u].size(); ++k){

                int cost = costTable[i][u] + edgeTable[u][k].cost;
                int to = edgeTable[u][k].to;
                if(cost < costTable[i][to]){
                    costTable[i][to] = cost;
                    if(!markSet[to]){
                        if(que.empty())
                        {
                            que.push_front(to);
                        }
                        else
                        {
                            if(costTable[i][to] <= costTable[i][que.front()])
                                que.push_front(to);
                            else
                                que.push_back(to);
                        }
                        markSet[to] = true;    //bitvec.set(v);
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
*  修改时间：2017-4-19
*************************************************************************/
void Graph::resetServer(){
    dealSource();
    edge[superSource] = 0;
    for(auto & s : server){
        addEdge(superSource, s.first, maxCap, 0);
    }

}

/*************************************************************************
*  函数名称：Graph::setServer
*  功能说明：重新将server连接至超级源点,使用降级后的容量
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-20
*************************************************************************/
void Graph::setServer(){
    dealSource();
    edge[superSource] = 0;
    for(auto & s : server){
        addEdge(superSource, s.first, s.second, 0);
    }
}

/*************************************************************************
*  函数名称：Graph::addEdge
*  功能说明：在网络流中的添加正向边和反向边
*  参数说明：u，v：对偶顶点，flow：流量，cost：费用
*  函数返回：无
*  修改时间：2017-4-9
*************************************************************************/
void Graph::addEdge(int from, int to, int cap, int cost){

    Link * p1 = &buffer[offset];
    Link * p2 = p1 + 1;
    (* p1) = {to, cap, cost, cap, cost, edge[from], p2};
    (* p2) = {from, 0, -cost, 0, -cost, edge[to], p1};
    edge[from] = p1;
    edge[to] = p2;
    offset += 2;

}

/*************************************************************************
*  函数名称：Graph::dealSource
*  功能说明：对源节点进行重新连接
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-19
*************************************************************************/
void Graph::dealSource(){
    offset = curOff;
    Link * p = edge[superSource];
    while(p){
        edge[p->to] = edge[p->to]->next;
        p = p->next;
    }
    p = &buffer[offset];
    for(Link * e = buffer; e != p; ++e){
        e->cost = e->tempCost;
        e->availCap = e->cap;
    }
}

/*************************************************************************
*  函数名称：Graph::augment
*  功能说明：寻找增广路
*  参数说明：u:起点，c:流量
*  函数返回：增广流量
*  修改时间：2017-4-9
*************************************************************************/
int Graph::augment(int u, int c){
    if(u == superTarget)
        return flowCost += dist * c, c;
    int flow = c;
    markSet[u] = true;
    for(Link * e = edge[u]; e; e = e->next){
        if(e->availCap && !e->cost && !markSet[e->to]){
            int f = augment(e->to,MIN(flow, e->availCap));
            e->availCap -= f;
            e->reve->availCap += f;
            flow -= f;
            if(!flow) return c;
        }
    }
    return c - flow;
}

/*************************************************************************
*  函数名称：Graph::modLabel
*  功能说明：顶标修改
*  参数说明：无
*  函数返回：bool
*  修改时间：2017-4-19
*************************************************************************/
bool Graph::modLabel(){
    //bitset is slower than bool []

    memset(markSet, 0, sizeof(markSet));
    memset(dis, 0x3f, sizeof(dis));

    deque <int> deq;
    deq.push_back(superSource);
    dis[superSource] = 0;
  //  bitvec.set(src);
    markSet[superSource] = true;
    while(!deq.empty()){
        int u = deq.front();
        deq.pop_front();
        markSet[u] = false;   //bitvec.reset(u);
        for(Link *e = edge[u]; e; e = e->next){
            int to = e->to;
            if(e->availCap && dis[u] + e->cost < dis[to]){
                dis[to] = dis[u] + e->cost;
                if(markSet[to]) continue;
                //bitvec.set(v);
                if(deq.empty())
                {
                    deq.push_front(to);
                }else
                {
                    if(dis[to] < dis[deq.front()])
                        deq.push_front(to);
                    else
                        deq.push_back(to);
                }
                markSet[to] = true;
            }
        }
        markSet[u] = false;
    }

    for(Link *e = buffer, *p = & buffer[offset]; e != p; ++e)
        e->cost -= dis[e->to] - dis[e->reve->to];
    dist += dis[superTarget];

    return dis[superTarget] < INF;
}

/*************************************************************************
*  函数名称：Graph::getHardwareCost
*  功能说明：求某个流量对应服务器档次的硬件价格
*  参数说明：flow服务器节点实际提供的流量
*  函数返回：路径费用
*  修改时间：2017-4-10
*************************************************************************/
//int Graph::getHardwareCost(int flow){
//    map<int, int>::iterator it = capRank.lower_bound(flow);
//    return capCost[it->first];
//}

/*************************************************************************
*  函数名称：Graph::calRightCap
*  功能说明：对服务器发出的流量选择合适的档次
*  参数说明：flow服务器节点实际提供的流量
*  函数返回：路径费用
*  修改时间：2017-4-10
*************************************************************************/
int Graph::calRightCap(int flow){
    if(flow >= maxCap) return maxCap;
    auto it = capCost.lower_bound(flow);
    return it->first;
}

int Graph::MCMF(){
    setServer();
    int flow = 0;
    dist = flowCost = 0;
    while(modLabel()){
        int traffic(1);
        while(traffic){
            memset(markSet, 0, sizeof(markSet));
            traffic = augment(superSource, INT_MAX);
            flow += traffic;
        }
    }
    for(Link *e = edge[superSource]; e; e = e->next){
        int cap = calRightCap(e->cap - e->availCap);
  //      server.at(e->to) = calRightCap(e->cap - e->availCap);
        flowCost += capCost[server.at(e->to)] + netCost[e->to];

    }
    return (flow == totalDemand) ? flowCost : INF;
}

/*************************************************************************
*  函数名称：Graph::minCostFlow
*  功能说明：求解最小费用流
*  参数说明：无
*  函数返回：路径费用
*  修改时间：2017-4-10
*************************************************************************/
int Graph::minCostFlow(){
    //reset server node to super source
    resetServer();
    int flow = 0;
    dist = flowCost = 0;
    while(modLabel()){
        int traffic(1);   //any number but 0
        while(traffic){
           // bitvec.reset();
            memset(markSet, 0, sizeof(markSet));
            traffic = augment(superSource, INT_MAX);
            flow += traffic;
        }
    }
    for(Link *e = edge[superSource]; e; e = e->next){
//        server.at(e->to) = calRightCap(e->cap - e->availCap);
        flowCost += capCost[calRightCap(e->cap - e->availCap)] + netCost[e->to];
    }
    return (flow == totalDemand) ? flowCost : INF;
}

void Graph::getFlow(){
    for(Link *e = edge[superSource]; e; e = e->next){
        server.at(e->to) = calRightCap(e->cap - e->availCap);
    }
}

/*************************************************************************
*  函数名称：Graph::getPath
*  功能说明：获得最小费用路径及路径上的流量
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::getPath(){
    while (true) {
        vector <int> path;
        int s = superSource, traffic = INF;
        while (superSource != superTarget) {
            bool flag = false;
            for (Link *e = edge[s]; e; e = e->next) {
                if (e->cap > e->availCap) {
                    traffic = MIN(traffic, e->cap - e->availCap);
                    s = e->to;
                    flag = true; break;
                }
            }
            if (!flag) break;
        }
        if (s != superTarget) break;
        flowSet.push_back(traffic);
        s = superSource;
        while (s != superTarget) {
            for (Link *e = edge[s]; e; e = e->next) {
                if (e->cap > e->availCap) {
                    e->availCap += traffic;
                    s = e->to;  break;
                }
            }
            if (s != superTarget) path.push_back(s);
        }
        pathSet.push_back( move(path));
    }
}

/*************************************************************************
*  函数名称：Graph::getRes
*  功能说明：将路径和流量转化为输出结果的字符串
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::getRes(){
    stringstream txt;
    txt<<(int)pathSet.size()<<"\n\n";
    for(unsigned int i = 0;i < pathSet.size(); ++i){
        for(unsigned int j = 0;j < pathSet[i].size() - 1; j++){
            txt<<pathSet[i][j]<<" ";
        }
        int conId = pathSet[i][pathSet[i].size() - 1] - verNum;
        txt<<conId<<" ";
        txt<<flowSet[i]<<" ";
        txt<<capRank[server.at(pathSet[i][0])];
        if(i != pathSet.size()-1)
           txt<<"\n";
    }
    string res_str = txt.str();
    res = new char[res_str.size()];
    strcpy(res, res_str.c_str());
}

/*************************************************************************
*  函数名称：Graph::calServerCost
*  功能说明：求解由部署服务器和硬件成本带来的服务器总费用
*  参数说明：无
*  函数返回：int 服务器的总费用
*  修改时间：2017-4-10
*************************************************************************/
//int Graph::calServerCost(){
//    int totalCost = 0;
//    for(auto & s : server){
//        totalCost += netCost.at(s.first) + capCost.at(s.second);
//    }
//    return totalCost;
//}

/*************************************************************************
*  函数名称：Graph::calPriority
*  功能说明：按照一定规则求各网络节点的部署优先级
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::calPriority(){
    map<int, int>::iterator it = costSet.begin();
    int ref = 0, cost = it->first;
    while(it != costSet.end()){
        it->second = ++ref;
        ++it;
    }
    for(int i = 0; i < verNum; ++i){
        priIndex[i] = i;

        priority_queue<PAIR, vector<PAIR>, greater<PAIR>> que;
        for(int j = 0; j < conNum; ++j){
            if(costTable[j][i] < cost / conList[j].demand)
                que.emplace(costTable[j][i],j);
        }

        int need = 0, fee = 0;// upBound = MIN(sumCap[i], rankCap[rankCap.size() - 1]);
        while(!que.empty()){
            PAIR p = que.top();
            que.pop();
            need += MIN(sumCap[i] - need, conList[p.second].demand);
            fee += p.first;
            if(need == sumCap[i]) break;
        }
        double pri = (costSet[netCost[i]] * 1.0/ ref + 1) * fee;
//        printf("pri %d : %f\n",i,pri - fee);
        priSet[i] = need - floor(pri)  ;
    }
    costSet.clear();
    sort(priIndex.begin(), priIndex.end(), \
         [](const int &a,const int &b){return priSet[a] > priSet[b];});
}

/*************************************************************************
*  函数名称：Graph::neightbor
*  功能说明：计算一个网络节点相邻节点中有多少个节点部署了服务器
*  参数说明：int 网络节点
*  函数返回：bool 多于1个为真
*  修改时间：2017-4-18
*************************************************************************/
//bool Graph::neightbor(int id){
//    int cnt = 0;
//    for(unsigned int i = 0; i < edgeTable[id].size(); ++i){
//        if(isDeploy[edgeTable[id][i].to]){
//            cnt++;
//            if(cnt > 1) break;
//        }
//    }
//    return (cnt > 1) ? true : false;
//}

/*************************************************************************
*  函数名称：Graph::initSolution
*  功能说明：初始化解决方案
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-18
*************************************************************************/
void Graph:: initSolution(){
    memset(isDeploy,0,sizeof(isDeploy));
    priority_queue<PAIR, vector<PAIR>,greater<PAIR>> serverPos;
    for(int i = 0; i < conNum; ++i){
        int netid = conList[i].netId;
        if(conList[i].demand <= (totalDemand>>2) / conNum && priSet[netid] < priIndex[conNum])
            continue;
        serverPos.emplace(priSet[netid],netid);
        server.emplace(netid,0);
        isDeploy[netid] = true;
    }
    for(int i = 0; i < (conNum>>2); ++i){
        int id = priIndex[i];
        if(!isDeploy[id]){
            serverPos.emplace(priSet[id],id);
            server.emplace(id,0);
            isDeploy[id] = true;
        }
    }

    while(!serverPos.empty()){
        PAIR p = serverPos.top();
        serverPos.pop();
        server.erase(p.second);
        int cost = minCostFlow();
        if(cost < mincost){
            mincost = cost;
            isDeploy[p.second] = false;
        }else
            server.emplace(p.second,0);
    }
    minCostFlow();
    getFlow();
//    printf("mincost: %d\n", mincost);

}

//void Graph::initSolution(){
//    memset(isDeploy,0,sizeof(isDeploy));
//    priority_queue<PAIR, vector<PAIR>, greater<PAIR>> serverPos;

//    for(int i = 0; i < conNum; ++i){
//        int id = priIndex[i], netId = conList[i].netId;
//        int cap = rankCap[rankCap.size() - 1];
//        int cap1(cap);
//        if(sumCap[id] < cap){
//            auto it = capCost.lower_bound(sumCap[id]);
//            cap1 = it->first;
//        }
//        if(sumCap[netId] + conList[i].demand < cap){
//            auto it = capCost.lower_bound(sumCap[netId] + conList[i].demand);
//            cap = it->first;
//        }
//        serverPos.emplace(priSet[id],id);
//        serverPos.emplace(priSet[netId],netId);
//        server.emplace(netId,cap);
//        server.emplace(id,cap1);
//        isDeploy[id] = true;
//        isDeploy[netId] = true;
//    }
//    //delete server to cut down cost
//    while(!serverPos.empty()){
//        PAIR p = serverPos.top();
//        serverPos.pop();
//        if(!isDeploy[p.second]) continue;
//        int flow = server.at(p.second);
//        server.erase(p.second);
//        int cost = minCostFlow();
//        if(netCost[p.second] + capCost[flow] +mincost > cost){
//            mincost = cost;
//            isDeploy[p.second] = false;
//        }else{
//            server.emplace(p.second,flow);
//        }
//    }

//}

/*************************************************************************
*  函数名称：Graph::updateAddSet
*  功能说明：将添加后能使费用比当前费用小的节点放入
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::updateAddSet(){
    addSet.clear();
    for(int i = 0; i < conNum && !Timer::timeout(); ++i){
        int id = priIndex[i];
        if(isDeploy[id]) continue;
        server.emplace(id, maxCap);
        int cost = MCMF();
        server.erase(id);
        if(cost < mincost){
            addSet.emplace(cost,id);
        }
    }
    if(addSet.empty())
        code &= (~1);
    else
        code |= 1;
}

/*************************************************************************
*  函数名称：Graph::updateDeleteSet
*  功能说明：更新删除后能使费用降低的服务器节点
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::updateDeleteSet(){
    deleteSet.clear();
    for(auto it = server.begin(); it != server.end() && !Timer::timeout();){
        int fi = it->first,se = it->second;
        server.erase(it++);
        int cost = MCMF();
        server.emplace(fi, se);
        if(cost < mincost){
            deleteSet.emplace(cost, fi);
        }
    }
    if(deleteSet.empty())
        code &= (~2);
    else
        code |= 2;
}

/*************************************************************************
*  函数名称：Graph::updateMoveSet
*  功能说明：更新移动到某个相邻节点后后能使费用降低的服务器节点
*  参数说明：mcost，使费用降到最低的费用值，PAIR p:从first节点移到second节点
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::updateMoveSet(int & mcost, PAIR & point){
    moveSet.clear();
    map<int, int>::iterator it = server.begin();
    map<int, int>::iterator save;
    while(it != server.end() && !Timer::timeout()){
        int fi = it->first,se = it->second;
        save = it++;
        server.erase(save);
//
        for(unsigned int i = 0; i < edgeTable[fi].size() && !Timer::timeout(); ++i){
            int to = edgeTable[fi][i].to;
            if(isDeploy[to] || priSet[to] < priSet[priIndex[verNum>>1]]) continue;
         //   if(sumCap[to] < sumCap[fi] - 25) continue;
            server.emplace(to, maxCap);
            int cost = MCMF();
            server.erase(to);
            if(cost < mincost){
                moveSet.emplace(fi,to);
                if(cost < mcost){
                    point.first = fi;
                    point.second = to;
                    mcost = cost;
                }
                break;
            }
        }
        server.emplace(fi, se);
    }
    if(moveSet.empty())
        code &= (~4);
    else
        code |= 4;
}

/*************************************************************************
*  函数名称：Graph::updateDownSet
*  功能说明：更新对服务器进行降级后能使费用减少的节点
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-21
*************************************************************************/
void Graph::updateDownSet(){
    downSet.clear();
    for(auto it = server.begin(); it != server.end() && !Timer::timeout(); it++){
        if(capRank[it->second] == 0) continue;
        int cap = it->second;
        it->second = rankCap[capRank[it->second] - 1];
        int cost = MCMF();
        it->second = cap;
        if(cost < mincost){
            downSet.emplace(cost,it->first);
        }
    }
    if(downSet.empty())
        code &= (~8);
    else
        code |= 8;
}

void Graph::upRankSet(){
    upSet.clear();
    for(auto it = server.begin(); it != server.end() && !Timer::timeout(); it++){
        if(it->second == maxCap) continue;
        int cap = it->second;
        it->second = rankCap[capRank[it->second] + 1];
        int cost = MCMF();
        it->second = cap;
        if(cost < mincost){
            upSet.emplace(cost,it->first);
        }
    }
    if(upSet.empty())
        code &= (~16);
    else
        code |= 16;
}

/*************************************************************************
*  函数名称：Graph::addServer
*  功能说明：增加一个服务器节点以减少总费用
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-20
*************************************************************************/
void Graph::addServer(){
 //   printf("add\n");
    updateAddSet();
    if(code % 2 == 0) return;
    map<int, int>::iterator it = addSet.begin();
    server.emplace(it->second,maxCap);
    mincost = it->first;
    isDeploy[it->second] = true;
   // addSet.erase(it++);
    if((int)addSet.size() > 1){
        ++it;
        while(it != addSet.end()){
            server.emplace(it->second,maxCap);
            int cost = MCMF();
            if(mincost > cost){
                mincost = cost;
                isDeploy[it->second] = true;
            }else
                server.erase(it->second);
            ++it;
        }

    }
}

/*************************************************************************
*  函数名称：Graph::deleteServer
*  功能说明：删除某个或某些服务器节点以减少总费用
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-20
*************************************************************************/
void Graph::deleteServer(){
 //   printf("delete\n");
    updateDeleteSet();
    if((code>>1) % 2 == 0) return;
    map<int, int>::iterator it = deleteSet.begin();
    mincost = it->first;
    server.erase(it->second);
    isDeploy[it->second] = false;
    if((int)deleteSet.size() > 1){
        ++it;
        while(it != deleteSet.end()){
            int cap = server.at(it->second);
            server.erase(it->second);
            int cost = MCMF();
            if(cost < mincost){
                mincost = cost;
                isDeploy[it->second] = false;
            }else
                server.emplace(it->second, cap);
            ++it;
        }
    }
}

/*************************************************************************
*  函数名称：Graph::moveServer
*  功能说明：移动某个或某些服务器节点以减少总费用
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-20
*************************************************************************/
void Graph::moveServer(){
 //   printf("move\n");
    int mcost = INF;
    PAIR point;
    updateMoveSet(mcost,point);
    if((code>>2) % 2 == 0) return;
    int cap = server.at(point.first);
    server.erase(point.first);
    server.emplace(point.second, cap);
    isDeploy[point.first] = false;
    isDeploy[point.second] = true;
    mincost = mcost;
    if(moveSet.size() > 1){
        moveSet.erase(point.first);
        for(map<int, int>::iterator it = moveSet.begin(); it != moveSet.end(); ++it){
            int cap = server.at(it->first);
            server.erase(it->first);
            server.emplace(it->second,cap);
            int cost = MCMF();
            if(cost < mincost){
                mincost = cost;
                isDeploy[it->first] = false;
                isDeploy[it->second] = true;
            }else{
                server.erase(it->second);
                server.emplace(it->first, cap);
            }
        }
    }
}

void Graph::downServer(){
//    printf("down\n");
    updateDownSet();
    if((code>>3) % 2 == 0) return;
    auto it = downSet.begin();
    int cap = server.at(it->second);
    cap = rankCap[capRank[cap] - 1];
    server[it->second] = cap;
    mincost = it->first;
}

void Graph::upServer(){
//    printf("up\n");
    upRankSet();
    if((code>>4) % 2 == 0) return;
    auto it = upSet.begin();
    int cap = server.at(it->second);
    cap = rankCap[capRank[cap] + 1];
    server.at(it->second) = cap;
    mincost = it->first;
}

/*************************************************************************
*  函数名称：Graph::getNewServer
*  功能说明：获得新的且使费用减小的服务器部署方案
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-20
*************************************************************************/
void Graph::getNewServer(int loop){
    switch(loop % 5){
    case 0:
        addServer();
        break;
    case 1:
        moveServer();
        break;
    case 2:
       // addServer();
        upServer();
        break;
    case 3:
        downServer();
        break;
    case 4:
        deleteServer();
    }
}

/*************************************************************************
*  函数名称：Graph::solver
*  功能说明：迭代求解
*  参数说明：无
*  函数返回：无
*  修改时间：2017-4-10
*************************************************************************/
void Graph::solver(){
    int loop = 0;
        while(!Timer::timeout() && code != 0){
            getNewServer(loop);
    //        printf("cost :%d\n",mincost );
            loop++;
        }
        MCMF();
        getPath();
        getRes();
//        for(auto & s : server){
//            printf("id: %d, cap: %d\n",s.first,s.second);
//        }
     //   printf("cost :%d\n",mincost );
}
