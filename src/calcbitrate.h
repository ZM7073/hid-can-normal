#ifndef CALCBITRATE_H
#define CALCBITRATE_H

struct Reg{
public:
    Reg(int brp, int t1, int t2, long br):
            BRP(brp), TSG1(t1), TSG2(t2), bitRate(br)
    {
        // bitRate=fclk_io/(2*(BRP+1)*((TSG1+1)+(TSG2+1)+1);
        samplePos=(TSG1+1)*100/((TSG2+1)+(TSG1+1));
    }
    Reg(){}

    int BRP;
    int TSG1;
    int TSG2;
    long bitRate;
    int samplePos;
};

#define BR5K 500000
#define BR1M 1000000
#define BRBIAS 0.15

#define fclk_io 19.6608E6 // 19.6608MHz
#define fclk_main 50E6 // 50MHz

int addItem();
Reg selectReg(long realBR, int realSP);

#endif // CALCBITRATE_H
