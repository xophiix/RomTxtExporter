#include "stdafx.h"
#include "RomImageExportImporter.h"
#include "NDSFileParser.h"
#include "Mini4WDTexFile.h"

#include <Windows.h>

RomImageExporterImporter::RomImageExporterImporter()
{

}

RomImageExporterImporter::~RomImageExporterImporter()
{

}

bool RomImageExporterImporter::ExportToSingleBMP( const NDSFileParser& romFile, const wchar_t* szBmpFilePath )
{
    const NDSFileParser::FileInfoList* texFiles = romFile.GetFileListByType("tex");
    if ( !texFiles )
        return false;

    CreateDirectory( L"Tex", 0 );

    byte* pRomData = romFile.GetDatePtr();
    for ( size_t i = 0; i < texFiles->size(); i++ )
    {
        const NDSFileParser::FileItem* fileItem = texFiles->at(i);
        
        Mini4WDTexFile texFile( pRomData + fileItem->offset, fileItem->size );

        wchar_t szFileNameW[256] = L"Tex/";
        std::string fullName = fileItem->pathName + '/' + fileItem->fileName;
        int length = fullName.length();
        MultiByteToWideChar( CP_ACP, 0, fullName.c_str(), length, szFileNameW + 4, 252 );

        szFileNameW[ length ] = L'.';
        szFileNameW[ length + 1 ] = L'b';
        szFileNameW[ length + 2 ] = L'm';
        szFileNameW[ length + 3 ] = L'p';

        std::string pathName = "Tex/" + fileItem->pathName;
        CreateDirectoryA( pathName.c_str(), 0 );

        texFile.SaveAsBmp( szFileNameW );
    }

    return true;
}

bool RomImageExporterImporter::ImportFromBmpBatch( NDSFileParser& romFile, const wchar_t* texRootPath )
{
    wchar_t szPrePath[MAX_PATH];
    GetCurrentDirectory( MAX_PATH, szPrePath );
    if ( !SetCurrentDirectory( texRootPath ) )
    {
        SetCurrentDirectory( szPrePath );
        return false;
    }

    const NDSFileParser::FileInfoList* texFiles = romFile.GetFileListByType("tex");
    if ( !texFiles )
        return false;

    byte* pRomData = romFile.GetDatePtr();
    for ( size_t i = 0; i < texFiles->size(); i++ )
    {
        const NDSFileParser::FileItem* fileItem = texFiles->at(i);

        Mini4WDTexFile texFile( pRomData + fileItem->offset, fileItem->size );

        wchar_t szFileNameW[256] = L"";
        std::string fullName = fileItem->pathName + '/' + fileItem->fileName;
        fullName.erase(0,1);
        int length = fullName.length();
        MultiByteToWideChar( CP_ACP, 0, fullName.c_str(), length, szFileNameW, 256 );

        szFileNameW[ length - 1 ] = L'p';
        szFileNameW[ length - 2 ] = L'm';
        szFileNameW[ length - 3 ] = L'b';        

        texFile.LoadFromBmp( szFileNameW, false );
    }
    
    SetCurrentDirectory( szPrePath );
    return true;
}

bool RomImageExporterImporter::ImportFromBmp( NDSFileParser& romFile, const wchar_t* srcTexFile, const wchar_t* bmpRoot, const wchar_t* bmpFile )
{
    wchar_t szPrePath[MAX_PATH];
    GetCurrentDirectory( MAX_PATH, szPrePath );
    if ( !SetCurrentDirectory( bmpRoot ) )
    {
        SetCurrentDirectory( szPrePath );
        return false;
    }

    const NDSFileParser::FileInfoList* texFiles = romFile.GetFileListByType("tex");
    if ( !texFiles )
        return false;

    byte* pRomData = romFile.GetDatePtr();
    for ( size_t i = 0; i < texFiles->size(); i++ )
    {
        const NDSFileParser::FileItem* fileItem = texFiles->at(i);
        
        wchar_t szFileNameW[256] = L"";
        std::string fullName = fileItem->pathName + '/' + fileItem->fileName;
        fullName.erase(0,1);
        int length = fullName.length();
        MultiByteToWideChar( CP_ACP, 0, fullName.c_str(), length, szFileNameW, 256 );

        if ( wcscmp( szFileNameW, srcTexFile ) == 0 )
        {
            Mini4WDTexFile texFile( pRomData + fileItem->offset, fileItem->size );
            texFile.LoadFromBmp( bmpFile, false );
            break;
        }
    }

    SetCurrentDirectory( szPrePath );
    return true;
}