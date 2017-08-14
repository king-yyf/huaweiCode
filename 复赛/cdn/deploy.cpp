#include "deploy.h"

chrono::time_point<chrono::system_clock> Timer::beginTime;

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
//    printf("here\n");
    pG->solver();
    write_result(pG->res, filename);
    //delete pG;
}

   
//              1                  2
//case0    217788  212668      416650  391***
//case1    185117  184467      95708  93853
//case2    56172  56172       99707  99223
//case3    50768  47689       100738 102244
//case4    54819  55118            63003
//case5
//case6
//case7
//case8    53491  42466       108904  70468
//case9

//初始解          1                           2
//case0    221906(54)   198627      434141(109)  387126
//case1    197887(38)   176785      407754(90)   93853
//case2    240918       56172       99707        99223
//case3    50768        47689       100738       102244
//case4    54819        55118
//case5
//case6
//case7
//case8    53491  42466       108904  70468
//case9    236444
