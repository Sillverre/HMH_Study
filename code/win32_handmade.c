#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h> //TODO: à delete, Juste car OutputDebugString ne marche pas -_-

#define PI32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

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
static LPDIRECTSOUNDBUFFER SecondaryBuffer;


struct HMH_Window_dimension{
    int Width;
    int Height;
};
//// Xinput 
// Xinput functions to import
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserId, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserId, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
//
static void HMH_LoadXinput(void){
    HMODULE XinputLib= LoadLibraryA("Xinput1_4.dll"); // TODO: tester sur windows 8
    if(!XinputLib){
        XinputLib= LoadLibraryA("Xinput1_3.dll");
    }
    if(!XinputLib){
        XinputLib= LoadLibraryA("Xinput9_1_0.dll");
    }
    if(XinputLib){
        XInputGetState = (x_input_get_state*) GetProcAddress(XinputLib,"XInputGetState");
        XInputSetState = (x_input_set_state*) GetProcAddress(XinputLib,"XInputSetState");
    }
}
//// DirectSound
//

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID lpGUID,LPDIRECTSOUND *ppDS,LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static void HMH_InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize){
    //NOTE: load library
    HMODULE DSoundLib= LoadLibraryA("dsound.dll");
    if (DSoundLib){
        //NOTE: Get DSound object
        direct_sound_create* DirectSoundCreate = (direct_sound_create*) GetProcAddress(DSoundLib,"DirectSoundCreate");

        LPDIRECTSOUND DSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DSound, 0))){

            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * WaveFormat.nSamplesPerSec;
            WaveFormat.cbSize = 0;

            if(SUCCEEDED(DSound->lpVtbl->SetCooperativeLevel(DSound, Window, DSSCL_PRIORITY))){
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                //NOTE: Make a primary buffer (not a real buffer, it's just to set up the sound card with SetFormat. Legacy logic -_-)
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DSound->lpVtbl->CreateSoundBuffer(DSound, &BufferDescription, &PrimaryBuffer, 0))){ 
                    
                    HRESULT error = PrimaryBuffer->lpVtbl->SetFormat(PrimaryBuffer, &WaveFormat);
                    if(SUCCEEDED(error)){
                        OutputDebugStringA("Primary Buffer format was set.\n");
                    }
                    else{
                        //TODO: diagnostic
                    }
                }
                else{
                    //TODO: diagnostic
                }
            }
            else{
                //TODO: diagnostic
            }

            //NOTE: Make a secondary buffer
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat; //TODO: à vérifier
            HRESULT error = DSound->lpVtbl->CreateSoundBuffer(DSound, &BufferDescription, &SecondaryBuffer, 0);
            if(SUCCEEDED(error)){ 
                //NOTE: Launch
            }
            

            
        }
        else{

        }
    }
    else{

    }
}



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
    buffer->Memory = VirtualAlloc(0, BitMapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

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
        BOOL altWasDown = ((lParam & (1 << 29)) != 0);
        if ((VKCode == VK_F4) && (altWasDown)){
            bRunning = 0;
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
        result = DefWindowProcA(hWnd,Msg,wParam,lParam);
        break;
    }

    return result;
}
struct HMH_sound_output{
    int SamplesPerSec;
    int ToneHz;
    int16 ToneVolume;
    uint32 RunningSampleId;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    real32 tSine;
    int latencySampleCnt;
};

static void HMH_fillSoundBuffer(struct HMH_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite){
    VOID* region1;
    DWORD region1Size;
    VOID* region2;
    DWORD region2Size;
    
    if(SUCCEEDED(SecondaryBuffer->lpVtbl->Lock(SecondaryBuffer, ByteToLock, BytesToWrite, &region1, &region1Size, &region2, &region2Size, 0))){

        //TODO: assert regionSizes are valid
        int16* SampleOut = (int16*) region1;
        DWORD region1SampleCnt = region1Size/SoundOutput->BytesPerSample;
        for(DWORD SampleId = 0; SampleId < region1SampleCnt; ++SampleId){
            
            real32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16) (SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            SoundOutput->tSine += 2.0f * PI32 * 1.0f / (real32) SoundOutput->WavePeriod;
            ++SoundOutput->RunningSampleId;
        }

        SampleOut = (int16*) region2;
        DWORD region2SampleCnt = region2Size/SoundOutput->BytesPerSample;
        for(DWORD SampleId = 0; SampleId < region2SampleCnt; ++SampleId){
            
            real32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16) (SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;
            SoundOutput->tSine += 2.0f * PI32 * 1.0f / (real32) SoundOutput->WavePeriod;
            ++SoundOutput->RunningSampleId;
        }

        SecondaryBuffer->lpVtbl->Unlock(SecondaryBuffer, region1, region1Size, region2, region2Size);
    }
}



int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    
    LARGE_INTEGER PerfCntFreqResult;
    QueryPerformanceFrequency(&PerfCntFreqResult);
    int64 PerfCntFreq = PerfCntFreqResult.QuadPart;

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

            HDC DContextPaint = GetDC(WindowHandle);

            int XOffset = 0;
            int YOffset = 0;

            //// Sound init
            //
            struct HMH_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSec = 40000;
            SoundOutput.ToneHz = 512;
            SoundOutput.ToneVolume = 3000;
            SoundOutput.RunningSampleId = 0;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSec/SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSec * SoundOutput.BytesPerSample;
            SoundOutput.latencySampleCnt = SoundOutput.SamplesPerSec / 15;

            HMH_InitDSound(WindowHandle, SoundOutput.SamplesPerSec, SoundOutput.SecondaryBufferSize);
            HMH_fillSoundBuffer(&SoundOutput, 0, (SoundOutput.latencySampleCnt * SoundOutput.BytesPerSample));
            SecondaryBuffer->lpVtbl->Play(SecondaryBuffer, 0, 0, DSBPLAY_LOOPING);
            //

            //Start Performance counters
            LARGE_INTEGER LastCnt;
            QueryPerformanceCounter(&LastCnt);
            uint64 LastCycleCnt = __rdtsc();

            //Game Loop
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

                        XOffset += StickX / 4096;
                        YOffset += StickY / 4096;

                        SoundOutput.ToneHz = 512 + (int) (256.0f*((real32)StickY / 30000.0f));
                        SoundOutput.WavePeriod = SoundOutput.SamplesPerSec/SoundOutput.ToneHz;
                    }
                    else{
                        //pas dispo
                    }
                }
                RenderWeirdGradient( &BackBuffer,XOffset, YOffset);

                //NOTE: DSound output test
                DWORD PlayCursor;
                DWORD WriteCursor;
                if(SUCCEEDED(SecondaryBuffer->lpVtbl->GetCurrentPosition(SecondaryBuffer, &PlayCursor, &WriteCursor))){

                    DWORD ByteToLock  = (SoundOutput.RunningSampleId * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                    DWORD targetCursor = (PlayCursor + (SoundOutput.latencySampleCnt * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite;
                    if (ByteToLock > targetCursor){
                        BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                        BytesToWrite += targetCursor;
                    }
                    else{
                        BytesToWrite = targetCursor - ByteToLock;
                    }

                    HMH_fillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
                    
                }

                struct HMH_Window_dimension Dimension = getWindowDimension(WindowHandle);

                HMH_DisplayBufferInWindow(&BackBuffer, DContextPaint, Dimension.Width, Dimension.Height);


                ////
                // Performance counters
                uint64 EndCycleCnt = __rdtsc();
                LARGE_INTEGER EndCnt;
                QueryPerformanceCounter(&EndCnt);

                //TODO: Display PerformanceCnt
                uint64 CycleElapsed = EndCycleCnt - LastCycleCnt;
                int64 CntElapsed = EndCnt.QuadPart - LastCnt.QuadPart;
                real32 msPerFrame = (1000.0f * (real32) CntElapsed) / (real32) PerfCntFreq;
                real32 FPS = (real32) PerfCntFreq / (real32) CntElapsed;
                real32 megaCyclePerFrame = ((real32) CycleElapsed /(1000.0f*1000.0f));

                char Buffer[256];
                sprintf(Buffer, "ms/frame: %f ms // FPS: %f // MegaCycle/frame: %f \n", msPerFrame, FPS, megaCyclePerFrame);
                printf("%s", Buffer);
                fflush(stdout);

                LastCnt = EndCnt;
                LastCycleCnt = EndCycleCnt;
                ////
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