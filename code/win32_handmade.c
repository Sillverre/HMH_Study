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

struct HMH_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int bytesPerPixel;
};

static struct HMH_offscreen_buffer BackBuffer;

struct HMH_Window_dimension{
    int Width;
    int Height;
};

struct HMH_Window_dimension getWindowDimension(HWND window){
    struct HMH_Window_dimension WD;
    RECT ClientRect;
    GetClientRect(window, &ClientRect);
    WD.Height = ClientRect.bottom - ClientRect.top;
    WD.Width = ClientRect.right - ClientRect.left;

    return WD;
}

void RenderWeirdGradient(struct HMH_offscreen_buffer buffer, int XOffset, int YOffset){

    //TODO: check si passage par value est mieux
    
    uint8 *Row = (uint8*) buffer.Memory;
    for (int Y = 0; Y < buffer.Height; ++Y){
        uint32 *Pixel = (uint32*) Row; 

        for (int X = 0; X < buffer.Width; ++X){
            uint8 blue = (X + XOffset);
            uint8 green = (Y + YOffset);

            *Pixel++ = (green << 8 | blue);
            
        }
        Row += buffer.Pitch;
    }

}


void HMH_ResizeDIBSection(struct HMH_offscreen_buffer *buffer,int Width, int Height){

    // TODO: tester les perfs

    if(buffer->Memory){
        VirtualFree(buffer->Memory,0, MEM_RELEASE);
    }

    buffer->Width = Width;
    buffer->Height = Height;
    buffer->bytesPerPixel = 4;


    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height; //negative pour qu'en mémoire, les lignes descendent
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitMapMemorySize = (buffer->Width*buffer->Height) * buffer->bytesPerPixel;
    buffer->Memory = VirtualAlloc(0, BitMapMemorySize, MEM_COMMIT,PAGE_READWRITE);

    buffer->Pitch = Width*buffer->bytesPerPixel;
}

void HMH_DisplayBufferInWindow(HDC DContextPaint, int WindowWidth, int WindowHeight, struct HMH_offscreen_buffer buffer,int X, int Y, int Width, int Height){
    
    //TODO : change the aspect ratio
    StretchDIBits(DContextPaint,0,0, WindowWidth, WindowHeight, 0,0, buffer.Width, buffer.Height,buffer.Memory, &(buffer.Info), DIB_RGB_COLORS, SRCCOPY);
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


        struct HMH_Window_dimension Dimension = getWindowDimension(hWnd);
        HMH_DisplayBufferInWindow( DContextPaint, Dimension.Width, Dimension.Height, BackBuffer,X,Y, Width, Height);
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
    HMH_ResizeDIBSection(&BackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
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
                RenderWeirdGradient( BackBuffer,XOffset, YOffset);

                HDC DContextPaint = GetDC(WindowHandle);
                struct HMH_Window_dimension Dimension = getWindowDimension(WindowHandle);

                HMH_DisplayBufferInWindow( DContextPaint, Dimension.Width, Dimension.Height, BackBuffer,0, 0, Dimension.Width, Dimension.Height);
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