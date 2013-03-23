// RomTxtExporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RomTxtExporter.h"
#include "NDSFileParser.h"
#include "RomImageExportImporter.h"
#include "Mini4WDTexFile.h"

#include <iostream>
#include <fstream>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{           
//     if ( argc < 4 )
//     {        
//         wcout << L"usage: RomTxtExporter codeTable romFile exportFile" << endl;
// 
//         return -1;
//     }
//    
//     const wchar_t* codeTableFileName = argv[1];
    const wchar_t* srcFileName = argv[1];
//     const wchar_t* destFileName = argv[3];
//     const wchar_t* targetImageFileName = L"allinone.bmp";
    
    ifstream f( srcFileName, ios::binary );
    if ( !f )
    {
        cout << "没有找到ROM文件" << endl;
        return -1;
    }

    cout << "解析中，请稍等" << endl;
    NDSFileParser ndsFile;
    if ( !ndsFile.Load(f) )
    {
        cout << "只有迷你四驱ROM可以解析！" << endl;
        return -1;
    }

//     RomTxtExporter exporter;
//     if ( !exporter.LoadCodeTable( codeTableFileName ) )
//     {
//         wcout << L"没有提供码表" << endl;
//         return -1;
//     }

//     if ( !exporter.Import( L"测试稿.txt", ndsFile, L"新码表.txt" ) )
//     {
//         wcout << L"文本导入失败！" << endl;
//         return -1;
//     }

//     if ( !exporter.Export( ndsFile, destFileName ) )
//     {
//         wcout << L"文本导出出现错误！" << endl;
//         return -1;
//     }

//     Mini4WDTexFile file( L"b2_mark.tex" );
//     file.SaveAsBmp( L"b2_mark.bmp");

    RomImageExporterImporter imageExporter;
//     if ( !imageExporter.ExportToSingleBMP( ndsFile, NULL ) )
//     {
//         cout << "图片导出出现错误！" << endl;
//         return -1;
//     }
//     if ( !imageExporter.ImportFromBmpBatch(ndsFile, L"Tex") )
//     {
//         cout << "图片导入错误！" << endl;
//         return -1;
//     }

    if ( !imageExporter.ImportFromBmp(ndsFile, L"logo/copy_t.tex", L"Tex", L"logo/copy_t.bmp") )
        return -1;

    ndsFile.SaveAs( L"bmp_imported.nds" );

    cout << "图片导入完成！" << endl;

	return 0;
}

