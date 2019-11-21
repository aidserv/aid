
#pragma once
#include "gles1_server_context.h"
#include "gles2_server_context.h"
#include "renderControl_server_context.h"
#include "RenderLibImpl.h"

struct st_opengles_info {
	gles1_server_context_t m_gles1;
	gles2_server_context_t m_gles2;
	renderControl_server_context_t m_renderControl;
	emugl::RenderLib* m_render;
};
