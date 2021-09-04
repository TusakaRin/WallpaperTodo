//#include <iostream>
//#include <Windows.h>
//
//
//int main()
//{	
//	//const wchar_t *filenm = L"E:/Fun/image.png";
//	//SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void *)filenm, SPIF_UPDATEINIFILE);		
//	
//	return 0;
//}
#include <png.h>
#define cimg_use_png 1
#include <CImg.h>

#include <iostream>
#include <locale>
#include <codecvt>
#include <windows.h>
#include <WinBase.h>
#include <atlstr.h>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

void WatchDirectory(LPTSTR, std::shared_ptr<spdlog::logger>);
using namespace std;
using namespace cimg_library;

void HideConsole()
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

int writeImage(char* filename, int width, int height, char* title)
{
    int code = 0;
    FILE* fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep row = NULL;

    // Open file for writing (binary mode)
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        code = 1;
        goto finalise;
    }

    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        code = 1;
        goto finalise;
    }

    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Could not allocate info struct\n");
        code = 1;
        goto finalise;
    }

    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        code = 1;
        goto finalise;
    }

    png_init_io(png_ptr, fp);

    // Write header (8 bit colour depth)
    png_set_IHDR(png_ptr, info_ptr, width, height,
        8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // Set title
    if (title != NULL) {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = "Title";
        title_text.text = title;
        png_set_text(png_ptr, info_ptr, &title_text, 1);
    }

    png_write_info(png_ptr, info_ptr);

    // Allocate memory for one row (3 bytes per pixel - RGB)
    row = (png_bytep)malloc(3 * width * sizeof(png_byte));

    // Write image data
    int x, y;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (x == 0 || x == (width - 1) || y == 0 || y == (height - 1))
            {
                row[x * 3 + 0] = 0x00;
                row[x * 3 + 1] = 0x00;
                row[x * 3 + 2] = 0x00;
            }
            else
            {
                row[x * 3 + 0] = 0x00;
                row[x * 3 + 1] = 0x00;
                row[x * 3 + 2] = 0xff;
            }
        }
        png_write_row(png_ptr, row);
    }

    // End write
    png_write_end(png_ptr, NULL);

finalise:
    if (fp != NULL) fclose(fp);
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    if (row != NULL) free(row);

    return code;
}


void _tmain(int argc, TCHAR* argv[])
{
    if (argc != 2)
    {
        _tprintf(TEXT("Usage: %s <dir>\n"), argv[0]);
        return;
    }
    auto console = spdlog::stdout_color_mt("console");
    HideConsole();
    // auto my_logger = spdlog::basic_logger_mt("file_logger", "logs/basic-log.txt");
    WatchDirectory(argv[1], console);
}

string UTF8ToANSI(string s)
{
    BSTR    bstrWide;
    char* pszAnsi;
    int     nLength;
    const char* pszCode = s.c_str();

    nLength = MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, NULL, NULL);
    bstrWide = SysAllocStringLen(NULL, nLength);

    MultiByteToWideChar(CP_UTF8, 0, pszCode, strlen(pszCode) + 1, bstrWide, nLength);

    nLength = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    pszAnsi = new char[nLength];

    WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, pszAnsi, nLength, NULL, NULL);
    SysFreeString(bstrWide);

    string r(pszAnsi);
    delete[] pszAnsi;
    return r;
}


void WatchDirectory(LPTSTR lpDir, std::shared_ptr<spdlog::logger> logger)
{
    DWORD dwWaitStatus;
    HANDLE dwChangeHandles[2];
    TCHAR lpDrive[4];
    TCHAR lpFile[_MAX_FNAME];
    TCHAR lpExt[_MAX_EXT];

    DWORD lastUpdateTime = GetTickCount();
    int cnt = 0;
    logger->info("System started at {}", lastUpdateTime);

    _tsplitpath_s(lpDir, lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

    lpDrive[2] = (TCHAR)'\\';
    lpDrive[3] = (TCHAR)'\0';

    dwChangeHandles[0] = FindFirstChangeNotification(
        lpDir,                         // directory to watch 
        FALSE,                         // do not watch subtree 
        FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes 

    if (dwChangeHandles[0] == INVALID_HANDLE_VALUE)
    {
        printf("\n ERROR: FindFirstChangeNotification function failed.\n");
        ExitProcess(GetLastError());
    }

    if ((dwChangeHandles[0] == NULL))// || (dwChangeHandles[1] == NULL))
    {
        printf("\n ERROR: Unexpected NULL from FindFirstChangeNotification.\n");
        ExitProcess(GetLastError());
    }

    while (TRUE)
    {
        dwWaitStatus = WaitForMultipleObjects(1, dwChangeHandles,
            FALSE, INFINITE);
        switch (dwWaitStatus)
        {
        case WAIT_OBJECT_0:

            if (GetTickCount() - lastUpdateTime < 5000) {
                cnt++;
                logger->info("dir {} changed! count: {}", CW2A(lpDir), to_string(cnt));
            }
            else {  // new saving
                lastUpdateTime = GetTickCount();
                cnt = 0;
                logger->info("dir {} changed! count: {}", CW2A(lpDir), to_string(cnt));
            }
            if(cnt == 2){
                ifstream fin;
                string line;

                fin.open(string(CW2A(lpDir)) + "/todolist.txt", ios::in);

                vector<string> todolist;
                while (getline(fin, line)) {
                    cout << "Adding job: " << UTF8ToANSI(line) << endl;
                    todolist.emplace_back(line);
                }
                fin.close();


                // int result = writeImage("result.png", 50, 50, "This is my test image");
                // cout << result << endl;
                CImg<float> image;
                string path = "./resources/base.png";
                image.load_png(path.c_str());
                unsigned char white[] = { 255, 255, 255};
                for(long i=0; i < todolist.size(); i++) {
                    image.draw_text(300, 300 + (int)(i * 20), todolist[i].c_str(), white, 0, 1, 16);
                }
                image.save_png("./resources/result.png");
                const wchar_t *filenm = L"E:/Learning/projects/WallpaperTodo/Debug/resources/result.png";
                // const wchar_t *filenm = L"E:/Fun/image2.png";
                SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (void *)filenm, SPIF_UPDATEINIFILE);		
                cout << GetLastError() << endl;
            }

            if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE)
            {
                printf("\n ERROR: FindNextChangeNotification function failed.\n");
                ExitProcess(GetLastError());
            }
            break;

        case WAIT_TIMEOUT:

            // A timeout occurred, this would happen if some value other 
            // than INFINITE is used in the Wait call and no changes occur.
            // In a single-threaded environment you might not want an
            // INFINITE wait.

            printf("\nNo changes in the timeout period.\n");
            break;

        default:
            printf("\n ERROR: Unhandled dwWaitStatus.\n");
            ExitProcess(GetLastError());
            break;
        }
    }
}
