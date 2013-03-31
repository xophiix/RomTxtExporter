#include "StdAfx.h"
#include "RomTxtExporter.h"
#include "NDSFileParser.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

RomTxtExporter::RomTxtExporter(void)
{
    memset( m_byteFlag, 0, sizeof(m_byteFlag) );
}

RomTxtExporter::~RomTxtExporter(void)
{
}

bool RomTxtExporter::LoadCodeTable( const wchar_t* filename )
{
    ifstream f(filename, ios::binary );
    if ( !f.is_open() )
        return false;

    f.seekg(2); // 跳过UNICODE头

    while ( !f.eof() )
    {
        wstring strLine;
        
        wchar_t c = 0;
        while ( !f.eof() )
        {
            f.read( (char*)&c, sizeof(c) );
            if ( c == L'\r' )
                break;

            strLine.push_back(c);
        }

        // skip 0x0a
        f.read( (char*)&c, sizeof(c) );

        uint32 code = 0;
        wchar_t character = 0;        

        swscanf( strLine.c_str(), L"%x=%c", &code, &character );

        if ( code == 0 )
            continue;

        CodeItem codeItem;        
        codeItem.byteCount = ( code & 0x00ff00 ) ? 2 : 1;
        codeItem.code = (unsigned short)code;
        codeItem.character = character;

        codeItem.reverse_code = (unsigned short)code;
        codeItem.code = (( code & 0xff ) << 8) | ((code & 0xff00) >> 8);        

        m_codeTable[ codeItem.code ] = codeItem;

        m_byteFlag[ (byte)code ] = true;

        if ( codeItem.byteCount == 2 )
            m_byteFlag[ (byte)(code >> 8) ] = true;

		m_charToCodeMap[ character ] = code;
    }

    return true;
}

bool RomTxtExporter::Export( const NDSFileParser& ndsFile, const wchar_t* destFilename )
{
    ofstream outFile( destFilename, ios::binary );    
    if ( !outFile )
    {
        return false;
    }

    // 只导出spt和dci后缀的文件中的文本
    const char* targetTypes[] = { "spt", "dic" };

    // unicode BOM
    outFile.put((char)0xff);
    outFile.put((char)0xfe);

    byte* pData = ndsFile.GetDatePtr();    
    int segCount = 0;
    for ( int i = 0; i < sizeof(targetTypes)/sizeof(targetTypes[0]); i++ )
    {
        const NDSFileParser::FileInfoList* pTargetFiles = ndsFile.GetFileListByType(targetTypes[i]);
        if ( !pTargetFiles )
            continue;
       
        for ( size_t j = 0; j < pTargetFiles->size(); j++ )
        {
            const NDSFileParser::FileItem* pFileItem = pTargetFiles->at(j);

            int textOffsetInFile = pFileItem->offset;
            int actualSize = pFileItem->size;
            if ( targetTypes[i] == "dic" )
            {
                // dic的文本在文件中的位置，可跳过前面的非文本数据！
                int relOffset = ( *(int*)( pData + pFileItem->offset ) + 1 ) * 4;
                textOffsetInFile += relOffset;
                actualSize -= relOffset;
            }

            ExtractByOffset( outFile, pData, textOffsetInFile, actualSize, segCount );
        }
    }
    
    return true;
}

void RomTxtExporter::OutputTextSegment( wstring &strTextSegment, int& segmentCount, int startIndex, int endIndex, ostream &outFile )
{
    if ( !strTextSegment.empty() )
    {
        // 只有一个换行啊
        if ( strTextSegment == L"<n>\n" || strTextSegment == L"<r>")
        {
            strTextSegment.clear();
            return;
        }

        wstringstream wss;              
        wss << L"##########[" << ++segmentCount << L"]##########\n";
        wss << hex << startIndex << L"-" << endIndex << dec << L", " << (endIndex - startIndex) << L"\n";
        wss << strTextSegment << L"\n" << endl;

        wstring s = wss.str();

        outFile.write( (const char*)s.c_str(), s.length() * sizeof(wchar_t) );

        strTextSegment.clear();
    }
}

bool RomTxtExporter::ExtractByOffset( std::ostream &outFile, byte* pSrcData, int offset, int size, int& segmentCount )
{
    wstring strTextSegment;

    int startIndex = 0;
    int i = offset;
    int endOffset = offset + size;
    while ( i < endOffset )
    {
        uint32 aByte = (uint32)pSrcData[i];
        // 控制字符检查
        if ( (byte)aByte == '<' && i+1 < endOffset )
        {            
            switch( pSrcData[i+1] )
            {
            case 'c':
            case 's':
                {
                    int j = i+2;
                    if ( pSrcData[j] == ':' )
                    {
                        j++;
                        if ( pSrcData[i+1] == 'c' )
                        {
                            char c = pSrcData[j];
                            char c1 = c | 0x20;
                            int maxCount = 6;
                            while (  j < endOffset && c != '>' && maxCount-- > 0 
                                && ( ( c >= '0' && c <= '9' ) || ( c1 >= 'a' && c1 <= 'f' ) ) // 必须0-F
                                )
                                j++;
                        }
                        else if ( pSrcData[i+1] == 's' )
                        {
                            while ( pSrcData[j] != '>' && j < endOffset && pSrcData[j] >= '0' && pSrcData[j] <= '9' )
                                j++;
                        }                    
                    }

                    if ( pSrcData[j] == '>' )
                    {
                        if ( strTextSegment.empty() )
                            startIndex = i;

                        for ( int idx = i; idx <= j; idx++ )
                        {
                            strTextSegment.push_back( (wchar_t)pSrcData[idx] );
                        }
                    }

                    i = j+1;
                    continue;
                }
                break;                
            }
        }
        else if ( (byte)aByte == (byte)0x0a ) // 换行单独处理
        {
            if ( strTextSegment.empty() )
                startIndex = i;

            strTextSegment.append(L"<n>");
            strTextSegment.push_back(L'\n');
            i++;
            continue;
        }
        else if ( (byte)aByte == (byte)0x0d ) // 回车单独处理
        {
            if ( strTextSegment.empty() )
                startIndex = i;

            strTextSegment.append(L"<r>");
            i++;
            continue;
        }

//         else if ( (byte)aByte == '\0' )
//         {
//             // 角色代号
//             if ( i+2 < endOffset && pSrcData[i+1] == 'S' && pSrcData[i+2] >= '0' && pSrcData[i+2] <= '9' )
//             {
//                 if ( strTextSegment.empty() )
//                     startIndex = i;
// 
//                 for ( int idx = i; idx <= i+2; idx++ )
//                 {
//                     strTextSegment.push_back( (wchar_t)pSrcData[idx] );
//                 }
// 
//                 strTextSegment[0] = L'_';   // 方便辨识
//                 i += 3;
//                 continue;
//             }
//         }

        // 不在码表的字节
        if ( !m_byteFlag[ pSrcData[i] ] )
        {
            // 生成一段描述
            OutputTextSegment(strTextSegment, segmentCount, startIndex, i, outFile);

            i++;
            continue;
        }

        CodeTable::const_iterator iCode = m_codeTable.find( aByte );
        if ( iCode != m_codeTable.end() && iCode->second.byteCount == 1 )
        {
            // 单字节编码
            if ( strTextSegment.empty() )
                startIndex = i;

            strTextSegment.push_back( iCode->second.character );
            i++;
        }
        else
        {
            // 双字节编码
            if ( i+1 < endOffset )
            {
                uint32 bByte = (uint32)pSrcData[i+1];
                bool bValid = false;
                if ( m_byteFlag[ bByte ] )
                {
                    bByte = (uint32)(bByte << 8) | aByte;

                    iCode = m_codeTable.find( bByte );
                    if ( iCode != m_codeTable.end() )
                    {
                        if ( strTextSegment.empty() )
                            startIndex = i;

                        if ( iCode->second.character == L'?' ) // 特殊的无法显示字符
                        {
                            // 直接用<编码>还原
                            strTextSegment.push_back(L'<');
                            wchar_t szDigit[5] = {0};
                            _itow( iCode->second.reverse_code, szDigit, 16 );
                            strTextSegment.append( szDigit );
                            strTextSegment.push_back(L'>');
                        }
                        else
                        {
                            strTextSegment.push_back( iCode->second.character );
                        }
                        
                        bValid = true;
                    }
                }                

                if ( !bValid )
                {
                    OutputTextSegment( strTextSegment, segmentCount, startIndex, i, outFile );
                    i++;
                }
                else
                    i += 2;
            }
        }
    }

    // 生成一段描述
    OutputTextSegment( strTextSegment, segmentCount, startIndex, i, outFile );
    return true;
}

bool RomTxtExporter::Import( const wchar_t* textFileName, NDSFileParser& ndsFile, const wchar_t* newCodeTableFile )
{
    ifstream textFile( textFileName, ios::binary );
    if ( !textFile )
        return false;

    struct TextBlock
    {
        int startAddress;
        int endAddress;
        int byteSize;
        wstring translatedText;         /// 用于查看
        string translatedTextToImport;  /// 用于导入，会转成非unicode
    };

    textFile.seekg(2); // 跳过文件头
    textFile.seekg(0, ios::end);
    size_t fileLen = textFile.tellg();
    textFile.seekg(2);
    
    wchar_t* textBuffer = new wchar_t [ fileLen / 2 ];
    textFile.read( (char*)textBuffer, fileLen );
    textFile.close();

    const uint16 startNewCode = 0x889f;   /// SJIS编码汉字第一个编码，以此产生新的码表，注意需要翻转
    uint16 curNewCode = startNewCode;
    uint16 charToNewCodeMap[65536] = {0}; 
    CodeTable newCodeTable;

    vector<TextBlock> totalTextBlocks;
    // 统计字符
    int i = 0;
    int charCount = fileLen / 2;
    while ( i < charCount )
    {
        // #####行
        while ( i < charCount && textBuffer[i] == L'#' )
            i++;

        wstring strIndex;
        if ( textBuffer[i] == L'[' )
        {
            i++;
            while ( i < charCount && textBuffer[i] != L']' )
            {
                strIndex.push_back( textBuffer[i] );
                i++;
            }

            i++;
        }

        while ( i < charCount && textBuffer[i] == L'#' )
            i++;

        if ( textBuffer[i] == 0x000a )
            i++;

        TextBlock textBlock;
        
        // 开始地址
        wstring strStartAddress;
        while ( i < charCount && textBuffer[i] != L'-' )
        {
            strStartAddress.push_back( textBuffer[i] );
            i++;
        }

        swscanf( strStartAddress.c_str(), L"%x", &textBlock.startAddress );

        // 字节数       
        while ( i < charCount && textBuffer[i] != L' ' )
            i++;
        i++;

        wstring strBytes;
        while ( i < charCount && textBuffer[i] >= L'0' && textBuffer[i] <= L'9' )
        {
            strBytes.push_back( textBuffer[i] );
            i++;
        }

        swscanf( strBytes.c_str(), L"%d", &textBlock.byteSize );
        textBlock.endAddress = textBlock.startAddress + textBlock.byteSize;

        while ( i < charCount && textBuffer[i] == 0x000a )
            i++;

        // 正文
        while ( i < charCount && textBuffer[i] != L'#' )
        {
            if ( textBuffer[i] == L'<' )
            {
                if ( textBuffer[i+1] == L'n' && textBuffer[i+2] == L'>' )
                {
                    textBlock.translatedText.push_back( 0x0a );
                    i += 4;
                }
                else if ( textBuffer[i+1] == L'r' && textBuffer[i+2] == L'>' )
                {
                    textBlock.translatedText.push_back( 0x0d );
                    i += 3;
                }
            }

            // 汉字
            if ( textBuffer[i] > 127 )
            {
                // 获取相应的编码
                if ( !charToNewCodeMap[ textBuffer[i] ] )
                {
                    charToNewCodeMap[ textBuffer[i] ] = curNewCode;
                    
                    CodeItem codeItem( curNewCode, true, textBuffer[i] );                    
                    newCodeTable[ curNewCode ] = codeItem;
                    
                    bool valid = false;
                    while ( !valid )
                    {
                        ++curNewCode;
                        valid = m_codeTable.find( CodeItem::ReverseCode( curNewCode ) ) != m_codeTable.end();
                    }                    
                }

				//uint16 newCode = m_charToCodeMap[ textBuffer[i] ];
                uint16 newCode = charToNewCodeMap[ textBuffer[i] ];

                textBlock.translatedText.push_back( textBuffer[i] );

                // 替换为新的编码
                textBlock.translatedTextToImport.push_back( (char)(newCode >> 8) );
                textBlock.translatedTextToImport.push_back( (char)newCode );
            }
            else if ( textBuffer[i] != 0x000a )
            {
                textBlock.translatedText.push_back( textBuffer[i] );
                textBlock.translatedTextToImport.push_back( (char)textBuffer[i] );
            }

            i++;
        }
        
        if ( i >= charCount )
            break;

        // 补全空格
        int importStrLength = (int)textBlock.translatedTextToImport.length();
        if ( importStrLength < textBlock.byteSize )
        {
            int amendCount = textBlock.byteSize - importStrLength;
            for ( int k = 0; k < amendCount; k++ )
            {
                textBlock.translatedTextToImport.push_back( k % 2 ? (char)0x20 : 0 );
            }            
        }
        else if ( importStrLength > textBlock.byteSize )
        {
            textBlock.translatedTextToImport.erase( textBlock.byteSize,importStrLength - textBlock.byteSize );
            wcout << strIndex << L"文本长度超出原始大小，已做截取！" << endl;
        }

        totalTextBlocks.push_back( textBlock );
    }

    delete [] textBuffer;

    // 新码表建立完毕
    wcout << "总字符种数: " << curNewCode - startNewCode << endl;

    // 保存新码表
    if ( !SaveCodeTable( newCodeTable, newCodeTableFile ) )
    {
        wcout << L"新码表生成失败！" << endl;
        return false;
    }

    // 导入到rom
    byte* pRomData = ndsFile.GetDatePtr();

    for ( size_t i = 0; i < totalTextBlocks.size(); i++ )
    {
        const TextBlock& textBlock = totalTextBlocks[i];
        memcpy( pRomData + textBlock.startAddress, textBlock.translatedTextToImport.c_str(), textBlock.byteSize );
    }

    return true;
}

bool RomTxtExporter::SaveCodeTable( const CodeTable& codeTable, const wchar_t* fileName )
{
    ofstream f( fileName, ios::binary );
    if ( !f )
        return false;

    f.put((char)0xff);
    f.put((char)0xfe);

    for ( CodeTable::const_iterator i = codeTable.begin(); i != codeTable.end(); ++i )
    {
        const CodeItem& codeItem = i->second;

        wchar_t szLine[9] = {0};
        swprintf( szLine, 9, L"%x=%c\r\n", codeItem.reverse_code, codeItem.character );
        
        f.write( (const char*)szLine, 16 );
    }

    return true;
}