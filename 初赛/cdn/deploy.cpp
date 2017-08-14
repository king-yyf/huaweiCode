#include "deploy.h"

chrono::time_point<chrono::system_clock> Timer::beginTime;
int Graph::serverCost;
vector <int> Graph::priSet;

//You need to complete the function 
void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{
	Timer::tic();
    Graph * pG = new Graph();
    pG->initGraph(topo,line_num);
    pG->calCost();
    pG->calPriority();
    pG->initSolution();
    pG->solver();
    write_result(pG->res, filename);
    //delete pG;
}

   
//           1               2              3
//case0    29580  28958    68020  67657   187652
//case1    29029  28935    63980  63519   201178 196683
//case2    29321  28371    65732  63775   196330 195232(440S) 202791
//case3    30585  30558    68549  67933   197334 196079
//case4    29895  29011    64557  63003   205167
//case5                    68508  67854
//case6
//case7                                   204222
//case8    29122  29306    70645  70468   202935(timeout)
