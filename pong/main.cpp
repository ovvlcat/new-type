//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;

// секция данных игры  
typedef struct
{
    float x, y, width, height, rad, dx, dy, speed, gravity, jump, jumpheight;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
    bool isJumping;         // флаг прыжка
    bool isOnGround;        // на земле ли
} sprite;

typedef struct //структура для платформ
{
    float x, y, width, height;
    HBITMAP hBitmap;
} platform;

typedef struct //структура для локации
{
    float groundLevel;      // уровень земли
    vector<platform>platforms; // платформы в локации
    HBITMAP background;     // фон локации
} location;

sprite racket;//ракетка игрока
sprite cube;
struct
{
    int score, balls;//количество набранных очков и оставшихся "жизней"
    bool action = false;//состояние - ожидание (игрок должен нажать пробел) или игра
} game;

struct
{
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
}   window;

HBITMAP hBack;// хэндл для фонового изображения

//cекция кода

void InitGame()
{
    //в этой секции загружаем спрайты с помощью функций gdi
    //пути относительные - файлы должны лежать рядом с .exe 
    //результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
    racket.hBitmap = (HBITMAP)LoadImageA(NULL, "rash.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    cube.hBitmap = (HBITMAP)LoadImageA(NULL, "rash.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    hBack = (HBITMAP)LoadImageA(NULL, "back2.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    racket.width = 128; //хитбокс
    racket.height = 210;
    racket.speed = 15;//скорость перемещения ракетки
    racket.x = window.width / 2.;//ракетка посередине окна
    racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракетки
    racket.jump = 30;
    racket.jumpheight = racket.y - (racket.jump*2.f);
    racket.gravity = 5;

    cube.width = 300;
    cube.height = 300;
    cube.x = window.width/4;
    cube.y = window.height/ 2;
}
void Jump()
{
    racket.y -= racket.jump;
}

void ShowScore()
{
    //поиграем шрифтами и цветами
    SetTextColor(window.context, RGB(160, 160, 160));
    SetBkColor(window.context, RGB(0, 0, 0));
    SetBkMode(window.context, TRANSPARENT);
    auto hFont = CreateFont(70, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "CALIBRI");
    auto hTmp = (HFONT)SelectObject(window.context, hFont);

    char txt[32];//буфер для текста
    _itoa_s(racket.y, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
    TextOutA(window.context, 10, 100, "HeroY", 5);
    TextOutA(window.context, 200, 100, (LPCSTR)txt, strlen(txt));

    _itoa_s(racket.x, txt, 10);//преобразование числовой переменной в текст. текст окажется в переменной txt
    TextOutA(window.context, 10, 150, "HeroX", 5);
    TextOutA(window.context, 200, 150, (LPCSTR)txt, strlen(txt));
}

void ProcessInput()
{
    if (GetAsyncKeyState(VK_LEFT)) racket.x -= racket.speed;
    if (GetAsyncKeyState(VK_RIGHT)) racket.x += racket.speed;
    //if (GetAsyncKeyState(VK_UP)) racket.y -= racket.speed;
    if (GetAsyncKeyState(VK_DOWN)) racket.y += racket.speed;
    if (GetAsyncKeyState(VK_UP))
    {
        Jump();
    };

    if (racket.y < 100)
    {
        racket.jump = 0.f;
    }
}

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
    HBITMAP hbm, hOldbm;
    HDC hMemDC;
    BITMAP bm;

    hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

    if (hOldbm) // Если не было ошибок, продолжаем работу
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

       
        
        StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
        

        SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
    }
    DeleteDC(hMemDC); // Удаляем контекст памяти
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон
    ShowBitmap(window.context, racket.x, racket.y, racket.width, racket.height, racket.hBitmap);//ракетка игрока
    ShowBitmap(window.context, cube.x , cube.y, cube.width, cube.height, cube.hBitmap); //отображение платформы
}

void LimitRacket()
{
    racket.x = max(racket.x, racket.width / 2.);//если коодината левого угла ракетки меньше нуля, присвоим ей ноль
    racket.x = min(racket.x, window.width - racket.width / 2.);//аналогично для правого угла
    racket.y = max(racket.y, racket.height * 0.1);
    racket.y = min(racket.y, window.height - racket.height * 3.2);
}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
    window.width = r.right - r.left;//определяем размеры и сохраняем
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//второй буфер
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
    GetClientRect(window.hWnd, &r);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры

    //mciSendString(TEXT("play ..\\Debug\\music.mp3 repeat"), NULL, 0, NULL);
    ShowCursor(NULL);
    
    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        racket.y += racket.gravity;

        ShowRacketAndBall();//рисуем фон, ракетку и шарик
        ShowScore();//рисуем очик и жизни
        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)

        ProcessInput();//опрос клавиатуры
        
        LimitRacket();//проверяем, чтобы ракетка не убежала за экран

        if (racket.y >= 407.f)
        {
            racket.jump = 20.f;
        }
        //ProcessBall();//перемещаем шарик
        //ProcessRoom();//обрабатываем отскоки от стен и каретки, попадание шарика в картетку
    }
}
//сделать так, чтобы пока персонаж находится на земле, гравитация была равна нулю и включалась когда он в воздухе. требуется запихать её в цикл, вытащив из основного while. попытаться сделать переход в другую локу. это делается через куб коллизии и телепортации персонажа в начало координат по х