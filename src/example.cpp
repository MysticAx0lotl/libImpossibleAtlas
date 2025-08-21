#include "libImpossibleAtlas.hpp"

//This is an example that generates a blank level
int main(int argc, char *argv[])
{
    ImageAtlas test(argv[1], true);

    /*
    Image *cstbg = new Image;

    Fragment *cust = new Fragment;
    cust->x_short_1 = 0.892578125;
    cust->y_short_2 = 0.45703125;
    cust->w_short_3 = 0.078125;
    cust->h_short_4 = 0.533203125;

    cust->name_utf_0 = "CstBgtest";

    cstbg->FragmentArr.push_back(*cust);
    cstbg->fragmentArrLen += 1;

    test.addImage(cstbg);
    */

    std::string outdir = argv[1];

    std::string outfix = outdir.substr(0, outdir.length()-3);

    test.saveToXml(outfix + "xml");
    
    return 0;
}
