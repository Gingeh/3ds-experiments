#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>

#define MAX_SPRITES 256
#define GRAVITY 0.2

#define TOP_WIDTH 400
#define TOP_HEIGHT 240
#define BOT_WIDTH 320
#define BOT_HEIGHT 240

typedef struct
{
	C2D_Sprite spr;
	float dx, dy;
} Sprite;

static Sprite sprites[MAX_SPRITES];
static size_t numSprites = 0;
static C2D_SpriteSheet spriteSheet;

static C3D_Mtx bottomView;

static C2D_TextBuf staticTextBuf;
static C2D_TextBuf dynTextBuf;
static C2D_Text text[2];

static void spawnSprite()
{
	if (numSprites == MAX_SPRITES)
		return;

	Sprite *sprite = &sprites[numSprites];
	if (rand() % 20 == 0)
	{
		C2D_SpriteFromSheet(&sprite->spr, spriteSheet, 4);
		C2D_SpriteScale(&sprite->spr, 0.75f, 0.75f);
	}
	else
	{
		C2D_SpriteFromSheet(&sprite->spr, spriteSheet, rand() % 4);
	}
	C2D_SpriteSetCenter(&sprite->spr, 0.5f, 0.5f);
	C2D_SpriteSetPos(&sprite->spr, TOP_WIDTH / 2.0f, TOP_HEIGHT / 2.0f);
	sprite->dx = rand() * 10.0f / RAND_MAX - 5.0f;
	sprite->dy = rand() * 10.0f / RAND_MAX - 5.0f;

	numSprites++;
}

static void killSprite()
{
	if (numSprites == 0)
		return;

	numSprites--;
}

static void updateSprites()
{
	for (size_t i = 0; i < numSprites; i++)
	{
		Sprite *sprite = &sprites[i];

		sprite->dy += GRAVITY;
		C2D_SpriteMove(&sprite->spr, sprite->dx, sprite->dy);
		C2D_SpriteRotate(&sprite->spr, 0.1);

		float *x = &sprite->spr.params.pos.x;
		float *y = &sprite->spr.params.pos.y;
		float r = 8.0f;

		if (*y < TOP_HEIGHT)
		{ // Top Screen
			if (*x < r || *x > TOP_WIDTH - r)
			{
				sprite->dx *= -1;
				*x = C2D_Clamp(*x, r, TOP_WIDTH - r);
			}

			if (*y > TOP_HEIGHT - r && (*x < 40 || *x > BOT_WIDTH + 40))
			{
				sprite->dy = -abs(sprite->dy) - GRAVITY / 2.0;
				*y = TOP_HEIGHT - r;
			}
		}
		else
		{ // Bottom Screen
			if (*x < 40 + r || *x > BOT_WIDTH + 40 - r)
			{
				sprite->dx *= -1;
				*x = C2D_Clamp(*x, 40.0f + r, BOT_WIDTH + 40.0f - r);
			}

			if (*y > TOP_HEIGHT + BOT_HEIGHT - r)
			{
				sprite->dy *= -1;
				sprite->dy -= GRAVITY / 2.0;
				*y = TOP_HEIGHT + BOT_HEIGHT - r;
			}
		}
	}
}

int main()
{
	romfsInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

	C2D_ViewTranslate((BOT_WIDTH - TOP_WIDTH) / 2.0f, -TOP_HEIGHT);
	C2D_ViewSave(&bottomView);

	spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");

	staticTextBuf = C2D_TextBufNew(256);
	dynTextBuf = C2D_TextBufNew(256);
	C2D_TextParse(&text[0], staticTextBuf, " ball\n un-ball\n quit");
	C2D_TextOptimize(&text[0]);

	srand(time(NULL));

	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_B)
			break;

		if (kDown & KEY_DUP)
			spawnSprite();

		if (kDown & KEY_DDOWN)
			killSprite();

		C2D_TextBufClear(dynTextBuf);
		char buf[128];
		snprintf(buf, sizeof(buf), "balls: %d", numSprites);
		C2D_TextParse(&text[1], dynTextBuf, buf);
		C2D_TextOptimize(&text[1]);

		updateSprites();

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

		C2D_ViewReset();
		C2D_TargetClear(top, C2D_Color32f(0.5f, 0.8f, 0.9f, 1.0f));
		C2D_SceneBegin(top);
		C2D_DrawText(&text[0], 0, TOP_WIDTH - 20.0 - text[0].width * 0.9, 15.0, 0.0, 0.9, 0.9);
		C2D_DrawText(&text[1], 0, 20.0, 15.0, 0.0, 1.0, 1.0);
		for (size_t i = 0; i < numSprites; i++)
			C2D_DrawSprite(&sprites[i].spr);

		C2D_ViewRestore(&bottomView);
		C2D_TargetClear(bottom, C2D_Color32f(0.5f, 0.8f, 0.9f, 1.0f));
		C2D_SceneBegin(bottom);
		for (size_t i = 0; i < numSprites; i++)
		{
			C2D_DrawSprite(&sprites[i].spr);
		}

		C3D_FrameEnd(0);
	}

	C2D_TextBufDelete(staticTextBuf);
	C2D_TextBufDelete(dynTextBuf);
	C2D_SpriteSheetFree(spriteSheet);
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	romfsExit();
	return 0;
}
