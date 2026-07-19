#include <windows.h>
#include <stdint.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;



// TODO: à bouger plus tard
static BOOL bRunning;
static BITMAPINFO BitMapInfo;
static void *BitMapMemory;

//TEMP
static int BitMapWidth;
static int BitMapHeight;
static int bytesPerPixel = 4;


void RenderWeirdGradient(int XOffset, int YOffset){

    int Width = BitMapWidth;
    //int Height = BitMapHeight;
    

    int Pitch = Width*bytesPerPixel;
    uint8 *Row = (uint8*) BitMapMemory;
    for (int Y = 0; Y < BitMapHeight; ++Y){
        uint32 *Pixel = (uint32*) Row; 

        for (int X = 0; X < BitMapWidth; ++X){
            uint8 blue = (X + XOffset);
            uint8 green = (Y + YOffset);

            *Pixel++ = (green << 8 | blue);
            
        }
        Row += Pitch;
    }

}


void HMH_ResizeDIBSection(int Width, int Height){

    // TODO: tester les perfs

    if(BitMapMemory){
        VirtualFree(BitMapMemory,0, MEM_RELEASE);
    }

    BitMapWidth = Width;
    BitMapHeight = Height;


    BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
    BitMapInfo.bmiHeader.biWidth = BitMapWidth;
    BitMapInfo.bmiHeader.biHeight = -BitMapHeight; //negative pour qu'en mémoire, les lignes descendent
    BitMapInfo.bmiHeader.biPlanes = 1;
    BitMapInfo.bmiHeader.biBitCount = 32;
    BitMapInfo.bmiHeader.biCompression = BI_RGB;

    int BitMapMemorySize = (BitMapWidth*BitMapHeight) * bytesPerPixel;
    BitMapMemory = VirtualAlloc(0, BitMapMemorySize, MEM_COMMIT,PAGE_READWRITE);

    //TODO : clear to black
}

void HMH_UpdateWindow(HDC DContextPaint, RECT *ClientRect, int X, int Y, int Width, int Height){

    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    //StretchDIBits(DContextPaint,X,Y, Width, Height, X,Y, Width, Height, BitMapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
    StretchDIBits(DContextPaint,0,0, BitMapWidth, BitMapHeight, X,Y, WindowWidth, WindowHeight, BitMapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
HMH_MainWindowCallback(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result = 0;

    switch (Msg)
    {
    case WM_SIZE:
        OutputDebugStringA("WM_SIZE");
        RECT ClientRect;
        GetClientRect(hWnd, &ClientRect);
        int Height = ClientRect.bottom - ClientRect.top;
        int Width = ClientRect.right - ClientRect.left;
        HMH_ResizeDIBSection(Width,Height);
        break;
    case WM_DESTROY:
        OutputDebugStringA("WM_DESTROY");
        bRunning = 0;
        break;
    case WM_CLOSE:
        OutputDebugStringA("WM_CLOSE");
        bRunning = 0;
        break;        
    case WM_ACTIVATEAPP:
        OutputDebugStringA("WM_ACTIVATEAPP");
        break;
    case WM_PAINT:{
        OutputDebugStringA("WM_PAINT");
        PAINTSTRUCT paintStruct;
        HDC DContextPaint = BeginPaint(hWnd, &paintStruct);
        
        int X = paintStruct.rcPaint.left;
        int Y = paintStruct.rcPaint.top;
        int Height = paintStruct.rcPaint.bottom - paintStruct.rcPaint.top;
        int Width = paintStruct.rcPaint.right - paintStruct.rcPaint.left;

        RECT ClientRect;
        GetClientRect(hWnd, &ClientRect);
        HMH_UpdateWindow( DContextPaint, &ClientRect ,X,Y, Width, Height);
        EndPaint(hWnd, &paintStruct);
    }
        break;
    default:
        //OutputDebugStringA("default");
        result = DefWindowProc(hWnd,Msg,wParam,lParam);
        break;
    }

    return result;
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    
    WNDCLASSA WindowClass = {0};
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = HMH_MainWindowCallback;
    WindowClass.hInstance = hInstance;
    WindowClass.lpszClassName = "HandMadeHeroWindowClass";

    if (RegisterClass(&WindowClass)){
        HWND WindowHandle = CreateWindowEx(
            0, 
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            hInstance,
            0);
        if(WindowHandle){
            bRunning = 1;
            int XOffset = 0;
            int YOffset = 0;
            while(bRunning){
            
                MSG message;
                while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)){
                    if (message.message == WM_QUIT){
                        bRunning = 0;
                    }

                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                RenderWeirdGradient(XOffset, YOffset);

                HDC DContextPaint = GetDC(WindowHandle);
                RECT ClientRect;
                GetClientRect(WindowHandle, &ClientRect);

                int WindowWidth = ClientRect.bottom - ClientRect.top;
                int WindowHeight = ClientRect.right - ClientRect.left;

                HMH_UpdateWindow( DContextPaint, &ClientRect , 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(WindowHandle, DContextPaint);

                ++XOffset;
            }
        }
        else{
            MessageBox(0,"La fenêtre a planté 2.","Test",MB_OK|MB_ICONINFORMATION);
        }
    }
    else{
        MessageBox(0,"La fenêtre a planté 1.","Test",MB_OK|MB_ICONINFORMATION);
    }

    return 0;
}