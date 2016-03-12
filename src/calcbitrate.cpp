#include <vector>
#include <iostream>
#include "math.h"
#include "calcbitrate.h"
#include <QDebug>

using std::vector;
//using std::abs;
using std::max;

std::vector<Reg> r5k;
std::vector<Reg> r1m;

int addItem() {
    int ret=0;
    Reg *pReg;
    for(int BRP=0; BRP<256; BRP++)
        for(int TSG1=0; TSG1<16; TSG1++)
            for(int TSG2=0; TSG2<8; TSG2++)
            {
        int temp=(TSG1+1)+(TSG2+1)+1;
        if(temp>=8 && temp<=25 && TSG1>TSG2 && TSG2>=1) {
            long tmpBR=fclk_main/(2*(BRP+1)*((TSG1+1)+(TSG2+1)+1));

            if(tmpBR > BR5K*(1-BRBIAS) && tmpBR < BR5K*(1+BRBIAS)) {
                pReg=new Reg(BRP, TSG1, TSG2, tmpBR);
                r5k.push_back(*pReg);

                delete pReg;
                pReg=NULL;
            }

            if(tmpBR > BR1M*(1-BRBIAS) && tmpBR < BR1M*(1+BRBIAS)) {
                pReg=new Reg(BRP, TSG1, TSG2, tmpBR);
                r1m.push_back(*pReg);

                delete pReg;
                pReg=NULL;
            }
        }
    }

    if(r1m.size()==0 || r5k.size()==0)
        ret=1;// flag

    qDebug() << "r1m.size(): " << r1m.size() << "  r5k.size(): " << r5k.size() << endl;

    return ret;
}

Reg selectReg(long realBR, int realSP) {
    vector<Reg>::iterator b, e;
    float BRp, SPp;
    Reg ret;

    if(r1m.size()==0 || r5k.size()==0) {
        // error msg
        return ret;
    }

    if(abs(realBR - BR5K) < 1) {
        b=r5k.begin();
        e=r5k.end();
    }
    else if(abs(realBR - BR1M) < 1) {
        b=r1m.begin();
        e=r1m.end();
    }
    else
    {}

    for(vector<Reg>::iterator iter=b; iter != e; iter++) {
        float sum=2.0, sumTmp;

        BRp=(float)abs(iter->bitRate-realBR)/max(iter->bitRate, realBR);
        SPp=(float)abs(iter->samplePos-realSP)/max(iter->samplePos, realSP);

        sumTmp=BRp+SPp;
        if(sumTmp < sum) {
            sum=sumTmp;
            ret=*iter;
        }
    }
    return ret;
}
/*
#include <ctime>
int main()
{
    time_t start, end;
    start = clock();
    addItem();
    long realBR=1000000;
    int realSP=50;
    Reg testReg=selectReg(realBR, realSP);
    end = clock();

    for(vector<Reg>::iterator iter=r1m.begin(); iter!=r1m.end(); iter++)
    {
        cout << "br: " << iter->bitRate << "  sp: " << iter->samplePos << endl;
    }
}*/
