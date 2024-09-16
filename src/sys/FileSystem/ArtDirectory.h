//
// Created by artypoole on 15/09/24.
//

#ifndef ARTDIRECTORY_H
#define ARTDIRECTORY_H



#include "Files.h"
#include "LinkedList.h"


class ArtFile;

class ArtDirectory
{
public:
    ArtDirectory(ArtDirectory* parent, const DirectoryData& data);
    ~ArtDirectory();


    int add_file(const FileData& data) ;
    int add_subdir(ArtDirectory* parent, const DirectoryData& data) ;

    //todo: remove dir and remove file

    ArtFile* search(const char* filename);
    ArtFile* search_recurse(char* filename);

private:

    ArtDirectory* parent_directory;
    DirectoryData directory_data;
    LinkedList<ArtDirectory> directories={};
    LinkedList<ArtFile> files = {};
};




#endif //ARTDIRECTORY_H
