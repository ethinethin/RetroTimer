#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

struct screen {
	unsigned int w;
	unsigned int h;
	const char *name;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_bool running;
	SDL_bool timer;
	SDL_Texture **numbers;
	SDL_Texture *scanlines;
	int time_current[3];
	int time_saved[3];
	int time_drawn[3];
	int cur_ticks;
	int first_tick;
};

/* Function prototypes */
static void	display_init(struct screen *cur_screen);
static void	load_sprites(struct screen *cur_screen);
static void	draw_scanlines(struct screen *cur_screen);
static void	display_quit(struct screen *cur_screen);
static void	draw_screen(struct screen *cur_screen);
static void	draw_time(struct screen *cur_screen);
static void	print_num(struct screen *cur_screen, int x, int y, int num);
static void	calc_time(struct screen *cur_screen);
static void	toggle_timer(struct screen *cur_screen);

int
main(int argc, char *argv[])
{
	SDL_Event event;
	struct screen cur_screen = { 660, 160, "RetroTimer", NULL, NULL, SDL_FALSE, SDL_FALSE, NULL, NULL, {0, 0, 0}, {0, 0, 0}, {-1, -1, -1}, 0, 0 };
	/* Initialize display */
	display_init(&cur_screen);
	/* If 3 command line arguments are provided, convert them to numbers for the starting time */
	if (argc == 4) {
		cur_screen.time_current[0] = cur_screen.time_saved[0] = atoi(argv[1]);
		cur_screen.time_current[1] = cur_screen.time_saved[1] = atoi(argv[2]);
		cur_screen.time_current[2] = cur_screen.time_saved[2] = atoi(argv[3]);
	}
	/* Draw the initial time */
	draw_screen(&cur_screen);
	
	/* Enter main input loop */
	while (cur_screen.running == SDL_TRUE) {
		/* If timer is turned on, update ticks and draw time */
		if (cur_screen.timer == SDL_TRUE) {
			cur_screen.cur_ticks = SDL_GetTicks();
			calc_time(&cur_screen);
			draw_screen(&cur_screen);
		}
		SDL_Delay(10);
		/* If necessary, check input and handle it */
		if (SDL_PollEvent(&event) == 0) continue;
		if (event.type == SDL_QUIT) {
			cur_screen.running = SDL_FALSE;
		} else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_SPACE) { /* toggle timer on/off if space is pressed */
				toggle_timer(&cur_screen);
			} else if (cur_screen.timer == SDL_FALSE) { /* if timer is off, let user change it */
				switch(event.key.keysym.sym) {
					case SDLK_q: /* subtract hours */
						cur_screen.time_current[0] -= 1;
						if (cur_screen.time_current[0] < 0) cur_screen.time_current[0] = 99;
						break;
					case SDLK_w: /* add hours */
						cur_screen.time_current[0] += 1;
						if (cur_screen.time_current[0] > 99) cur_screen.time_current[0] = 0;
						break;
					case SDLK_a: /* subtract minutes */
						cur_screen.time_current[1] -= 1;
						if (cur_screen.time_current[1] < 0) cur_screen.time_current[1] = 59;
						break;
					case SDLK_s: /* add minutes */
						cur_screen.time_current[1] += 1;
						if (cur_screen.time_current[1] > 59) cur_screen.time_current[1] = 0;
						break;
					case SDLK_z: /* subtract seconds */
						cur_screen.time_current[2] -= 1;
						if (cur_screen.time_current[2] < 0) cur_screen.time_current[2] = 59;
						break;
					case SDLK_x: /* add seconds */
						cur_screen.time_current[2] += 1;
						if (cur_screen.time_current[2] > 59) cur_screen.time_current[2] = 0;
						break;
				}
				cur_screen.time_saved[0] = cur_screen.time_current[0];
				cur_screen.time_saved[1] = cur_screen.time_current[1];
				cur_screen.time_saved[2] = cur_screen.time_current[2];
				draw_screen(&cur_screen);
			}
		}
	}
	/* Loop over, time to quit */
	display_quit(&cur_screen);
	return 0;
}

static void
display_init(struct screen *cur_screen)
{
	/* Initialize SDL */
	SDL_Init(SDL_INIT_EVERYTHING);
	/* Create the main window and renderer */
	cur_screen->window = SDL_CreateWindow(cur_screen->name, 1600, 1000, cur_screen->w, cur_screen->h, 0);
	cur_screen->renderer = SDL_CreateRenderer(cur_screen->window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE);
	/* Load number sprites */
	load_sprites(cur_screen);
	/* Set up scan lines */
	cur_screen->scanlines = SDL_CreateTexture(cur_screen->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cur_screen->w, cur_screen->h);
	draw_scanlines(cur_screen);
	/* Up and running */
	cur_screen->running = SDL_TRUE;
}

static void
load_sprites(struct screen *cur_screen)
{
	int i;
	SDL_Surface *num_bmp;
	SDL_Surface *num_surf;
	SDL_Rect rect;
	
	/* Load bitmap and allocate memory */
	num_bmp = SDL_LoadBMP("numbers.bmp");
	cur_screen->numbers = malloc(sizeof(*cur_screen->numbers)*22);
	
	/* Iterate through sprite sheet */
	rect.x = 0; rect.y = 0; rect.w = 6; rect.h = 12;
	for (i = 0; i < 22; i++) {
		num_surf = SDL_CreateRGBSurface(0, 6, 12, 24, 0x00, 0x00, 0x00, 0x00);
		SDL_SetColorKey(num_surf, 1, 0xFF00FF);
		//SDL_FillRect(num, 1, 0xFF00FF);
		rect.x = i * 6;
		SDL_BlitSurface(num_bmp, &rect, num_surf, NULL);
		cur_screen->numbers[i] = SDL_CreateTextureFromSurface(cur_screen->renderer, num_surf);
	}
	SDL_FreeSurface(num_bmp);
	SDL_FreeSurface(num_surf);
}

static void
draw_scanlines(struct screen *cur_screen)
{
	int i;
	SDL_SetRenderTarget(cur_screen->renderer, cur_screen->scanlines);
	SDL_SetTextureBlendMode(cur_screen->scanlines, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(cur_screen->renderer, 0, 0, 0, 192);
	for (i = 0; i <= cur_screen->h; i += 3) {
		SDL_RenderDrawLine(cur_screen->renderer, 0, i, cur_screen->w, i);
	}
	SDL_SetRenderTarget(cur_screen->renderer, NULL);
}

static void
display_quit(struct screen *cur_screen)
{
	int i;
	
	SDL_DestroyRenderer(cur_screen->renderer);
	cur_screen->renderer = NULL;
	SDL_DestroyWindow(cur_screen->window);
	cur_screen->window = NULL;
	SDL_DestroyTexture(cur_screen->scanlines);
	for (i = 0; i < 22; i++) {
		SDL_DestroyTexture(cur_screen->numbers[i]);
	}
	SDL_Quit();
}

static void
draw_screen(struct screen *cur_screen)
{
	/* If the same time is already displayed, don't update */
	if (cur_screen->time_current[0] == cur_screen->time_drawn[0] &&
	    cur_screen->time_current[1] == cur_screen->time_drawn[1] &&
	    cur_screen->time_current[2] == cur_screen->time_drawn[2]) {
		return;
	}

	/* Clear screen to grey if the timer is paused, magenta if the timer is on
	 * zero seconds, or purple otherwise */
	if (cur_screen->timer == SDL_FALSE) {
		SDL_SetRenderDrawColor(cur_screen->renderer, 96, 96, 96, 255);
	} else if (cur_screen->time_current[2] == 0) {
		SDL_SetRenderDrawColor(cur_screen->renderer, 192, 0, 255, 255);
	} else {
		SDL_SetRenderDrawColor(cur_screen->renderer, 96, 0, 128, 255);
	}
	SDL_RenderClear(cur_screen->renderer);
	/* Draw time and scanlines, then output to display */
	draw_time(cur_screen);
	SDL_Rect rect = { 0, 0, cur_screen->w, cur_screen->h };
	SDL_RenderCopy(cur_screen->renderer, cur_screen->scanlines, NULL, &rect);
	SDL_RenderPresent(cur_screen->renderer);
	
	/* Update "last time" */
	cur_screen->time_drawn[0] = cur_screen->time_current[0];
	cur_screen->time_drawn[1] = cur_screen->time_current[1];
	cur_screen->time_drawn[2] = cur_screen->time_current[2];
}

static void
draw_time(struct screen *cur_screen)
{
	int i;
	int color;
	char hhmmss[9];
	
	/* If the timer is paused, use the grey sprites */
	if (cur_screen->timer == SDL_FALSE) {
		color = 11;
	} else {
		color = 0;
	}
	
	/* Copy time to string - include colons if timer is paused or seconds are even */
	if (cur_screen->time_current[2] % 2 == 0 || cur_screen->timer == SDL_FALSE) {
		sprintf(hhmmss, "%02d:%02d:%02d", cur_screen->time_current[0], cur_screen->time_current[1], cur_screen->time_current[2]);
	} else {
		sprintf(hhmmss, "%02d %02d %02d", cur_screen->time_current[0], cur_screen->time_current[1], cur_screen->time_current[2]);
	}
	/* Draw each character */
	for (i = 0; i < 8; i++) {
		if (hhmmss[i] == ' ') continue;
		print_num(cur_screen, i * 60 + 20 * i + 20, 20, hhmmss[i] - 48 + color);
	}
}

static void
print_num(struct screen *cur_screen, int x, int y, int num)
{
	int i, j;
	SDL_Rect coords = { x, y, 60, 120 };
	
	/* Draw sprite for the number */
	SDL_RenderCopy(cur_screen->renderer, cur_screen->numbers[num], NULL, &coords);
}

static void
calc_time(struct screen *cur_screen)
{
	int hour;
	int min;
	int sec;
	/* Calculate new time with ticks */
	sec = (cur_screen->cur_ticks - cur_screen->first_tick)/1000;
	/* Add saved time */
	sec += cur_screen->time_saved[2] + cur_screen->time_saved[1]*60 + cur_screen->time_saved[0]*60*60;
	if (sec >= 60) {
		min = (int) sec/60;
		sec -= min*60;
		if (min >= 60) {
			hour = (int) min/60;
			min -= hour*60;
			if (hour > 99) hour = 0;
		} else {
			hour = 0;
		}
	} else {
		min = 0;
		hour = 0;
	}
	/* Put the values in there */
	cur_screen->time_current[0] = hour;
	cur_screen->time_current[1] = min;
	cur_screen->time_current[2] = sec;
}

static void
toggle_timer(struct screen *cur_screen)
{
	cur_screen->timer = !cur_screen->timer;
	/* If you're pausing, update the old time and output */
	if (cur_screen->timer == SDL_FALSE) {
		cur_screen->time_saved[0] = cur_screen->time_current[0];
		cur_screen->time_saved[1] = cur_screen->time_current[1];
		cur_screen->time_saved[2] = cur_screen->time_current[2];
	} else {
		/* Unpausing, get a new first tick */
		cur_screen->first_tick = SDL_GetTicks();
	}
	cur_screen->time_drawn[0] = cur_screen->time_drawn[1] = cur_screen->time_drawn[2] = -1;
	draw_screen(cur_screen);
}
