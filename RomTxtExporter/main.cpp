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

void help()
{
	cout << "usage: RomTxtExporter -i romFile translatedRomFile jisCodeTable newCodeTable translatedText translatedImagePath[option]" << endl; 
	cout << "usage: RomTxtExporter -e romFile jisCodeTable exportTextFile exportImagePath[option]" << endl; 
	cout << "工作模式 -e表示导出，-i表示导入\n"
		<< "***当-i模式时，参数如下\n"
		<< "\tromFile	rom文件路径\n"
		<< "\ttranslatedRomFile	导入后的rom文件新路径\n"
		<< "\tjisCodeTable	Shift-JIS码表文件路径\n"
		<< "\tnewCodeTable	翻译的文本生成的新码表文件保存路径\n"
		<< "\ttranslatedText	汉化后的文本文件路径\n"
		<< "\ttranslatedImagePath	汉化后的图片目录路径，可选参数，没有则不导入图片\n"
		<< "***当-e模式时，参数如下\n"
		<< "\tromFile	rom文件路径\n"				
		<< "\tjisCodeTable	Shift-JIS码表文件路径\n"
		<< "\texportTextFile	文本导出文件名\n"	
		<< "\texportImagePath	导出的图片目录路径，可选参数，没有则不导出图片\n"
		<< "\n" << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{           
	if ( argc < 2 )
	{        
		help();
		return -1;
	}

	const wchar_t* mode = argv[1];		
	const wchar_t* srcFileName = argv[2];

	if ( wstring(mode) == L"-i" )
	{
		if ( argc < 6 )
		{
			help();
			return -1;
		}		
	}
	else if ( wstring(mode) == L"-e" )
	{
		if ( argc < 4 )
		{
			help();
			return -1;
		}		
	}
	else
	{
		help();
		return -1;
	}

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

	RomTxtExporter exporter;
	RomImageExporterImporter imageExporter;

	if ( wstring(mode) == L"-e" )
	{
		//cout << "usage: RomTxtExporter -e romFile jisCodeTable exportTextFile exportImagePath[option]" << endl; 
		const wchar_t* destFileName = argv[3];
		const wchar_t* codeTableFileName = argv[4];		 
		const wchar_t* translatedTextFile = argv[5];
		const wchar_t* translatedImagePath = argv[6];

		cout << "开始导出文本到" << destFileName << endl;
		if ( !exporter.Export( ndsFile, destFileName ) )
		{
			cout << "文本导出出现错误！" << endl;
			return -1;
		}
		cout << "文本导出完毕！" << endl;

		if ( translatedImagePath && translatedImagePath[0] )
		{
			cout << "开始导出图片到" << destFileName << endl;
			if ( !imageExporter.ExportToSingleBMP( ndsFile, translatedImagePath ) )
			{
				cout << "导出图片错误" << endl;
				return -1;
			}
			cout << "导出图片完毕!" << destFileName << endl;
		}

		cout << "导出完毕" << endl;
		return 0;
	}
	else if ( wstring(mode) == L"-i" )
	{
		//"usage: RomTxtExporter -i romFile translatedRomFile jisCodeTable newCodeTable translatedText translatedImagePath[option]
		const wchar_t* destFileName = argv[3];
		const wchar_t* codeTableFileName = argv[4];
		const wchar_t* newCodeTableFileName = argv[5];
		const wchar_t* translatedTextFile = argv[6];
		const wchar_t* translatedImagePath = argv[7];

		cout << "读取原始码表" << endl;
		if ( !exporter.LoadCodeTable( codeTableFileName ) )
		{
			cout << "没有提供码表" << endl;
			return -1;
		}

		if ( translatedTextFile && translatedTextFile[0] )
		{
			if ( !exporter.Import( translatedTextFile, ndsFile, newCodeTableFileName ) )
			{
				cout << "文本导入失败！" << endl;
				return -1;
			}
			else
				cout << "汉化文本导入成功!" << endl;
		}

		if ( translatedImagePath && translatedImagePath[0] )
		{
			if ( !imageExporter.ImportFromBmpBatch(ndsFile, translatedImagePath ) )
			{
				cout << "图片导入错误！" << endl;
				return -1;
			}
			else
				cout << "图片导入成功" << endl;
		}		 

		cout << "更新rom文件" << endl;
		if ( !ndsFile.SaveAs( destFileName ) )
		{
			cout << "另存新rom错误！" << endl;
			return -1;
		}

		cout << "rom码表替换完成，请在CrystalTile里替换字库（以后会将该功能集成到本程序里）" << endl;
		return 0;
	}

	return 0;
}

