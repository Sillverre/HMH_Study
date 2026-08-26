#include <windows.h>
#include <stdint.h>
#include <xinput.h>

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
//// Xinput 
// Xinput functions to import
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserId, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
    return 0;
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserId, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
    return 0;
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
//
static void HMH_LoadXinput(void){
    HMODULE XinputLib= LoadLibraryA("Xinput1_3.dll");
    if(XinputLib){
        XInputGetState = (x_input_get_state*) GetProcAddress(XinputLib,"XInputGetState");
        XInputSetState = (x_input_set_state*) GetProcAddress(XinputLib,"XInputSetState");
    }
}
////



struct HMH_Window_dimension getWindowDimension(HWND window){
    struct HMH_Window_dimension WD;
    RECT ClientRect;
    GetClientRect(window, &ClientRect);
    WD.Height = ClientRect.bottom - ClientRect.top;
    WD.Width = ClientRect.right - ClientRect.left;

    return WD;
}

void RenderWeirdGradient(struct HMH_offscreen_buffer *buffer, int XOffset, int YOffset){

    //TODO: check si passage par value est mieux
    
    uint8 *Row = (uint8*) buffer->Memory;
    for (int Y = 0; Y < buffer->Height; ++Y){
        uint32 *Pixel = (uint32*) Row; 

        for (int X = 0; X < buffer->Width; ++X){
            uint8 blue = (X + XOffset);
            uint8 green = (Y + YOffset);

            *Pixel++ = (green << 8 | blue);
            
        }
        Row += buffer->Pitch;
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

void HMH_DisplayBufferInWindow( struct HMH_offscreen_buffer *buffer, HDC DContextPaint, int WindowWidth, int WindowHeight){
    
    //TODO : change the aspect ratio
    StretchDIBits(DContextPaint,0,0, WindowWidth, WindowHeight, 0,0, buffer->Width, buffer->Height,buffer->Memory, &(buffer->Info), DIB_RGB_COLORS, SRCCOPY);
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

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        uint32 VKCode = wParam;
        BOOL wasDown = ((lParam & (1 << 30)) != 0);
        BOOL isDown = ((lParam & (1 << 31)) == 0);
        if(isDown != wasDown){
            if(VKCode == 'Z'){

            }
            else if(VKCode == 'Q'){

            }
            else if(VKCode == 'S'){

            }
            else if(VKCode == 'D'){

            }
            else if(VKCode == 'A'){

            }
            else if(VKCode == 'E'){

            }
            else if(VKCode == VK_UP){

            }
            else if(VKCode == VK_DOWN){

            }
            else if(VKCode == VK_LEFT){

            }
            else if(VKCode == VK_RIGHT){

            }
            else if(VKCode == VK_SPACE){

            }
            else if(VKCode == VK_ESCAPE){

            }
        }
        break;
    }
        
    case WM_PAINT:{
        OutputDebugStringA("WM_PAINT");
        PAINTSTRUCT paintStruct;
        HDC DContextPaint = BeginPaint(hWnd, &paintStruct);

        struct HMH_Window_dimension Dimension = getWindowDimension(hWnd);
        HMH_DisplayBufferInWindow(&BackBuffer, DContextPaint, Dimension.Width, Dimension.Height);
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
    
    HMH_LoadXinput();

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
                
                // TODO: poll plus souvent qu'à chaque frame
                for (DWORD controllerId = 0; controllerId < XUSER_MAX_COUNT; controllerId++){
                    XINPUT_STATE controllerState;
                    if(XInputGetState(controllerId, &controllerState) == ERROR_SUCCESS){
                        //branché
                        //TODO: check si le numéro de packet dwPacketNumber grossit trop vite
                        XINPUT_GAMEPAD *Pad = &controllerState.Gamepad;

                        BOOL padU = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        BOOL padD = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        BOOL padL = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        BOOL padR = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        BOOL padStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        BOOL padBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        BOOL padLT = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
                        BOOL padRT = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
                        BOOL padLS = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        BOOL padRS = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        BOOL padA = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        BOOL padB = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        BOOL padX = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        BOOL padY = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        if(padA){
                            YOffset += 2;
                        }
                         if(padY){
                            YOffset -= 2;
                        }
                        else if(padB){
                            XOffset += 2;
                        }
                        else if(padX){
                            XOffset -= 2;
                        }
                    }
                    else{
                        //pas dispo
                    }
                }
                XINPUT_VIBRATION Vibration;
                Vibration.wLeftMotorSpeed = 60000;
                Vibration.wRightMotorSpeed = 60000;
                //XInputSetState(0, &Vibration);

                RenderWeirdGradient( &BackBuffer,XOffset, YOffset);

                HDC DContextPaint = GetDC(WindowHandle);
                struct HMH_Window_dimension Dimension = getWindowDimension(WindowHandle);

                HMH_DisplayBufferInWindow(&BackBuffer, DContextPaint, Dimension.Width, Dimension.Height);
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