#include "libImpossibleAtlas.hpp"

//Loads a binary into a vector
static std::vector<unsigned char> ReadAllBytes(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        return std::vector<unsigned char>{};
    }

    return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

//Java handles evereything in big-Endian
//Since TIG's level editor is written in java, ints and shorts are written as big-Endian
//They must be converted to little-Endian after being read to be useable here
//This function takes an array of chars and a byte to start from. 
//It bit-shifts the starting byte and the next three bytes, then joins them together into a single int
//file = loaded file as a array of chars, startingOffset = the byte to start processing from
int readIntFromJava(std::vector<unsigned char> &file, int startingOffset)
{
    unsigned int bit1, bit2, bit3, bit4;
    bit1 = static_cast<unsigned int>(file[startingOffset]);
    bit2 = static_cast<unsigned int>(file[startingOffset + 1]);
    bit3 = static_cast<unsigned int>(file[startingOffset + 2]);
    bit4 = static_cast<unsigned int>(file[startingOffset + 3]);

    bit1 = bit1 << 24;
    bit2 = bit2 << 16;
    bit3 = bit3 << 8;
    //bit 4 doesn't get shifted

    unsigned int resultU = bit1 | bit2 | bit3 | bit4;
    int result = static_cast<int>(resultU);
    return result;
}

float readFloatFromJava(std::vector<unsigned char> &file, int startingOffset)
{
    unsigned int bit1, bit2, bit3, bit4;
    bit1 = static_cast<unsigned int>(file[startingOffset]);
    bit2 = static_cast<unsigned int>(file[startingOffset + 1]);
    bit3 = static_cast<unsigned int>(file[startingOffset + 2]);
    bit4 = static_cast<unsigned int>(file[startingOffset + 3]);

    bit1 = bit1 << 24;
    bit2 = bit2 << 16;
    bit3 = bit3 << 8;


    unsigned int resultU = bit1 | bit2 | bit3 | bit4;
    float result = std::bit_cast<float>(resultU);
    return result;
}

//This function takes an array of chars and a byte to start from. 
//It bit-shifts the starting byte and the next byte, then joins them together into a single short
//file = loaded file as a array of chars, startingOffset = the byte to start processing from
short readShortFromJava(std::vector<unsigned char> &file, int startingOffset)
{
    unsigned short bit1, bit2;
    bit1 = static_cast<unsigned int>(file[startingOffset]);
    bit2 = static_cast<unsigned int>(file[startingOffset + 1]);

    bit1 = bit1 << 8;
    //bit 2 doesn't get shifted

    unsigned short resultU = bit1 | bit2;
    short result = static_cast<short>(resultU);
    return result;
}

std::string readUTF8FromJava(std::vector<unsigned char> &file, int startingOffset)
{
    int currentOffset = startingOffset;
    int strLen = readShortFromJava(file, currentOffset);
    std::string result;

    currentOffset += 2;

    for(int i = currentOffset; i < (currentOffset + strLen); i++)
    {
        result += file[i];
    }

    return result;
}

//Java handles evereything in big-Endian
//Since TIG's level editor is written in java, ints and shorts are read as big-Endian
//They must be converted to little-Endian after being written to be accepted by the game
void writeJavaInt(std::ofstream& datafile, int sourceInt)
{
    unsigned int swapSource = __builtin_bswap32(static_cast<unsigned int>(sourceInt));
    datafile.write(reinterpret_cast<const char*>(&swapSource), sizeof(swapSource));
}

void writeJavaFloat(std::ofstream& datafile, float sourceFloat)
{
    unsigned int swapSource = __builtin_bswap32(std::bit_cast<unsigned int>(sourceFloat));
    datafile.write(reinterpret_cast<const char*>(&swapSource), sizeof(swapSource));
}

//Java handles evereything in big-Endian
//Since TIG's level editor is written in java, ints and shorts are read as big-Endian
//They must be converted to little-Endian after being written to be accepted by the game
void writeJavaShort(std::ofstream& datafile, short sourceShort)
{
    unsigned short swapSource = static_cast<unsigned short>((sourceShort >> 8) | (sourceShort << 8));
    datafile.write(reinterpret_cast<const char*>(&swapSource), sizeof(swapSource));
}

void writeJavaUTF8(std::ofstream& datafile, std::string_view sourceStr)
{
    writeJavaShort(datafile, sourceStr.size());
    datafile.write(sourceStr.data(), sourceStr.size());
}

//Endianess doesn't matter for bools or char arrays, this function handles exporting those
void writeOtherData(std::ofstream& datafile, unsigned char data)
{
    datafile.write(reinterpret_cast<const char*>(&data), sizeof(data));
}

//Constructor that generates a blank level if no filepath is given
ImageAtlas::ImageAtlas(bool debugMode)
{
    if(debugMode){std::cout << "No filepath was given, a blank atlas will be generated..." << std::endl;}

    imagesArrLen = 0;
    numFragments = 0;

    if(debugMode){std::cout << "Blank atlas generated!" << std::endl;}
}

//Constructor that calls loadXML or loadBin depending on the detected file format
ImageAtlas::ImageAtlas(const std::string& filename, bool debugMode)
{
    if(debugMode){std::cout << "Attempting to read file " << filename << std::endl;}
    std::vector<unsigned char> atlasChars = ReadAllBytes(filename); //load file from path
    std::string beginning;
    for(int i = 0; i < 4; i++)
    {
        beginning += atlasChars[i];
    }

    if(beginning == "<?xm")
    {
        atlasChars.clear();
        this->loadXML(filename, true);
    }
    else
    {
        this->loadBin(atlasChars, true);
    }
}

//Destructor that serves no purpose right now
ImageAtlas::~ImageAtlas()
{

}

//Parse level data from the given filepath, called by constructor
void ImageAtlas::loadBin(std::vector<unsigned char> atlasChars, bool debugMode)
{
    int currentByte = 0; //tracks the current byte in the file

    //make sure we actually loaded data
    if (atlasChars.size() == 0)
    {
        if(debugMode){std::cout << "Loaded empty file, data will not be processed!" << std::endl;}
        ImageAtlas(false); //call blank constructor if empty file is loaded
    }
    else
    {
        int tempInt;
        short tempShort;
        this->imagesArrLen = readShortFromJava(atlasChars, 0);
        if(debugMode){std::cout << "There are " << this->imagesArrLen << " images in the file!"<< std::endl;}
        currentByte += 2;
        Image *tempImg = new Image;
        Fragment *tempFrag = new Fragment;
        for(int i = 0; i < this->imagesArrLen; i++)
        {
            if(debugMode){std::cout << "Getting length of image name..." << std::endl;}
            tempShort = readShortFromJava(atlasChars, currentByte);
    
            if(debugMode){std::cout << "Name is " << tempShort << " characters long" << std::endl;}

            tempImg->name_imageType_0 = readUTF8FromJava(atlasChars, currentByte);

            currentByte += tempShort + 2;

            if(debugMode){std::cout << "Begin load image " << tempImg->name_imageType_0 << std::endl;}

            tempImg->fragmentArrLen = readShortFromJava(atlasChars, currentByte);

            if(debugMode){std::cout << "There are " << tempImg->fragmentArrLen << " fragments in the image!"<< std::endl;}

            currentByte += 2;

            tempShort = readShortFromJava(atlasChars, currentByte);
    
            if(debugMode){std::cout << "Alpha is " << tempShort << " characters long" << std::endl;}

            tempImg->alpha = readUTF8FromJava(atlasChars, currentByte);

            tempShort = readShortFromJava(atlasChars, currentByte);
    
            if(debugMode){std::cout << "Alpha is " << tempImg->alpha << std::endl;}

            currentByte += tempShort + 2;
            
            
            for(int j = 0; j < tempImg->fragmentArrLen; j++)
            {
                tempShort = readShortFromJava(atlasChars, currentByte);
                if(debugMode){std::cout << "Name of fragment " << j << " is " << tempShort << " characters long" << std::endl;}
                
                tempFrag->name_utf_0 = readUTF8FromJava(atlasChars, currentByte);
                if(debugMode){std::cout << "Name of fragment " << j << " is " << tempFrag->name_utf_0 << std::endl;}

                currentByte += tempShort + 2;

                tempFrag->x_short_1 = readFloatFromJava(atlasChars, currentByte);

                if(debugMode){std::cout << "xpos of fragment " << j << " is " << tempFrag->x_short_1 << std::endl;}

                currentByte += 4;

                tempFrag->y_short_2 = readFloatFromJava(atlasChars, currentByte);

                if(debugMode){std::cout << "ypos of fragment " << j << " is " << tempFrag->y_short_2 << std::endl;}

                currentByte += 4;

                tempFrag->w_short_3 = readFloatFromJava(atlasChars, currentByte);

                if(debugMode){std::cout << "width of fragment " << j << " is " << tempFrag->w_short_3 << std::endl;}

                currentByte += 4;

                tempFrag->h_short_4 = readFloatFromJava(atlasChars, currentByte);

                if(debugMode){std::cout << "height of fragment " << j << " is " << tempFrag->h_short_4 << std::endl;}

                currentByte += 4;

                tempFrag->indexInVec = j;

                tempImg->FragmentArr.push_back(*tempFrag);
                this->numFragments++;
            }

            std::cout << "Loaded " << tempImg->FragmentArr.size() << " fragment(s)!" << std::endl;
            
            this->ImagesArr.push_back(*tempImg);           
        }

        std::cout << "Loaded " << this->ImagesArr.size() << " images!" << std::endl;
        this->imagesArrLen = ImagesArr.size();

        delete tempFrag;
        delete tempImg;
    }

    if(debugMode){std::cout << "Loaded entire atlas!" << std::endl;}
}

void ImageAtlas::loadXML(const std::string& fillepath, bool debugMode)
{
    std::cout << "Reading XML files is currently unsupported, sorry :(" << std::endl;
}

void ImageAtlas::saveToBin(const std::string& filepath)
{
    std::ofstream dataOut;
    dataOut.open(filepath.c_str(), std::ios_base::binary | std::ios_base::out);

    writeJavaShort(dataOut, imagesArrLen);

    for(int i = 0; i < this->imagesArrLen; i++)
    {
        writeJavaUTF8(dataOut, this->ImagesArr[i].name_imageType_0);
        writeJavaShort(dataOut,this->ImagesArr[i].fragmentArrLen);
        writeJavaUTF8(dataOut, this->ImagesArr[i].alpha);

        for(int j = 0; j < this->ImagesArr[i].fragmentArrLen; j++)
        {
            writeJavaUTF8(dataOut, this->ImagesArr[i].FragmentArr[j].name_utf_0);
            writeJavaFloat(dataOut, this->ImagesArr[i].FragmentArr[j].x_short_1);
            writeJavaFloat(dataOut, this->ImagesArr[i].FragmentArr[j].y_short_2);
            writeJavaFloat(dataOut, this->ImagesArr[i].FragmentArr[j].w_short_3);
            writeJavaFloat(dataOut, this->ImagesArr[i].FragmentArr[j].h_short_4);
        }
    }
}

void ImageAtlas::saveToXml(const std::string& filepath)
{
    std::ofstream outXML(filepath);

    outXML << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl << "<ImageAtlas>" << std::endl;
    outXML << "    <ImagesArr>" << std::endl;

    for(int i = 0; i < this->imagesArrLen; i++)
    {
        outXML << "        <Image name_imageType_0=\"" << this->ImagesArr[i].name_imageType_0 << "\" alpha=\"" << this->ImagesArr[i].alpha << "\">" << std::endl;
        outXML << "            <FragmentArr>" << std::endl;

        for(int j = 0; j < this->ImagesArr[i].fragmentArrLen; j++)
        {
            outXML << "                <Fragment name_utf_0=\"" << this->ImagesArr[i].FragmentArr[j].name_utf_0 << "\" ";
            outXML << "x_short_1=\"" << this->ImagesArr[i].FragmentArr[j].x_short_1 << "\" ";
            outXML << "y_short_2=\"" << this->ImagesArr[i].FragmentArr[j].y_short_2 << "\" ";
            outXML << "w_short_3=\"" << this->ImagesArr[i].FragmentArr[j].w_short_3 << "\" ";
            outXML << "h_short_4=\"" << this->ImagesArr[i].FragmentArr[j].h_short_4 << "\" />" << std::endl;
        }

        outXML << "            </FragmentArr>" << std::endl;
        outXML << "        </Image>" << std::endl;
    }

    outXML << "    </ImagesArr>" << std::endl;
    outXML << "</ImageAtlas>" << std::endl;


}

Image* ImageAtlas::getImageByIndex(int index)
{
    if(index >= this->imagesArrLen)
    {
        return &this->ImagesArr[index];
    }
    
    return nullptr;
}

Image* ImageAtlas::getImageByName(std::string_view name)
{
    for(int i = 0; i < this->imagesArrLen; i++)
    {
        if(this->ImagesArr[i].name_imageType_0 == name)
        {
            return &this->ImagesArr[i];
        }
    }

    return nullptr;
}

Fragment* ImageAtlas::getFragmentBy2DIndex(int indexX, int indexY)
{
    if(indexX >= imagesArrLen)
    {
        if(indexY >= this->ImagesArr[indexX].fragmentArrLen)
        {
            return &this->ImagesArr[indexX].FragmentArr[indexY];
        }
    }

    return nullptr;
}

Fragment* ImageAtlas::getFragmentByName(std::string_view name)
{
    for(int i = 0; i < this->imagesArrLen; i++)
    {
        for(int j = 0; j < ImagesArr[i].fragmentArrLen; j++)
        {
            if(this->ImagesArr[i].FragmentArr[j].name_utf_0 == name)
            {
                return &this->ImagesArr[i].FragmentArr[j];
            }
        }
    }

    return nullptr;
}

void ImageAtlas::addImage(Image *input)
{
    input->indexInVec = this->imagesArrLen;
    this->numFragments += input->fragmentArrLen;
    imagesArrLen++;
    this->ImagesArr.push_back(*input);
}

void ImageAtlas::removeImageByName(std::string_view name)
{
    for(int i = 0; i < imagesArrLen; i++)
    {
        if(this->ImagesArr[i].name_imageType_0 == name)
        {
            Image temp = this->ImagesArr[i];
            for(int j = i; j < imagesArrLen - 1; j++)
            {
                this->ImagesArr[j] = this->ImagesArr[j+1];
            }
            this->ImagesArr[imagesArrLen] = temp;
            this->ImagesArr.pop_back();
        }
    }
}
void ImageAtlas::removeFragmentByName(std::string_view name)
{
    
}

void ImageAtlas::removeImageByIndex(int index)
{

}

void ImageAtlas::removeFragmentByIndex(int index)
{

}

void ImageAtlas::printAllFragments()
{

}

void ImageAtlas::printAllImages()
{

}