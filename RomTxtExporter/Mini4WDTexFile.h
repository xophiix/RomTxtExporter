#pragma once

#include "Common.h"

class Mini4WDTexFile
{
public:
    Mini4WDTexFile( const byte* pData, uint32 size );
    Mini4WDTexFile( const wchar_t* texFileName );
    ~Mini4WDTexFile();

    bool SaveAsBmp( const wchar_t* szBmpFilePath );

    /**
    @param dependantTex 创建独立纹理文件，还是直接写回引用的数据
    */
    bool LoadFromBmp( const wchar_t* szBmpFilePath, bool dependantTex );

private:

    bool m_internalLoad;    /// 内部加载
    const byte* m_pData;
    uint32 m_size;

    enum DrawMode
    {        
        DrawModeTile = 0,        
        DrawModeObjH = 1,
        DrawModeObjV = 2,
    };

    struct FileHeader
    {
        enum { MagicID = 0x5845544E };
        uint32 magicID;    // 0
        byte padding0[6];  // 0x4
        uint16 width;       // 0xa
        uint16 height;      // 0xc
        uint16 colorCount;  // 0xe
        uint32 unknown0;
        uint16  drawMode;
        byte padding1[6];                
        uint16 offsetFromPalleteToImageData;
        uint16 unknown1;
    };

    const FileHeader* m_fileHeader;
    
    struct PalleteColor
    {
        enum
        {
            RedMask = 0x1f,
            GreenMask = RedMask << 5,
            BlueMask = GreenMask << 5,
        };

        union
        {
            uint16 color;
            byte colorBytes[2];
        };
    };

    enum
    { 
        TileDimension = 8,
        TilePixelCount = TileDimension * TileDimension
    };

    // 像素小于8位的，由于原始按BigEndian方式存的相邻像素，靠前像素存在高位，所以要反转一下
    static void RotateByteByBPP( byte* pStart, int size, uint16 bpp );

    const PalleteColor* m_colorTable;
    const byte* m_pImageData;
};
