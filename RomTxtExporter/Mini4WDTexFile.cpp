#include "StdAfx.h"
#include "Mini4WDTexFile.h"
#include <fstream>

#include <Windows.h>
#include <cassert>
#include <vector>

using namespace std;

Mini4WDTexFile::Mini4WDTexFile( const byte* pData, uint32 size )
    : m_pData(pData)
    , m_size(size)
    , m_internalLoad(false)
{
    assert( m_pData && m_size );
    m_fileHeader = (const FileHeader*)m_pData;
    m_colorTable = (const PalleteColor*)(m_pData + 0x20);
    m_pImageData = (const byte*)m_colorTable + m_fileHeader->colorCount * 2 + m_fileHeader->offsetFromPalleteToImageData;
}

Mini4WDTexFile::Mini4WDTexFile( const wchar_t* texFileName )
    : m_pData(NULL)
    , m_size(0)
    , m_internalLoad(true)
{
    ifstream file( texFileName, ios::binary );
    assert( file );

    file.seekg( 0, ios::end );
    m_size = (int)file.tellg();
    file.seekg( 0, ios::beg );

    m_pData = new byte [ m_size ];
    file.read( (char*)m_pData, m_size );

    m_fileHeader = (const FileHeader*)m_pData;
    m_colorTable = (const PalleteColor*)(m_pData + 0x20);
    m_pImageData = (const byte*)m_colorTable + m_fileHeader->colorCount * 2 + m_fileHeader->offsetFromPalleteToImageData;
}

Mini4WDTexFile::~Mini4WDTexFile()
{
    if ( m_internalLoad )
        delete[] m_pData;
}


bool Mini4WDTexFile::SaveAsBmp( const wchar_t* szBmpFilePath )
{
    ofstream file( szBmpFilePath, ios::binary );
    if ( !file )
        return false;

    BITMAPFILEHEADER bmHeader;
    BITMAPINFOHEADER bmInfoHeader;
    RGBQUAD rgbQuad[65536];

    ZeroMemory( &bmHeader, sizeof(bmHeader) );
    ZeroMemory( &bmInfoHeader, sizeof(bmInfoHeader) );

    ZeroMemory( rgbQuad, sizeof(rgbQuad) );

    bmHeader.bfType = MAKEWORD( 'B', 'M' );    
    bmHeader.bfOffBits = sizeof(bmHeader) + sizeof(bmInfoHeader) + sizeof(RGBQUAD) * m_fileHeader->colorCount;

    int colorCount = m_fileHeader->colorCount;
    while ( colorCount != 1 )
    {
        colorCount = colorCount >> 1;
        bmInfoHeader.biBitCount++;
    }

    bmHeader.bfSize = bmHeader.bfOffBits + m_fileHeader->width * m_fileHeader->height * bmInfoHeader.biBitCount / 8;
            
    bmInfoHeader.biClrUsed = m_fileHeader->colorCount;
    bmInfoHeader.biSize = sizeof(bmInfoHeader);
    bmInfoHeader.biCompression = BI_RGB;
    bmInfoHeader.biHeight = -(LONG)m_fileHeader->height;
    bmInfoHeader.biWidth = m_fileHeader->width;
    bmInfoHeader.biPlanes = 1;
    
    for ( uint32 i = 0; i < m_fileHeader->colorCount; i++ )
    {
        PalleteColor colorInTex = m_colorTable[i];
        rgbQuad[i].rgbBlue = ( (colorInTex.color & PalleteColor::BlueMask) >> 10 ) * 8;
        rgbQuad[i].rgbGreen = ( (colorInTex.color & PalleteColor::GreenMask) >> 5 ) * 8;
        rgbQuad[i].rgbRed = (colorInTex.color & PalleteColor::RedMask ) * 8;        
    }

    file.write( (const char*)&bmHeader, sizeof(bmHeader) );
    file.write( (const char*)&bmInfoHeader, sizeof(bmInfoHeader) );
    file.write( (const char*)rgbQuad, sizeof(RGBQUAD) * m_fileHeader->colorCount );
   
    #define pixelPerByte (8 / bmInfoHeader.biBitCount) // 必须用宏，如果我不想用浮点
    int imagePixelSize = m_fileHeader->width * m_fileHeader->height;
    int imageByteSize = imagePixelSize / pixelPerByte;    

    static std::vector<byte> buffer;
    buffer.resize( imageByteSize );

    if ( m_fileHeader->drawMode == DrawModeTile )
    {                
        memcpy( &buffer[0], m_pImageData, imageByteSize );
        RotateByteByBPP( &buffer[0], imageByteSize, bmInfoHeader.biBitCount );
    }
    else
    {
        // 8x8的tile  

        int tileCount = imagePixelSize / TilePixelCount;
        int tileDimenInByte = TileDimension / pixelPerByte;
        int tileCountInRow = m_fileHeader->width / TileDimension;
        int tileCountInCol = m_fileHeader->height / TileDimension;

        for ( int tileIndex = 0; tileIndex < tileCount; tileIndex++ )
        {
            int tileIndexY = tileIndex / tileCountInRow;
            int tileIndexX = tileIndex % tileCountInRow;
            int destTileStartBytePos = tileIndexY * TileDimension * m_fileHeader->width / pixelPerByte + tileIndexX *tileDimenInByte;
            int srcTileStartBytePos = tileIndex * TilePixelCount / pixelPerByte;

            byte* pDestRect = &buffer[destTileStartBytePos];

            if ( m_fileHeader->drawMode == DrawModeObjH )
            {
                for ( int rowInTile = 0; rowInTile < TileDimension; rowInTile++ )
                {
                    memcpy( pDestRect, &m_pImageData[srcTileStartBytePos + rowInTile * tileDimenInByte], tileDimenInByte );
                    RotateByteByBPP( pDestRect, tileDimenInByte, bmInfoHeader.biBitCount );
                    pDestRect += m_fileHeader->width / pixelPerByte; // next row
                }
            }
            else
            {
                // 行列均按字节单位
                for ( int colInTile = 0; colInTile < tileDimenInByte; colInTile++ )
                {
                    for ( int rowInTile = 0; rowInTile < TileDimension; rowInTile++ )
                    {
                        *pDestRect = m_pImageData[srcTileStartBytePos + colInTile * TileDimension + rowInTile ];
                        RotateByteByBPP( pDestRect, 1, bmInfoHeader.biBitCount );
                        pDestRect += m_fileHeader->width / pixelPerByte;
                    }

                    pDestRect = &buffer[destTileStartBytePos] + colInTile * TileDimension; // nex col pos
                }
            }                      
        }
    }

    file.write( (const char*)&buffer[0], imageByteSize );

    return true;
}

// 像素小于8位的，由于原始按BigEndian方式存的相邻像素，靠前像素存在高位，所以要反转一下
void Mini4WDTexFile::RotateByteByBPP( byte* pStart, int size, uint16 bpp )
{
    if ( bpp >= 8 )
        return;

    for ( int i = 0; i < size; i++ )
    {
        byte orgByte = *(pStart + i);
        if ( bpp == 4 )
            orgByte = (orgByte >> 4) | (orgByte << 4);
        else if ( bpp == 2 )
            orgByte = (orgByte >> 6) | ((orgByte & 0x30) >> 2) | ((orgByte & 0x0c) << 2) | (orgByte << 6);
        else if ( bpp == 1 )
        {
            byte c = 0x80;
            byte b = 0x01;
            byte result = 0;
            for ( int j = 0; j < 8; j++ )
            {
                bool bitSet = (orgByte & ( b << j )) > 0;
                if ( bitSet )
                    result |= (c >> j);
                else
                    result &= ~( c >> j);                                        
            }
            orgByte = result;
        }

        *(pStart+i) = orgByte;
    }
}

bool Mini4WDTexFile::LoadFromBmp( const wchar_t* szBmpFilePath, bool dependantTex )
{
    ifstream bmpFile( szBmpFilePath, ios::binary );
    if ( !bmpFile )
        return false;

    bmpFile.seekg( 0, ios::end );
    int length = (int)bmpFile.tellg();
    bmpFile.seekg( 0, ios::beg );

    byte* pBmpData = new byte [ length ];
    bmpFile.read( (char*)pBmpData, length );
    bmpFile.close();

    const BITMAPFILEHEADER* pBmpFileHeader = (const BITMAPFILEHEADER*)pBmpData;
    const BITMAPINFOHEADER* pBmpInfoHeader = (const BITMAPINFOHEADER*)( pBmpFileHeader + 1 );
    const RGBQUAD* colorTableBMP = (const RGBQUAD*)(pBmpInfoHeader + 1);
    const byte* pSrcImageData = (const byte*)(colorTableBMP + pBmpInfoHeader->biClrUsed);

    if ( dependantTex )
    {
                
    }

    for ( uint32 i = 0; i < m_fileHeader->colorCount; i++ )
    {
        PalleteColor& colorInTex = (PalleteColor&)(m_colorTable[i]);

        RGBQUAD colorInBMP = colorTableBMP[i];
        colorInTex.color = (( (colorInBMP.rgbBlue / 8 ) & 0x3f ) << 10) | (( (colorInBMP.rgbGreen / 8 ) & 0x1f ) << 5) | (( (colorInBMP.rgbRed / 8 ) & 0x3f ) );
    }

    #define pixelPerByteAgain (8 / pBmpInfoHeader->biBitCount) // 必须用宏，如果我不想用浮点
    int imagePixelSize = m_fileHeader->width * m_fileHeader->height;
    int imageByteSize = imagePixelSize / pixelPerByteAgain;    

    if ( m_fileHeader->drawMode == DrawModeTile )
    {                
        memcpy( (byte*)m_pImageData, pSrcImageData, imageByteSize );
        RotateByteByBPP( (byte*)m_pImageData, imageByteSize, pBmpInfoHeader->biBitCount );
    }
    else
    {
        // 8x8的tile  

        int tileCount = imagePixelSize / TilePixelCount;
        int tileDimenInByte = TileDimension / pixelPerByteAgain;
        int tileCountInRow = m_fileHeader->width / TileDimension;
        int tileCountInCol = m_fileHeader->height / TileDimension;

        bool isLeftBottomOrg = pBmpInfoHeader->biHeight > 0;
        for ( int tileIndex = 0; tileIndex < tileCount; tileIndex++ )
        {
            int tileIndexY = tileIndex / tileCountInRow;
            int tileIndexX = tileIndex % tileCountInRow;
            int srcTileStartBytePos = 0;
            if ( !isLeftBottomOrg )
                srcTileStartBytePos = tileIndexY * TileDimension * m_fileHeader->width / pixelPerByteAgain + tileIndexX *tileDimenInByte;
            else
                srcTileStartBytePos =  (tileCountInCol - tileIndexY - 1) * TileDimension * m_fileHeader->width / pixelPerByteAgain + tileIndexX *tileDimenInByte;

            int destTileStartBytePos = tileIndex * TilePixelCount / pixelPerByteAgain;

            byte* pDestRect = (byte*)m_pImageData + destTileStartBytePos;

            if ( m_fileHeader->drawMode == DrawModeObjH )
            {
                for ( int rowInTile = 0; rowInTile < TileDimension; rowInTile++ )
                {
                    memcpy( pDestRect, &pSrcImageData[srcTileStartBytePos + rowInTile * m_fileHeader->width / pixelPerByteAgain], tileDimenInByte );                    
                    pDestRect += tileDimenInByte; // next row
                }

                RotateByteByBPP( (byte*)m_pImageData + destTileStartBytePos, TileDimension * tileDimenInByte, pBmpInfoHeader->biBitCount );
            }
            else
            {                
                for ( int colInTile = 0; colInTile < tileDimenInByte; colInTile++ )
                {
                    for ( int rowInTile = 0; rowInTile < TileDimension; rowInTile++ )
                    {
                        *pDestRect = pSrcImageData[srcTileStartBytePos + colInTile + rowInTile * m_fileHeader->width / pixelPerByteAgain ];
                        pDestRect++;
                    }
                }

                RotateByteByBPP( (byte*)m_pImageData + destTileStartBytePos, TileDimension * tileDimenInByte, pBmpInfoHeader->biBitCount );
            }                      
        }
    }

    return true;
}