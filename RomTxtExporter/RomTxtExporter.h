#pragma once

#include <map>
#include <iosfwd>

#include "Common.h"

// 码表
struct CodeItem
{
    union
    {
        unsigned short code;
        unsigned char codeBytes[2];        
    };

    union
    {
        unsigned short reverse_code;        /// BigEndian表示
        unsigned char reverse_codeBytes[2];
    };
    
    uint8 byteCount;
    wchar_t character;

    CodeItem()
        : code(0)
        , reverse_code(0)
        , byteCount(0)
        , character(0)
    {}

    static uint16 ReverseCode( uint16 orgCode )
    {
        return (uint16)( (orgCode << 8) | (byte)(orgCode >> 8) );
    }

    CodeItem( uint16 _code, bool reversed, wchar_t c )
    {
        if ( reversed )
        {
            reverse_code = _code;
            code = ReverseCode(_code);
        }
        else
        {
            code = _code;
            reverse_code = ReverseCode(_code);
        }

        byteCount = 1;
        if ( reverse_code & 0xff00 )
            byteCount = 2;
        
        character = c;
    }
};

typedef std::map<uint32, CodeItem> CodeTable;

class NDSFileParser;

class RomTxtExporter
{
public:
    RomTxtExporter(void);
    ~RomTxtExporter(void);

    bool LoadCodeTable( const wchar_t* filename );
    bool SaveCodeTable( const CodeTable& codeTable, const wchar_t* fileName );

    bool Export( const NDSFileParser& ndsFile, const wchar_t* destFilename );

    bool ExtractByOffset( std::ostream &outFile, byte* pSrcData, int offset, int size, int& segmentCount );

    // 导入
    bool Import( const wchar_t* textFile, NDSFileParser& ndsFile, const wchar_t* newCodeTableFile );

private:
    void OutputTextSegment( std::wstring &strTextSegment, int& segmentCount, int startIndex, int endIndex, std::ostream &outFile );
    CodeTable m_codeTable;
    
    bool m_byteFlag[256];   /// 该字节是否在码表
};

