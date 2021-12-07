#include "SDL_Wrapper.h"

/* 11-12-15 D. Wilckens Cleaned-up SDL Wrapper */

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "SDL_mixer.h"


#include <fstream>
#include <sstream>

#define MAX_TEXTURES 64
#define MAX_MUSIC_CHUNKS 64
#define NUM_SOUND_CHANNELS 8
#define MAX_MIX_CHUNKS 64
// we'll use magenta for color key:
#define COLORKEY_R 255
#define COLORKEY_G 0
#define COLORKEY_B 255

static std::ofstream err_log;

struct FontBank {
	SDL_Texture*    texture;
	SDL_Rect        src_rects[256];
	int				height;
};

struct SystemData {
	int				window_width,
	window_height;
	bool			running;
	SDL_Window*		window;
	SDL_Renderer*	renderer;
	bool			pressed_keys[256];  // what keys got pressed since last refresh?
	bool			down_keys[256];		// what keys are currently down?
	bool            mouse_button_states[3];
	bool            mouse_button_previous_states[3];
	int				texture_count;
	SDL_Texture*	loaded_textures[MAX_TEXTURES];
	FontBank		default_font;
	Mix_Music*		music_chunks[MAX_MUSIC_CHUNKS];
	int				music_chunk_count;
	int				next_sound_channel;  // every time we play another sound effect, bump this up, modulo NUM_SOUND_CHANNELS
	Mix_Chunk*		mix_chunks[MAX_MIX_CHUNKS];
	int				mix_chunk_count;

	bool				internal_error;
	std::stringstream   internal_error_message;
};

static SystemData sys;

const int PlayForever = -1;

static const unsigned int LOOKUP_NOT_FOUND = 255;

const Color Black = { 0, 0, 0 },
Grey = { 157, 157, 157 }, White = { 255, 255, 255 }, Red = { 190, 38, 51 },
Pink = { 224, 111, 139 }, DarkBrown = { 73, 60, 43 }, Brown = { 164, 100, 34 },
Orange = { 235, 137, 49 }, Yellow = { 247, 226, 107 }, DarkGreen = { 47, 72, 78 },
Green = { 68, 137, 26 }, LightGreen = { 163, 206, 39 }, DarkBlue = { 27, 38, 50 },
Blue = { 0, 87, 132 }, LightBlue = { 49, 162, 242 }, PaleBlue = { 178, 220, 239 },
Magenta = { 255, 0, 255 }, ColorKey = { 255, 0, 255 };

// forward declarations
static void CreateFontBank(FontBank* fb, const char* fontName, int pointSize, SDL_Color color);
static void FreeAllTextures();
static void FreeAllMusicAndSoundChunks();

void WriteLog(const char* s) {
	err_log << s << std::endl;
	err_log.flush();
}

void InitSystem(int window_width, int window_height) {
	err_log.open("Logfile.txt", std::ofstream::out);
	sys.window_width = window_width;
	sys.window_height = window_height;
	int sdl_init_success =
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_EVENTS);
	if (sdl_init_success != 0) {
		WriteLog("Couldn't initialize SDL.");
		WriteLog(SDL_GetError());
		exit(0);
	}

	int create_window_success = SDL_CreateWindowAndRenderer(window_width, window_height, SDL_WINDOW_BORDERLESS, &(sys.window), &(sys.renderer));
	if (create_window_success != 0) {
		WriteLog("Couldn't create window & renderer.");
		WriteLog(SDL_GetError());
		exit(0);
	}
	SDL_SetWindowPosition(sys.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	if (sys.window != NULL)
		sys.running = true;
	else sys.running = false;
	for (int i = 0; i < 256; i++)
		sys.down_keys[i] = false;
	int IMG_flags = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
	if (IMG_Init(IMG_flags) != IMG_flags) {
		WriteLog("Failed to initialize all desired image formats.");
		WriteLog(IMG_GetError());
	}
	if (TTF_Init() != 0) {
		WriteLog("Failed to initialize TrueTypeFonts.");
		WriteLog(TTF_GetError());
		exit(0);
	}

	SDL_Color White = { 255, 255, 255, 255 };
	CreateFontBank(&sys.default_font, "OpenSans-Regular.ttf", 32, White);

	sys.texture_count = 0;
	int Mix_flags = MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG;
	if (Mix_Init(Mix_flags) != Mix_flags) {
		WriteLog("Failed to initialize all desired sound formats.");
		WriteLog(Mix_GetError());
	}
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) != 0) {
		WriteLog("Failed to open audio.");
		WriteLog(Mix_GetError());
		exit(0);
	}
	sys.music_chunk_count = 0;

	Mix_AllocateChannels(NUM_SOUND_CHANNELS);
	sys.next_sound_channel = 0;
	sys.mix_chunk_count = 0;

	sys.internal_error = false;
}

void CloseSystem() {
	FreeAllMusicAndSoundChunks();
	Mix_CloseAudio();
	Mix_Quit();
	TTF_Quit();
	FreeAllTextures();
	SDL_DestroyTexture(sys.default_font.texture);
	IMG_Quit();
	SDL_DestroyRenderer(sys.renderer);
	SDL_DestroyWindow(sys.window);
	SDL_Quit();
	sys.running = false;
	err_log.close();
}

bool QueryError() {
	return sys.internal_error;
}

const char* GetErrorMsg() {
	sys.internal_error = false;
	return sys.internal_error_message.str().c_str();
}

// this ERASES any existing error messages...sorry!
static void SetError(const char* msg) {
	sys.internal_error = true;
	sys.internal_error_message.flush();
	sys.internal_error_message << msg;
}


Music LoadMusic(const char* filename) {
	if (sys.music_chunk_count >= MAX_MUSIC_CHUNKS) {
		WriteLog("Error:LoadMusic:Reached maximum number of music chunks.");
		return{ 0 };
	}

	Mix_Music* chunk = Mix_LoadMUS(filename);
	if (chunk == NULL) {
		WriteLog("Error: Couldn't load this music filename:");
		WriteLog(filename);
		return{ 0 };
	}
	else {
		Music out;
		out.music_chunk = chunk;
		sys.music_chunks[sys.music_chunk_count] = chunk;
		sys.music_chunk_count++;
		return out;
	}
}

// can use PlayForever for num_loops
void PlayMusic(Music& s, int num_loops) {
	Mix_PlayMusic(s.music_chunk, num_loops);
}

// vol should be 0.0 to 1.0
void SetMusicVolume(float vol) {
	Mix_VolumeMusic((int)(vol * MIX_MAX_VOLUME));
}

void RestartMusic() {
	Mix_RewindMusic();
}

void PauseMusic() {
	Mix_PauseMusic();
}

void ResumeMusic() {
	Mix_ResumeMusic();
}

bool IsMusicPaused() {
	return (Mix_PausedMusic() == 1) ? true : false;
}

// from SDL_mixer docs: 'This can load WAVE, AIFF, RIFF, OGG, and VOC files.'
Sound LoadSound(const char* filename) {
	if (sys.mix_chunk_count >= MAX_MIX_CHUNKS) {
		WriteLog("Error:LoadSound:Reached maximum number of mix chunks.");
		return{ 0 };
	}
	Sound out;
	out.mix_chunk = Mix_LoadWAV(filename);
	if (out.mix_chunk == NULL) {
		WriteLog("Error:LoadSound:Failed to load file:");
		WriteLog(filename);
		WriteLog(Mix_GetError());
		return{ 0 };
	}
	sys.mix_chunks[sys.mix_chunk_count] = out.mix_chunk;
	sys.mix_chunk_count++;
	Mix_VolumeChunk(out.mix_chunk, MIX_MAX_VOLUME);
	return out;
}

// volume: 0.0 = silent, 1.0 = max volume
void PlaySound(Sound& sound, float volume) {
	int channel = sys.next_sound_channel;
	Mix_Volume(channel, (int)(MIX_MAX_VOLUME * volume));
	Mix_PlayChannel(channel, sound.mix_chunk, 0);
	sys.next_sound_channel++;
	if (sys.next_sound_channel >= NUM_SOUND_CHANNELS)
		sys.next_sound_channel = 0;
}

static void FreeAllMusicAndSoundChunks() {
	for (int i = 0; i < sys.music_chunk_count; i++) {
		Mix_FreeMusic(sys.music_chunks[i]);
	}
	sys.music_chunk_count = 0;

	for (int i = 0; i < sys.mix_chunk_count; i++) {
		Mix_FreeChunk(sys.mix_chunks[i]);
	}
	sys.mix_chunk_count = 0;
}


int GetWindowHeight() {
	return sys.window_height;
}

int GetWindowWidth() {
	return sys.window_width;
}

void ClearScreen() {
	SDL_RenderClear(sys.renderer);
}

static void PushTexture(SDL_Texture* text) {
	//  KEEP TRACK OF ALL LOADED TEXTURES TO DESTROY 'EM LATER...
	if (sys.texture_count >= MAX_TEXTURES) {
		WriteLog("Out of texture space.");
		exit(0);
	}
	else {
		sys.loaded_textures[sys.texture_count] = text;
		sys.texture_count++;
	}
	return;
}

static void FreeAllTextures() {
	for (int i = 0; i < sys.texture_count; i++)
		SDL_DestroyTexture(sys.loaded_textures[i]);
	sys.texture_count = 0;
}

Image LoadImage(const char* filename) {
	Image out;
	out.texture = NULL;
	SDL_Surface* surface;
	surface = IMG_Load(filename);
	if (!surface) {
		err_log << "IMG_Load Failed:" << IMG_GetError() << std::endl;
		return{ 0 };
	}
	// colorkey is magenta:
	Uint32 colorkey = SDL_MapRGB(surface->format, COLORKEY_R, COLORKEY_G, COLORKEY_B);
	SDL_SetColorKey(surface, SDL_TRUE, colorkey);
	out.texture = SDL_CreateTextureFromSurface(sys.renderer, surface);
	out.x = 0;
	out.y = 0;
	out.w = surface->w;
	out.h = surface->h;
	SDL_FreeSurface(surface);
	PushTexture(out.texture);
	return out;
}

Image CropImage(Image& im, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	Image out;
	out.texture = im.texture;
	out.x = im.x + x;
	out.y = im.y + y;
	out.w = w;
	out.h = h;
	if (out.x + out.w > im.x + im.w)
		out.w = im.x + im.w - out.x;
	if (out.y + out.h > im.y + im.h)
		out.h = im.y + im.h - out.y;
	return out;
}

void DrawImage(Image& im, int x, int y) {
	SDL_Rect src, dest;
	src.x = im.x;
	src.y = im.y;
	src.w = im.w;
	src.h = im.h;
	if (x + src.w >= sys.window_width)
		src.w = sys.window_width - x;
	if (y + src.h >= sys.window_height)
		src.h = sys.window_height - y;
	dest.x = x;
	dest.y = y;
	dest.w = src.w;
	dest.h = src.h;
	SDL_RenderCopy(sys.renderer, im.texture, &src, &dest);
}

static void SetColor(const Color& c) {
	SDL_SetRenderDrawColor(sys.renderer, c.r, c.g, c.b, 0);
}

void FillRect(int x, int y, int w, int h, const Color& c) {
	SetColor(c);
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderFillRect(sys.renderer, &rect);
}

void DrawPixel(int x, int y, const Color& c) {
	SetColor(c);
	SDL_RenderDrawPoint(sys.renderer, x, y);
}

void DrawPixel(int x, int y, int r, int g, int b) {
	SDL_SetRenderDrawColor(sys.renderer, r, g, b, 0);
	SDL_RenderDrawPoint(sys.renderer, x, y);
}

void DrawLine(int x1, int y1, int x2, int y2, const Color& c) {
	SetColor(c);
	SDL_RenderDrawLine(sys.renderer, x1, y1, x2, y2);
}

static unsigned char LookupKeysym(SDL_Keycode sym);
static unsigned int LookupChar(char c);

static void RefreshKeys() {
	for (int i = 0; i < 256; i++)
		sys.pressed_keys[i] = false;
	SDL_Event e;
	unsigned int index = 0;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_KEYDOWN:
			if (e.key.repeat == 0) {
				index = LookupKeysym(e.key.keysym.sym);
				sys.pressed_keys[index] = true;
				sys.down_keys[index] = true;
			}
			break;
		case SDL_KEYUP:
			index = LookupKeysym(e.key.keysym.sym);
			sys.down_keys[index] = false;
			break;
		case SDL_QUIT:
			sys.running = false;
			break;
		default: break;
		}
	}
	// Update mouse button status
	for (int i = 0; i < NUM_MOUSE_BUTTONS; i++) {
		sys.mouse_button_previous_states[i] = sys.mouse_button_states[i];
		sys.mouse_button_states[i] = IsMouseButtonDown(i);
	}

}

void Refresh() {
	SDL_RenderPresent(sys.renderer);
	RefreshKeys();
}

const char MinusKey = '-', EqualsKey = '=', BackQuoteKey = '`', LeftBracketKey = '[', RightBracketKey = ']',
BackslashKey = '\\', SemicolonKey = ';', QuoteKey = '\'', CommaKey = ',', PeriodKey = '.', SlashKey = '/', SpaceKey = ' ',
TabKey = '\t', LeftShiftKey = 'L', LeftControlKey = 'C', LeftAltKey = 'A', UpKey = '^', DownKey = 'D', LeftKey = '<',
RightKey = '>', EnterKey = 'E';

// LookupKeysym, LookupChar: These two should get written to be better:
static unsigned char LookupKeysym(SDL_Keycode sym) {
	switch (sym) {
	case SDLK_MINUS: return MinusKey;
	case SDLK_EQUALS: return EqualsKey;
	case SDLK_BACKQUOTE: return BackQuoteKey;
	case SDLK_LEFTBRACKET: return LeftBracketKey;
	case SDLK_RIGHTBRACKET: return RightBracketKey;
	case SDLK_BACKSLASH: return BackslashKey;
	case SDLK_SEMICOLON: return SemicolonKey;
	case SDLK_QUOTE: return QuoteKey;
	case SDLK_COMMA: return CommaKey;
	case SDLK_PERIOD: return PeriodKey;
	case SDLK_SLASH: return SlashKey;
	case SDLK_SPACE: return SpaceKey;
	case SDLK_TAB: return TabKey;
	case SDLK_LSHIFT: return LeftShiftKey;
	case SDLK_LCTRL: return LeftControlKey;
	case SDLK_LALT: return LeftAltKey;
	case SDLK_UP: return UpKey;
	case SDLK_DOWN: return DownKey;
	case SDLK_LEFT: return LeftKey;
	case SDLK_RIGHT: return RightKey;
	case SDLK_RETURN: return EnterKey;
	default:
		unsigned int raw = (unsigned int)sym;
		if (raw < 256)
			return (unsigned char)raw;
		else return LOOKUP_NOT_FOUND;
	}
}

static unsigned int LookupChar(char c) {
	// cast to unsigned char to get 0-255, then we're fine.
	return  (unsigned char)c;
}

bool WasKeyPressed(char c) {
	unsigned int lookup_index = LookupChar(c);
	if (lookup_index == LOOKUP_NOT_FOUND)
		return false;
	else return sys.pressed_keys[lookup_index];
}

bool IsKeyDown(char c) {
	unsigned int lookup_index = LookupChar(c);
	if (lookup_index == LOOKUP_NOT_FOUND)
		return false;
	else return sys.down_keys[lookup_index];
}

void Sleep(int ms) {
	SDL_Delay(ms);
}

bool IsRunning() {
	return sys.running;
}

// FIXME: Skip all non-printable characters:
static bool IsPrintableChar(int i) {
	if (i < 32 || i > 127)
		return false;
	else return true;
}

static int RenderChar(int character, int x, int y, FontBank* font) {
	if (IsPrintableChar(character) == false)
		return 0;
	SDL_Rect dest;
	dest.x = x;
	dest.y = y;
	dest.w = font->src_rects[character].w;
	dest.h = font->src_rects[character].h;
	SDL_RenderCopy(sys.renderer, font->texture, &font->src_rects[character],
		&dest);
	return font->src_rects[character].w;
}

static void RenderText(const char* str, int x, int y, FontBank* font) {
	int running_x = x;
	while (*str != 0) {
		running_x += RenderChar(*str, running_x, y, font);
		if (*str == '\n') {
			running_x = x;
			y += font->height;
		}
		if (*str == ' ') {
			// we use the letter 'A's width for spaces:
			running_x += font->src_rects['A'].w;
		}
		str++;
	}
}

void WriteString(const char* s, int x1, int y1) {
	RenderText(s, x1, y1, &sys.default_font);
}

void WriteInt(int n, int x1, int y1) {
	std::stringstream out;
	out << n;
	WriteString(out.str().c_str(), x1, y1);
}

void WriteChar(char c, int x1, int y1) {
	RenderChar(c, x1, y1, &sys.default_font);
}

static void CreateFontBank(FontBank* fb, const char* fontName, int pointSize, SDL_Color color) {
	// here we actually create a texture containing all the characters we'll need and store
	// it with a bunch of info on the dimensions and location of each character in the font bank
	TTF_Font* font = TTF_OpenFont(fontName, pointSize);
	if (font == NULL) {
		err_log << "Couldn't open font " << fontName << std::endl;
		err_log.flush();
		exit(0);
	}
	int font_height = TTF_FontHeight(font);
	fb->height = font_height;
	char text[2];
	text[1] = 0;
	int running_x = 0;
	for (int i = 0; i < 256; i++) {
		if (IsPrintableChar(i) == false)
			continue;
		int w, h;
		text[0] = (char)i;
		int return_code = TTF_SizeText(font, text, &w, &h);
		if (return_code == -1)
			err_log << "Font: Got a bad return code on " << i << std::endl;
		// else err_log << "Font: OK return code..." << w << "," << h << std::endl;
		fb->src_rects[i].w = w;
		fb->src_rects[i].h = h;
		fb->src_rects[i].x = running_x;
		fb->src_rects[i].y = 0;
		running_x += w;
	}
	// Get the format:
	SDL_Surface* letter_a = TTF_RenderText_Blended(font, "a", color);
	// err_log << "Format:" << letter_a->format->format <<
	//	" BPP:" << (int)letter_a->format->BitsPerPixel << "rgb masks:" << letter_a->format->Rmask << " " << letter_a->format->Gmask << " "
	//	<< letter_a->format->Amask << " " << letter_a->format->Amask << std::endl;
	Uint32 format = letter_a->format->format;
	int bpp;
	Uint32 Rmask, Gmask, Bmask, Amask;
	SDL_PixelFormatEnumToMasks(format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
	SDL_FreeSurface(letter_a);
	SDL_Surface* temp_surface = SDL_CreateRGBSurface(0,
		running_x, font_height, bpp, Rmask, Gmask, Bmask, Amask);

	err_log << "Font w/h:" << running_x << " " << font_height << std::endl;

	Uint32 TransparentColor = SDL_MapRGBA(temp_surface->format, color.r, color.g, color.b, 0);
	SDL_FillRect(temp_surface, NULL, TransparentColor);
	for (int i = 0; i < 256; i++) {
		text[0] = (char)i;
		if (IsPrintableChar(i) == false)
			continue;
		SDL_Surface* one_letter = TTF_RenderText_Blended(font, text, color);
		SDL_BlitSurface(one_letter, NULL, temp_surface, &(fb->src_rects[i]));
		SDL_FreeSurface(one_letter);
	}
	fb->texture = SDL_CreateTextureFromSurface(sys.renderer, temp_surface);
	SDL_FreeSurface(temp_surface);
	TTF_CloseFont(font);
}

// begin NEW STUFF

int GetMouseX() {
	int x;
	SDL_GetMouseState(&x, NULL);
	return x;
}

int GetMouseY() {
	int y;
	SDL_GetMouseState(NULL, &y);
	return y;
}

// use LEFT_MOUSE_BUTTON, MIDDLE_MOUSE_BUTTON, RIGHT_MOUSE_BUTTON for button
bool IsMouseButtonDown(int button) {
	Uint32 code = SDL_GetMouseState(NULL, NULL);
	switch (button) {
	case LEFT_MOUSE_BUTTON:
		return code & SDL_BUTTON(1);
	case MIDDLE_MOUSE_BUTTON:
		return code & SDL_BUTTON(2);
	case RIGHT_MOUSE_BUTTON:
		return code & SDL_BUTTON(3);
	default:
		return false;
	}
}

bool WasMouseButtonPressed(int button) {
	if (button >= 0 && button < NUM_MOUSE_BUTTONS) {
		return sys.mouse_button_states[button] && !sys.mouse_button_previous_states[button];
	}
	return false;
}