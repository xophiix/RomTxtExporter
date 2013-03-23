#pragma once

#include "Common.h"

class NDSFileParser;

class RomImageExporterImporter
{
public:
    RomImageExporterImporter();
    ~RomImageExporterImporter();

    bool ExportToSingleBMP( const NDSFileParser& romFile, const wchar_t* szBmpFilePath );

    bool ImportFromBmpBatch( NDSFileParser& romFile, const wchar_t* bmpRootPath );
    bool ImportFromBmp( NDSFileParser& romFile, const wchar_t* srcTexFile, const wchar_t* bmpRoot, const wchar_t* bmpFile );

private:
};