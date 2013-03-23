#pragma once

#include <string>
#include <iosfwd>
#include <vector>
#include <map>

#include "Common.h"

class NDSFileParser
{
public:

    struct NDSFileHeader
    {
        enum { size = 0x4000 };

        char romName[20];
        int unknown[3];

        struct _ArmFileInfo
        {
            int offset;
            int unknown[2];
            int size;
        };

        _ArmFileInfo arm9InsFile;
        _ArmFileInfo arm7InsFile;

        struct _FileBasicInfo
        {
            int offset;
            int size;
        };

        _FileBasicInfo fntInfo; /// 文件目录表
        _FileBasicInfo fatInfo; /// 文件分配表
        _FileBasicInfo arm9OverlayInfo; 
        _FileBasicInfo arm7OverlayInfo; 

        // 暂时不用关心的
        char padding[ size-sizeof(_ArmFileInfo)*2-sizeof(_FileBasicInfo)*4 - 20 - 12 ];
    };

    struct FileItem
    {
        int index;
        int offset;
        int size;
        std::string fileName;
        std::string typeName;
        std::string pathName;
    };

    struct DirectoryInfo
    {
        std::string name;
        std::string fullPathName;
        std::vector<FileItem> files;
        std::vector<DirectoryInfo> subDirs;
    };

    NDSFileParser(void);
    ~NDSFileParser(void);

    bool Load( std::istream& ifs );
    bool SaveAs( const wchar_t* fileName );

    unsigned char* GetDatePtr() const { return m_pData; }

    const DirectoryInfo& GetRootDir() const { return m_rootDir; }

    typedef std::vector<const FileItem*> FileInfoList;
    const FileInfoList* GetFileListByType( const char* pTypeName ) const;

private:

    bool Parse( unsigned char* pData );

    /**
    @param dir 待分析目录结构
    @param pData 当前数据指针
    @return 该目录读取的字节数
    */
    int ParseDir( DirectoryInfo& dir, unsigned char* pData, int& fileIndex );

    void RetrieveDirInfoRecursive( const DirectoryInfo& dir );

    byte* m_pData;
    int m_fileSize;

    struct FileOffset
    {
        int startOffset;
        int endOffset;
    };

    NDSFileHeader* m_fileHeader;
    DirectoryInfo m_rootDir;

    const FileOffset* m_pFileOffsetTable;

    typedef std::map<std::string, FileInfoList> FileInfoListMap;
    FileInfoListMap m_fileInfoMap;
};

