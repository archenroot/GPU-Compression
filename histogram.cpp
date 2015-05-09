#include <amp.h>
#include <iostream>
using namespace concurrency;

int main()
{
    static const unsigned DATASIZE = 1<<29;
    static const unsigned CHUNK    = 1<<16;
    static const unsigned BIN      = 1<<8;
    static const unsigned ITER     = CHUNK/sizeof(unsigned);
    static const unsigned WARP     = 32;

    // simple vector addition example
    std::vector<unsigned> inbuf(DATASIZE/sizeof(unsigned));
    std::vector<unsigned> outbuf((inbuf.size()/CHUNK)*BIN);

    concurrency::array_view<unsigned, 1> av0( inbuf.size(),  inbuf);
    concurrency::array_view<unsigned, 1> av1(outbuf.size(), outbuf);
    av0.discard_data();
    av1.discard_data();

    concurrency::parallel_for_each (av0.extent, [=](concurrency::index<1> idx) restrict(amp)
    {
        av0[idx] = idx[0]*1234567890;
    });

    concurrency::parallel_for_each ((av0.extent/ITER).tile<WARP>(), [=](concurrency::tiled_index<WARP> idx) restrict(amp)
    {
        tile_static unsigned freq[BIN*WARP];
        for (int k=0; k<BIN; k++)
            freq[k*WARP+idx.local[0]] = 0;
        for (int i=idx.global[0]*ITER,k=0; k<ITER; k++,i++)
        {
            unsigned x = av0[i];
            freq[ x      % BIN]++;
            freq[(x>> 8) % BIN]++;
            freq[(x>>16) % BIN]++;
            freq[(x>>24) % BIN]++;
        }
        for (int i=idx.global[0]*BIN,k=0; k<BIN; k++,i++)
            av1[i] = freq[k*WARP+idx.local[0]];
    });

    av1.synchronize();
//    for (int i=0; i<outbuf.size(); i++)
//      std::cout << (i%256==0? "\n== " : " ") << outbuf[i];

    return 0;
}