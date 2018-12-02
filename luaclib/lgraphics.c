#include "lgraphics.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "lfont.h"
#include "game.h"

extern Game *G;

static int
ldraw_sprite(lua_State *L) {
    GLuint vao, texture, camera;
    vao = luaL_checkinteger(L, 1);
    camera = lua_toboolean(L, 2);
    texture = luaL_optinteger(L, 3, 0);
    G->opengl->use_sp_program(camera);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    return 0;
}


static void
ROTATE(float x0, float y0, float a, float x1, float y1, float *x, float *y) {
	*x = (x1 - x0)*cos(a) - (y1 - y0)*sin(a) + x0;
	*y = (x1 - x0)*sin(a) + (y1 - y0)*cos(a) + y0;
}

static int
ldraw_text(lua_State *L) {
	uint32_t n, camera;
	float x0, y0, x, y, w, h, scale, xpos, ypos, angle;
	Character *ch;

	x0 = luaL_checknumber(L, 1);
	y0 = luaL_checknumber(L, 2);
	x = luaL_checknumber(L, 3);
	y = luaL_checknumber(L, 4);
	scale = luaL_checknumber(L, 5);
	n = luaL_len(L, 6);
	angle = luaL_checknumber(L, 7) * (M_PI/180.f);
	camera = lua_toboolean(L, 8);
	
	static float vertices[4][4] = {
		{0.f, 0.f,	0.f, 0.f},
		{0.f, 0.f,	0.f, 1.f},
		{0.f, 0.f,	1.f, 1.f},
		{0.f, 0.f,	1.f, 0.f},
	};

	G->opengl->use_tx_program(camera);


	for (int i = 1; i <= n; ++i) {
		lua_rawgeti(L, 6, i);
		ch = lua_touserdata(L, -1);

		xpos = x + ch->offsetx * scale;
		ypos = y - (ch->height - ch->offsety) * scale;

		w = ch->width * scale;
		h = ch->height * scale;

		// 左上 -> 左下 -> 右下 -> 右上 (逆时针)
		ROTATE(x0, y0, angle, xpos, ypos+h, &vertices[0][0], &vertices[0][1]);
		ROTATE(x0, y0, angle, xpos, ypos,   &vertices[1][0], &vertices[1][1]);
		ROTATE(x0, y0, angle, xpos+w, ypos, &vertices[2][0], &vertices[2][1]);
		ROTATE(x0, y0, angle, xpos+w, ypos+h, &vertices[3][0], &vertices[3][1]);

		glBindVertexArray(G->opengl->ft_vao);
		glBindTexture(GL_TEXTURE_2D, ch->texture);
		glBindBuffer(GL_ARRAY_BUFFER, G->opengl->ft_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		lua_pop(L, 1);
		x = x + (ch->advancex >> 6) * scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	return 0;
}


static int
lsprite(lua_State *L) {
	GLuint vao, vbo, ebo;

	static float vertices[4][4];

    static GLuint indices[] = {
        0,1,2,
        0,2,3
    };
	// 4piont position [x,y]
	// 左上 -> 左下 -> 右下 -> 右上 (逆时针)
	vertices[0][0] = luaL_checknumber(L, 1);
	vertices[0][1] = luaL_checknumber(L, 2);
	vertices[1][0] = luaL_checknumber(L, 3);
	vertices[1][1] = luaL_checknumber(L, 4);
	vertices[2][0] = luaL_checknumber(L, 5);
	vertices[2][1] = luaL_checknumber(L, 6);
	vertices[3][0] = luaL_checknumber(L, 7);
	vertices[3][1] = luaL_checknumber(L, 8);

	// texcoord
	vertices[0][2] = luaL_optnumber(L,  9, 0.f);
	vertices[0][3] = luaL_optnumber(L, 10, 1.f);
	vertices[1][2] = luaL_optnumber(L, 11, 0.f);
	vertices[1][3] = luaL_optnumber(L, 12, 0.f);
	vertices[2][2] = luaL_optnumber(L, 13, 1.f);
	vertices[2][3] = luaL_optnumber(L, 14, 0.f);
	vertices[3][2] = luaL_optnumber(L, 15, 1.f);
	vertices[3][3] = luaL_optnumber(L, 16, 1.f);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	const static uint32_t step = 4*sizeof(float);
	// pos + texcoord
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, step, (void*)(0));
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	lua_pushinteger(L, vao);
	lua_pushinteger(L, vbo);
	return 2;
}


static int
lupdate_sprite_aabb(lua_State *L) {
	GLuint vbo = luaL_checkinteger(L, 1);
	static float p[8];
	p[0] = luaL_checknumber(L, 2);
	p[1] = luaL_checknumber(L, 3);
	p[2] = luaL_checknumber(L, 4);
	p[3] = luaL_checknumber(L, 5);
	p[4] = luaL_checknumber(L, 6);
	p[5] = luaL_checknumber(L, 7);
	p[6] = luaL_checknumber(L, 8);
	p[7] = luaL_checknumber(L, 9);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	for (int i = 0; i < 4; ++i) {
		glBufferSubData(GL_ARRAY_BUFFER, i*4*sizeof(float), 2*sizeof(float), p + 2*i);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return 0;
}

static int
lupdate_sprite_texcoord(lua_State *L) {
	GLuint vbo = luaL_checkinteger(L, 1);
	static float coord[8];
	coord[0] = luaL_checknumber(L, 2);
	coord[1] = luaL_checknumber(L, 3);
	coord[2] = luaL_checknumber(L, 4);
	coord[3] = luaL_checknumber(L, 5);
	coord[4] = luaL_checknumber(L, 6);
	coord[5] = luaL_checknumber(L, 7);
	coord[6] = luaL_checknumber(L, 8);
	coord[7] = luaL_checknumber(L, 9);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	for (int i = 0; i < 4; ++i) {
		glBufferSubData(GL_ARRAY_BUFFER, (i*4+2)*sizeof(float), 2*sizeof(float), coord + 2*i);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return 0;
}


static int
ltexture(lua_State *L)
{
	GLuint texture;
	const char *filename;
	int width, height, channel;
	unsigned char *data;

	filename = luaL_checkstring(L, 1);
	data = stbi_load(filename, &width, &height, &channel, STBI_rgb_alpha);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
	lua_pushinteger(L, texture);
	return 1;
}



static int
lset_sp_color(lua_State *L) {
	uint32_t c;
	c = luaL_checkinteger(L, 1);
	G->opengl->set_sp_color(c);
	return 0;
}


static int
lset_tx_color(lua_State *L) {
	uint32_t c;
	c = luaL_checkinteger(L, 1);
	G->opengl->set_tx_color(c);
	return 0;
}


static int
lupdate_camera(lua_State *L) {
	float x, y, scale;

	x = luaL_checknumber(L, 1);
	y = luaL_checknumber(L, 2);
	scale = luaL_optnumber(L, 3, 1.f);
	G->opengl->update_camera(x, y, scale);
	return 0;
}


// 开始模板绘制 -> draw stencils(sprites) -> 结束模板绘制 -> draw sprites -> 清空模板
static int
lstart_stencil(lua_State *L) {
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0XFF);
	glStencilMask(0XFF);
	return 0;
}


static int
lstop_stencil(lua_State *L) {
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0x00);
	return 0;
}


static int
lclear_stencil(lua_State *L) {
	glClear(GL_STENCIL_BUFFER_BIT);
	glDisable(GL_STENCIL_TEST);
	return 0;
}


static int
lclear(lua_State *L) {
	uint32_t c;
	c = luaL_checkinteger(L, 1);
	glClearColor(R(c), G(c), B(c), A(c));
	return 0;
}


int
lib_graphics(lua_State *L)
{
    stbi_set_flip_vertically_on_load(true);
	luaL_Reg l[] = {
		{"clear_stencil", lclear_stencil},
		{"stop_stencil", lstop_stencil},
		{"start_stencil", lstart_stencil},
		{"draw_text", ldraw_text},
        {"draw_sprite", ldraw_sprite},
        {"update_sprite_texcoord", lupdate_sprite_texcoord},
        {"update_sprite_aabb", lupdate_sprite_aabb},
        {"update_camera", lupdate_camera},
        {"set_tx_color", lset_tx_color},
        {"set_sp_color", lset_sp_color},
        {"sprite", lsprite},
        {"texture", ltexture},
        {"clear", lclear},
		{NULL, NULL}
	};
	luaL_newlib(L, l);
	return 1;
}