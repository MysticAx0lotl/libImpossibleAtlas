#ifndef IMPOSSIBLE_ATLAS_MANAGER
#define IMPOSSIBLE_ATLAS_MANAGER

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include <bit>
#include <expected>

struct Fragment
{
    std::string name_utf_0;
    float x_short_1;
    float y_short_2;
    float w_short_3;
    float h_short_4;
    int indexInVec;
};

struct Image
{
    std::string name_imageType_0;
    std::string alpha;
    std::vector<Fragment> FragmentArr;
    short fragmentArrLen;
    int indexInVec;
};

class ImageAtlas
{
    public:
        ImageAtlas(bool);
        ImageAtlas(std::string const&, bool);
        ~ImageAtlas();

        void loadBin(std::vector<unsigned char>, bool);
        void loadXML(std::string const&, bool);

        Image* getImageByIndex(int);
        Image* getImageByName(std::string_view);
        Fragment* getFragmentBy2DIndex(int, int);
        Fragment* getFragmentByName(std::string_view);
        int getFragmentCount();
        int getImagesCount();
        
        void addImage(Image*);

        void removeImageByName(std::string_view);
        void removeFragmentByName(std::string_view);
        void removeImageByIndex(int);
        void removeFragmentBy2DIndex(int, int);

        void printAllFragments();
        void printAllImages();
        
        void saveToBin(std::string const&);
        void saveToXml(std::string const&);

    private:
        std::vector<Image> ImagesArr;
        int imagesArrLen;
        int numFragments;
        int trueHeight;
        int trueWidth;
        
};

#endif
