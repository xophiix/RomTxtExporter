#include "StdAfx.h"
#include "NDSFileParser.h"
#include <iostream>
#include <fstream>

using namespace std;

NDSFileParser::NDSFileParser(void)
    : m_pData(NULL)
    , m_fileHeader(NULL)
    , m_pFileOffsetTable(NULL)
    , m_fileSize(0)
{
}


NDSFileParser::~NDSFileParser(void)
{
    delete[] m_pData;
}

bool NDSFileParser::Load( std::istream& ifs )
{
    if ( !ifs )
        return false;

    ifs.seekg(0, ios::beg);
    ifs.seekg(0, ios::end);
    m_fileSize = (int)ifs.tellg();
    ifs.seekg(0, ios::beg);

    if ( !m_fileSize )
        return false;

    if ( m_pData )
        delete[] m_pData;
    m_pData = new unsigned char [ m_fileSize ];
    
    if ( !ifs.read((char*)m_pData, m_fileSize) )
        return false;

    return Parse( m_pData );
}

bool NDSFileParser::Parse( unsigned char* pData )
{
    if ( !pData && m_pData != pData )
        return false;

    m_fileHeader = (NDSFileHeader*)pData;

    // 解析文件目录
    typedef unsigned char byte;
    m_pFileOffsetTable = (const FileOffset*)(pData + m_fileHeader->fatInfo.offset);
    
    byte* pFNT = pData + m_fileHeader->fntInfo.offset;

    unsigned short overlayFileCount = *(unsigned short*)(pFNT + 4);

    byte* p = pFNT + 15;
    while ( *p == (byte)0xf0 )
    {
        p += 8;
    }

    p -= 7;

    int fileIndex = overlayFileCount;
    ParseDir( m_rootDir, p, fileIndex );

    // 提取同类型文件
    m_fileInfoMap.clear();
    RetrieveDirInfoRecursive( m_rootDir );    
    
    return true;
}

int NDSFileParser::ParseDir( DirectoryInfo& dir, unsigned char* pData, int& fileIndex )
{
    unsigned char* p = pData;

    if ( (*p & 0x80) == 0 ) // 文件
    {
        while( (*p & 0x80) == 0 && (*p != 0) )
        {
            FileItem file;
            int nameLen = (int)(*p & 0x7f);
            file.fileName.assign( (const char*)(p+1), nameLen ); 
            file.pathName = dir.fullPathName;
            file.index = fileIndex;

            file.offset = m_pFileOffsetTable[fileIndex].startOffset;
            file.size = m_pFileOffsetTable[fileIndex].endOffset - file.offset;

            fileIndex++;
            p += nameLen + 1;

            unsigned char* pNameBackward = p - 1;
            while ( *pNameBackward != '.' && nameLen-- )
            {
                file.typeName.insert( file.typeName.begin(), *pNameBackward );
                pNameBackward--;
            }

            dir.files.push_back( file );
        }        
    }
    
    // 跳过0字节
    if ( *p == 0 ) // 本目录结束
    {
        p++;
        return p - pData;
    }

    if ( p - m_pData >= m_fileHeader->fntInfo.offset + m_fileHeader->fntInfo.size )
        return p - pData;

    // 目录，则连续都是子目录名，直到本目录结束
    if ( *p & 0x80 )
    {                       
        while ( *p != 0 )
        {
            DirectoryInfo subDir;
            size_t nameLen = (size_t)(*p & 0x0f);
            subDir.name.assign( (const char*)(p+1), nameLen );
            subDir.fullPathName = dir.fullPathName + '/' + subDir.name;

            dir.subDirs.push_back( subDir );

            p += nameLen + 1;
            p += 2; // 跳过2个不关心字节
        }                           

        p++; // 跳过0字节

        if ( p - m_pData >= m_fileHeader->fntInfo.offset + m_fileHeader->fntInfo.size )
            return p - pData;

        // 此目录结束
        // 遍历子目录
        for ( size_t i = 0; i < dir.subDirs.size(); i++ )
        {
            p += ParseDir( dir.subDirs[i], p, fileIndex );
        }
    }

    return p - pData;
}

const NDSFileParser::FileInfoList* NDSFileParser::GetFileListByType( const char* pTypeName ) const
{
    FileInfoListMap::const_iterator i = m_fileInfoMap.find( pTypeName );
    return i != m_fileInfoMap.end() ? &i->second : NULL;
}

void NDSFileParser::RetrieveDirInfoRecursive( const DirectoryInfo& dir )
{
    for ( size_t i = 0; i < dir.files.size(); i++ )
    {
        const FileItem& file = dir.files[i];
        m_fileInfoMap[ file.typeName ].push_back( &file );
    }

    for ( size_t i = 0; i < dir.subDirs.size(); i++ )
    {
        RetrieveDirInfoRecursive( dir.subDirs[i] );
    }
}

bool NDSFileParser::SaveAs( const wchar_t* fileName )
{
    if ( !m_pData || !m_fileSize )
        return false;

    ofstream f( fileName, ios::binary );
    if ( !f )
        return false;

    f.write( (const char*)m_pData, m_fileSize );

    return true;
}