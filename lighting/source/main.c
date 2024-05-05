#include "blahaj.h"
#include "vshader_shbin.h"
#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "blahaj_t3x.h"
#include <tex3ds.h>

#define CLEAR_COLOR 0x87CEEBFF // skyblue

#define DISPLAY_TRANSFER_FLAGS                                                \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |                    \
	 GX_TRANSFER_RAW_COPY(0) | GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
	 GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |                           \
	 GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static DVLB_s *vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection;
static C3D_Mtx projection;

static int uLoc_modelView;
static C3D_Mtx modelTrans;
static C3D_Mtx modelRot;

static C3D_LightEnv lightEnv;
static C3D_Light light;
static C3D_LightLut lut_Phong;

static C3D_Tex blahaj_tex;
static void *vbo_data;

static void sceneInit(void)
{
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection =
		shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView =
		shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo *attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=uv
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v2=normal

	// Compute the projection matrix
	Mtx_PerspTilt(&projection, C3D_AngleFromDegrees(60.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, false);
	Mtx_Identity(&modelTrans);
	Mtx_Translate(&modelTrans, 0.0, 0.0, -4.0, true);
	Mtx_Identity(&modelRot);

	// Create the VBO (vertex buffer object)
	vbo_data = linearAlloc(sizeof(vertex_list));
	memcpy(vbo_data, vertex_list, sizeof(vertex_list));

	// Configure buffers
	C3D_BufInfo *bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 3, 0x210);

	Tex3DS_Texture t3ds_tex = Tex3DS_TextureImport(&blahaj_t3x, blahaj_t3x_size, &blahaj_tex, NULL, false);
	Tex3DS_TextureFree(t3ds_tex);

	C3D_TexSetFilter(&blahaj_tex, GPU_LINEAR, GPU_NEAREST);
	C3D_TexBind(0, &blahaj_tex);

	C3D_TexEnv *env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_FRAGMENT_PRIMARY_COLOR, GPU_FRAGMENT_SECONDARY_COLOR, GPU_TEXTURE0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MULTIPLY_ADD);

	static const C3D_Material material =
		{
			{1.0f, 1.0f, 1.0f}, // ambient
			{0.9f, 0.9f, 0.9f}, // diffuse
			{0.3f, 0.3f, 0.3f}, // specular0
			{0.0f, 0.0f, 0.0f}, // specular1
			{0.0f, 0.0f, 0.0f}, // emission
		};

	C3D_LightEnvInit(&lightEnv);
	C3D_LightEnvBind(&lightEnv);
	C3D_LightEnvMaterial(&lightEnv, &material);

	LightLut_Phong(&lut_Phong, 5);
	C3D_LightEnvLut(&lightEnv, GPU_LUT_D0, GPU_LUTINPUT_LN, false, &lut_Phong);

	C3D_FVec lightVec = FVec4_New(0.0f, 0.0f, -0.5f, 1.0f);

	C3D_LightInit(&light, &lightEnv);
	C3D_LightColor(&light, 1.0, 1.0, 1.0);
	C3D_LightPosition(&light, &lightVec);
}

static void sceneRender(void)
{
	C3D_Mtx modelView;
	Mtx_Multiply(&modelView, &modelTrans, &modelRot);

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);

	// Draw the VBO
	C3D_DrawArrays(GPU_TRIANGLES, 0, vertex_count);
}

static void sceneExit(void)
{
	// Free the VBO
	linearFree(vbo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}

int main()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	consoleInit(GFX_BOTTOM, NULL);
	printf("\x1b[2;3HLow Poly Blahaj by IsabelleDotJpeg");
	printf("\x1b[3;3Havailable at https://skfb.ly/o8pRw");
	printf("\x1b[5;7HRotate with the Circle Pad");
	printf("\x1b[6;10HPress Start to Quit");

	C3D_RenderTarget *target =
		C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	sceneInit();
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;

		circlePosition cPos;
		hidCircleRead(&cPos);
		if (cPos.dx > 10 || cPos.dx < -10)
			Mtx_RotateY(&modelRot, cPos.dx / 1000.0f, false);
		if (cPos.dy > 10 || cPos.dy < -10)
			Mtx_RotateX(&modelRot, -cPos.dy / 1000.0f, false);

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		{
			C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(target);
			sceneRender();
		}
		C3D_FrameEnd(0);
	}
	sceneExit();

	C3D_Fini();
	gfxExit();
	return 0;
}
