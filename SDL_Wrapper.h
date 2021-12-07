#ifndef SDL_WRAPPER_H
#define SDL_WRAPPER_H

/* we need to include SDL.h here even though this header doesn't need it because
* otherwise there will be linker problems-no WinMain-etc...don't believe me? Comment it out.
*/
#include "SDL.h"

struct _Mix_Music;
struct Mix_Chunk;
struct SDL_Texture;

struct Color {
	int r, g, b;
	Color(int r_, int g_, int b_) {
		r = r_;
		g = g_;
		b = b_;
	}
};

struct Image {
	SDL_Texture*	texture;
	int				x, y, w, h;
};

struct Music {
	struct _Mix_Music*  music_chunk;
};

struct Sound {
	Mix_Chunk*  mix_chunk;
};

extern const Color Black, Grey, White, Red, Pink, DarkBrown, Brown, Orange, Yellow, DarkGreen,
Green, LightGreen, DarkBlue, Blue, LightBlue, PaleBlue, Magenta;

extern const int PlayForever; // it's -1, don't tell anybody

extern const char MinusKey, EqualsKey, BackQuoteKey, LeftBracketKey, RightBracketKey,
BackslashKey, SemicolonKey, QuoteKey, CommaKey, PeriodKey, SlashKey, SpaceKey,
TabKey, LeftShiftKey, LeftControlKey, LeftAltKey, UpKey, DownKey, LeftKey,
RightKey, EnterKey;

void InitSystem(int window_width, int window_height);
void CloseSystem();
int GetWindowHeight();
int GetWindowWidth();

bool QueryError();
const char* GetErrorMsg();
void WriteLog(const char* s);

Music LoadMusic(const char* filename);
void PlayMusic(Music& s, int num_loops);
void SetMusicVolume(float vol);
void RestartMusic();
void PauseMusic();
void ResumeMusic();
bool IsMusicPaused();

Sound LoadSound(const char* filename);
void PlaySound(Sound& sound, float volume = 1.0);

void ClearScreen();
void DrawPixel(int x, int y, const Color& c);
void DrawPixel(int x, int y, int r, int g, int b);
void DrawLine(int x1, int y1, int x2, int y2, const Color& c);
void Refresh();

Image LoadImage(const char* filename);
Image CropImage(Image& im, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
void DrawImage(Image& im, int x, int y);
void FillRect(int x, int y, int w, int h, const Color& c);

bool WasKeyPressed(char c);
bool IsKeyDown(char c);

void Sleep(int ms);

bool IsRunning();

void WriteString(const char* s, int x1, int y1);
void WriteInt(int n, int x1, int y1);
void WriteChar(char c, int x1, int y1);

enum {
	LEFT_MOUSE_BUTTON, MIDDLE_MOUSE_BUTTON, RIGHT_MOUSE_BUTTON, NUM_MOUSE_BUTTONS
};

int GetMouseX();
int GetMouseY();
bool IsMouseButtonDown(int button);
bool WasMouseButtonPressed(int button);
#endif