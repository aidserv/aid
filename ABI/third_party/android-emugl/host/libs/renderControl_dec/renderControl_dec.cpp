// Generated Code - DO NOT EDIT !!
// generated by 'emugen'


#include <string.h>
#include "renderControl_opcodes.h"

#include "renderControl_dec.h"


#include "ProtocolUtils.h"

#include "ChecksumCalculatorThreadInfo.h"

#include <stdio.h>

//#define OPENGL_DEBUG_PRINTOUT
typedef unsigned int tsize_t; // Target "size_t", which is 32-bit for now. It may or may not be the same as host's size_t when emugen is compiled.

#ifdef OPENGL_DEBUG_PRINTOUT
//#  define DEBUG(...) do { if (emugl_cxt_logger) { emugl_cxt_logger(__VA_ARGS__); } } while(0)
#define DEBUG(...) printf(__VA_ARGS__);
#else
#  define DEBUG(...)  ((void)0)
#endif

#ifdef CHECK_GL_ERRORS
#  define SET_LASTCALL(name)  sprintf(lastCall, #name)
#else
#  define SET_LASTCALL(name)
#endif
using namespace emugl;

#if 1
static unsigned int GETDWORD(unsigned char *ptr)
{
	return *(unsigned char *)(ptr+0)
		|(*(unsigned char *)(ptr+1)<<8)
		|(*(unsigned char *)(ptr+2)<<16)
		|(*(unsigned char *)(ptr+3)<<24);
}
#endif

size_t renderControl_decoder_context_t::decode(void *buf, size_t len, IOStream *stream, ChecksumCalculator* checksumCalc) {
	if (len < 8) return 0; 
#ifdef CHECK_GL_ERRORS
	char lastCall[256] = {0};
#endif
	unsigned char *ptr = (unsigned char *)buf;
	const unsigned char* const end = (const unsigned char*)buf + len;
	while (end - ptr >= 8) {
		//uint32_t opcode = *(uint32_t *)ptr;
		//int32_t packetLen = *(int32_t *)(ptr + 4);
		#if 1
		uint32_t opcode = Unpack<uint32_t, int32_t>(ptr);
		int32_t packetLen = Unpack<int32_t, int32_t>(ptr+4);
		#else
		uint32_t opcode = 	GETDWORD(ptr);
		int32_t packetLen = GETDWORD(ptr+4);
		#endif
		if (end - ptr < packetLen) return ptr - (unsigned char*)buf;
        // Do this on every iteration, as some commands may change the checksum
        // calculation parameters.
        const size_t checksumSize = checksumCalc->checksumByteSize();
        const bool useChecksum = checksumSize > 0;
		switch(opcode) {
		case OP_rcGetRendererVersion: {
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8, ptr + 8, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcGetRendererVersion: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(GLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcGetRendererVersion()\n", stream);
			*(GLint *)(&tmpBuf[0]) = 			this->rcGetRendererVersion();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcGetRendererVersion");
			break;
		}
		case OP_rcGetEGLVersion: {
			uint32_t size_major __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t size_minor __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4, ptr + 8 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcGetEGLVersion: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_major;
			totalTmpSize += size_minor;
			totalTmpSize += sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_major(&tmpBuf[0], size_major);
			OutputBuffer outptr_minor(&tmpBuf[0 + size_major], size_minor);
			DEBUG("renderControl(%p): rcGetEGLVersion(%p(%u) %p(%u) )\n", stream, (EGLint*)(outptr_major.get()), size_major, (EGLint*)(outptr_minor.get()), size_minor);
			*(EGLint *)(&tmpBuf[0 + size_major + size_minor]) = 			this->rcGetEGLVersion((EGLint*)(outptr_major.get()), (EGLint*)(outptr_minor.get()));
			outptr_major.flush();
			outptr_minor.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcGetEGLVersion");
			break;
		}
		case OP_rcQueryEGLString: {
			EGLenum var_name = Unpack<EGLenum,uint32_t>(ptr + 8);
			uint32_t size_buffer __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			EGLint var_bufferSize = Unpack<EGLint,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcQueryEGLString: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_buffer;
			totalTmpSize += sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_buffer(&tmpBuf[0], size_buffer);
			DEBUG("renderControl(%p): rcQueryEGLString(0x%08x %p(%u) 0x%08x )\n", stream, var_name, (void*)(outptr_buffer.get()), size_buffer, var_bufferSize);
			*(EGLint *)(&tmpBuf[0 + size_buffer]) = 			this->rcQueryEGLString(var_name, (void*)(outptr_buffer.get()), var_bufferSize);
			outptr_buffer.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcQueryEGLString");
			break;
		}
		case OP_rcGetGLString: {
			EGLenum var_name = Unpack<EGLenum,uint32_t>(ptr + 8);
			uint32_t size_buffer __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			EGLint var_bufferSize = Unpack<EGLint,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcGetGLString: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_buffer;
			totalTmpSize += sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_buffer(&tmpBuf[0], size_buffer);
			DEBUG("renderControl(%p): rcGetGLString(0x%08x %p(%u) 0x%08x )\n", stream, var_name, (void*)(outptr_buffer.get()), size_buffer, var_bufferSize);
			*(EGLint *)(&tmpBuf[0 + size_buffer]) = 			this->rcGetGLString(var_name, (void*)(outptr_buffer.get()), var_bufferSize);
			outptr_buffer.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcGetGLString");
			break;
		}
		case OP_rcGetNumConfigs: {
			uint32_t size_numAttribs __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcGetNumConfigs: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_numAttribs;
			totalTmpSize += sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_numAttribs(&tmpBuf[0], size_numAttribs);
			DEBUG("renderControl(%p): rcGetNumConfigs(%p(%u) )\n", stream, (uint32_t*)(outptr_numAttribs.get()), size_numAttribs);
			*(EGLint *)(&tmpBuf[0 + size_numAttribs]) = 			this->rcGetNumConfigs((uint32_t*)(outptr_numAttribs.get()));
			outptr_numAttribs.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcGetNumConfigs");
			break;
		}
		case OP_rcGetConfigs: {
			uint32_t var_bufSize = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t size_buffer __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4, ptr + 8 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcGetConfigs: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_buffer;
			totalTmpSize += sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_buffer(&tmpBuf[0], size_buffer);
			DEBUG("renderControl(%p): rcGetConfigs(0x%08x %p(%u) )\n", stream, var_bufSize, (GLuint*)(outptr_buffer.get()), size_buffer);
			*(EGLint *)(&tmpBuf[0 + size_buffer]) = 			this->rcGetConfigs(var_bufSize, (GLuint*)(outptr_buffer.get()));
			outptr_buffer.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcGetConfigs");
			break;
		}
		case OP_rcChooseConfig: {
			uint32_t size_attribs __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8);
			InputBuffer inptr_attribs(ptr + 8 + 4, size_attribs);
			uint32_t var_attribs_size = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + size_attribs);
			uint32_t size_configs __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + size_attribs + 4);
			uint32_t var_configs_size = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + size_attribs + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + size_attribs + 4 + 4 + 4, ptr + 8 + 4 + size_attribs + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcChooseConfig: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_configs;
			totalTmpSize += sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_configs(&tmpBuf[0], size_configs);
			DEBUG("renderControl(%p): rcChooseConfig(%p(%u) 0x%08x %p(%u) 0x%08x )\n", stream, (EGLint*)(inptr_attribs.get()), size_attribs, var_attribs_size, (uint32_t*)(outptr_configs.get()), size_configs, var_configs_size);
			*(EGLint *)(&tmpBuf[0 + size_configs]) = 			this->rcChooseConfig((EGLint*)(inptr_attribs.get()), var_attribs_size, size_configs == 0 ? nullptr : (uint32_t*)(outptr_configs.get()), var_configs_size);
			outptr_configs.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcChooseConfig");
			break;
		}
		case OP_rcGetFBParam: {
			EGLint var_param = Unpack<EGLint,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcGetFBParam: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcGetFBParam(0x%08x )\n", stream, var_param);
			*(EGLint *)(&tmpBuf[0]) = 			this->rcGetFBParam(var_param);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcGetFBParam");
			break;
		}
		case OP_rcCreateContext: {
			uint32_t var_config = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_share = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			uint32_t var_glVersion = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCreateContext: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(uint32_t);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcCreateContext(0x%08x 0x%08x 0x%08x )\n", stream, var_config, var_share, var_glVersion);
			*(uint32_t *)(&tmpBuf[0]) = 			this->rcCreateContext(var_config, var_share, var_glVersion);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcCreateContext");
			break;
		}
		case OP_rcDestroyContext: {
			uint32_t var_context = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcDestroyContext: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcDestroyContext(0x%08x )\n", stream, var_context);
			this->rcDestroyContext(var_context);
			SET_LASTCALL("rcDestroyContext");
			break;
		}
		case OP_rcCreateWindowSurface: {
			uint32_t var_config = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_width = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			uint32_t var_height = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCreateWindowSurface: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(uint32_t);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcCreateWindowSurface(0x%08x 0x%08x 0x%08x )\n", stream, var_config, var_width, var_height);
			*(uint32_t *)(&tmpBuf[0]) = 			this->rcCreateWindowSurface(var_config, var_width, var_height);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcCreateWindowSurface");
			break;
		}
		case OP_rcDestroyWindowSurface: {
			uint32_t var_windowSurface = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcDestroyWindowSurface: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcDestroyWindowSurface(0x%08x )\n", stream, var_windowSurface);
			this->rcDestroyWindowSurface(var_windowSurface);
			SET_LASTCALL("rcDestroyWindowSurface");
			break;
		}
		case OP_rcCreateColorBuffer: {
			uint32_t var_width = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_height = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			GLenum var_internalFormat = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCreateColorBuffer: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(uint32_t);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcCreateColorBuffer(0x%08x 0x%08x 0x%08x )\n", stream, var_width, var_height, var_internalFormat);
			*(uint32_t *)(&tmpBuf[0]) = 			this->rcCreateColorBuffer(var_width, var_height, var_internalFormat);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcCreateColorBuffer");
			break;
		}
		case OP_rcOpenColorBuffer: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcOpenColorBuffer: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcOpenColorBuffer(0x%08x )\n", stream, var_colorbuffer);
			this->rcOpenColorBuffer(var_colorbuffer);
			SET_LASTCALL("rcOpenColorBuffer");
			break;
		}
		case OP_rcCloseColorBuffer: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCloseColorBuffer: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcCloseColorBuffer(0x%08x )\n", stream, var_colorbuffer);
			this->rcCloseColorBuffer(var_colorbuffer);
			SET_LASTCALL("rcCloseColorBuffer");
			break;
		}
		case OP_rcSetWindowColorBuffer: {
			uint32_t var_windowSurface = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_colorBuffer = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4, ptr + 8 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcSetWindowColorBuffer: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcSetWindowColorBuffer(0x%08x 0x%08x )\n", stream, var_windowSurface, var_colorBuffer);
			this->rcSetWindowColorBuffer(var_windowSurface, var_colorBuffer);
			SET_LASTCALL("rcSetWindowColorBuffer");
			break;
		}
		case OP_rcFlushWindowColorBuffer: {
			uint32_t var_windowSurface = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcFlushWindowColorBuffer: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(int);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcFlushWindowColorBuffer(0x%08x )\n", stream, var_windowSurface);
			*(int *)(&tmpBuf[0]) = 			this->rcFlushWindowColorBuffer(var_windowSurface);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcFlushWindowColorBuffer");
			break;
		}
		case OP_rcMakeCurrent: {
			uint32_t var_context = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_drawSurf = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			uint32_t var_readSurf = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcMakeCurrent: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcMakeCurrent(0x%08x 0x%08x 0x%08x )\n", stream, var_context, var_drawSurf, var_readSurf);
			*(EGLint *)(&tmpBuf[0]) = 			this->rcMakeCurrent(var_context, var_drawSurf, var_readSurf);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcMakeCurrent");
			break;
		}
		case OP_rcFBPost: {
			uint32_t var_colorBuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcFBPost: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcFBPost(0x%08x )\n", stream, var_colorBuffer);
			this->rcFBPost(var_colorBuffer);
			SET_LASTCALL("rcFBPost");
			break;
		}
		case OP_rcFBSetSwapInterval: {
			EGLint var_interval = Unpack<EGLint,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcFBSetSwapInterval: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcFBSetSwapInterval(0x%08x )\n", stream, var_interval);
			this->rcFBSetSwapInterval(var_interval);
			SET_LASTCALL("rcFBSetSwapInterval");
			break;
		}
		case OP_rcBindTexture: {
			uint32_t var_colorBuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcBindTexture: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcBindTexture(0x%08x )\n", stream, var_colorBuffer);
			this->rcBindTexture(var_colorBuffer);
			SET_LASTCALL("rcBindTexture");
			break;
		}
		case OP_rcBindRenderbuffer: {
			uint32_t var_colorBuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcBindRenderbuffer: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcBindRenderbuffer(0x%08x )\n", stream, var_colorBuffer);
			this->rcBindRenderbuffer(var_colorBuffer);
			SET_LASTCALL("rcBindRenderbuffer");
			break;
		}
		case OP_rcColorBufferCacheFlush: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			EGLint var_postCount = Unpack<EGLint,uint32_t>(ptr + 8 + 4);
			int var_forRead = Unpack<int,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcColorBufferCacheFlush: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcColorBufferCacheFlush(0x%08x 0x%08x %d )\n", stream, var_colorbuffer, var_postCount, var_forRead);
			*(EGLint *)(&tmpBuf[0]) = 			this->rcColorBufferCacheFlush(var_colorbuffer, var_postCount, var_forRead);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcColorBufferCacheFlush");
			break;
		}
		case OP_rcReadColorBuffer: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			GLint var_x = Unpack<GLint,uint32_t>(ptr + 8 + 4);
			GLint var_y = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4);
			GLint var_width = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4 + 4);
			GLint var_height = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4);
			GLenum var_format = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4);
			GLenum var_type = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4);
			uint32_t size_pixels __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcReadColorBuffer: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_pixels;
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_pixels(&tmpBuf[0], size_pixels);
			DEBUG("renderControl(%p): rcReadColorBuffer(0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x %p(%u) )\n", stream, var_colorbuffer, var_x, var_y, var_width, var_height, var_format, var_type, (void*)(outptr_pixels.get()), size_pixels);
			this->rcReadColorBuffer(var_colorbuffer, var_x, var_y, var_width, var_height, var_format, var_type, (void*)(outptr_pixels.get()));
			outptr_pixels.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcReadColorBuffer");
			break;
		}
		case OP_rcUpdateColorBuffer: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			GLint var_x = Unpack<GLint,uint32_t>(ptr + 8 + 4);
			GLint var_y = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4);
			GLint var_width = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4 + 4);
			GLint var_height = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4);
			GLenum var_format = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4);
			GLenum var_type = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4);
			uint32_t size_pixels __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4);
			InputBuffer inptr_pixels(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4, size_pixels);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + size_pixels, ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + size_pixels, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcUpdateColorBuffer: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(int);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcUpdateColorBuffer(0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x %p(%u) )\n", stream, var_colorbuffer, var_x, var_y, var_width, var_height, var_format, var_type, (void*)(inptr_pixels.get()), size_pixels);
			*(int *)(&tmpBuf[0]) = 			this->rcUpdateColorBuffer(var_colorbuffer, var_x, var_y, var_width, var_height, var_format, var_type, (void*)(inptr_pixels.get()));
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcUpdateColorBuffer");
			break;
		}
		case OP_rcOpenColorBuffer2: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcOpenColorBuffer2: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(int);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcOpenColorBuffer2(0x%08x )\n", stream, var_colorbuffer);
			*(int *)(&tmpBuf[0]) = 			this->rcOpenColorBuffer2(var_colorbuffer);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcOpenColorBuffer2");
			break;
		}
		case OP_rcCreateClientImage: {
			uint32_t var_context = Unpack<uint32_t,uint32_t>(ptr + 8);
			EGLenum var_target = Unpack<EGLenum,uint32_t>(ptr + 8 + 4);
			GLuint var_buffer = Unpack<GLuint,uint32_t>(ptr + 8 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCreateClientImage: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(uint32_t);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcCreateClientImage(0x%08x 0x%08x 0x%08x )\n", stream, var_context, var_target, var_buffer);
			*(uint32_t *)(&tmpBuf[0]) = 			this->rcCreateClientImage(var_context, var_target, var_buffer);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcCreateClientImage");
			break;
		}
		case OP_rcDestroyClientImage: {
			uint32_t var_image = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcDestroyClientImage: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(int);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcDestroyClientImage(0x%08x )\n", stream, var_image);
			*(int *)(&tmpBuf[0]) = 			this->rcDestroyClientImage(var_image);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcDestroyClientImage");
			break;
		}
		case OP_rcSelectChecksumHelper: {
			uint32_t var_newProtocol = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_reserved = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4, ptr + 8 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcSelectChecksumHelper: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcSelectChecksumHelper(0x%08x 0x%08x )\n", stream, var_newProtocol, var_reserved);
			this->rcSelectChecksumHelper(var_newProtocol, var_reserved);
			SET_LASTCALL("rcSelectChecksumHelper");
			break;
		}
		case OP_rcCreateSyncKHR: {
			EGLenum var_type = Unpack<EGLenum,uint32_t>(ptr + 8);
			uint32_t size_attribs __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			InputBuffer inptr_attribs(ptr + 8 + 4 + 4, size_attribs);
			uint32_t var_num_attribs = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + size_attribs);
			int var_destroy_when_signaled = Unpack<int,uint32_t>(ptr + 8 + 4 + 4 + size_attribs + 4);
			uint32_t size_glsync_out __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + size_attribs + 4 + 4);
			uint32_t size_syncthread_out __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + size_attribs + 4 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + size_attribs + 4 + 4 + 4 + 4, ptr + 8 + 4 + 4 + size_attribs + 4 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCreateSyncKHR: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = size_glsync_out;
			totalTmpSize += size_syncthread_out;
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			OutputBuffer outptr_glsync_out(&tmpBuf[0], size_glsync_out);
			OutputBuffer outptr_syncthread_out(&tmpBuf[0 + size_glsync_out], size_syncthread_out);
			DEBUG("renderControl(%p): rcCreateSyncKHR(0x%08x %p(%u) 0x%08x %d %p(%u) %p(%u) )\n", stream, var_type, (EGLint*)(inptr_attribs.get()), size_attribs, var_num_attribs, var_destroy_when_signaled, (uint64_t*)(outptr_glsync_out.get()), size_glsync_out, (uint64_t*)(outptr_syncthread_out.get()), size_syncthread_out);
			this->rcCreateSyncKHR(var_type, (EGLint*)(inptr_attribs.get()), var_num_attribs, var_destroy_when_signaled, (uint64_t*)(outptr_glsync_out.get()), (uint64_t*)(outptr_syncthread_out.get()));
			outptr_glsync_out.flush();
			outptr_syncthread_out.flush();
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcCreateSyncKHR");
			break;
		}
		case OP_rcClientWaitSyncKHR: {
			uint64_t var_sync = Unpack<uint64_t,uint64_t>(ptr + 8);
			EGLint var_flags = Unpack<EGLint,uint32_t>(ptr + 8 + 8);
			uint64_t var_timeout = Unpack<uint64_t,uint64_t>(ptr + 8 + 8 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 8 + 4 + 8, ptr + 8 + 8 + 4 + 8, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcClientWaitSyncKHR: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(EGLint);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcClientWaitSyncKHR(0x%016lx 0x%08x 0x%016lx )\n", stream, var_sync, var_flags, var_timeout);
			*(EGLint *)(&tmpBuf[0]) = 			this->rcClientWaitSyncKHR(var_sync, var_flags, var_timeout);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcClientWaitSyncKHR");
			break;
		}
		case OP_rcFlushWindowColorBufferAsync: {
			uint32_t var_windowSurface = Unpack<uint32_t,uint32_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4, ptr + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcFlushWindowColorBufferAsync: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcFlushWindowColorBufferAsync(0x%08x )\n", stream, var_windowSurface);
			this->rcFlushWindowColorBufferAsync(var_windowSurface);
			SET_LASTCALL("rcFlushWindowColorBufferAsync");
			break;
		}
		case OP_rcDestroySyncKHR: {
			uint64_t var_sync = Unpack<uint64_t,uint64_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 8, ptr + 8 + 8, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcDestroySyncKHR: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(int);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcDestroySyncKHR(0x%016lx )\n", stream, var_sync);
			*(int *)(&tmpBuf[0]) = 			this->rcDestroySyncKHR(var_sync);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcDestroySyncKHR");
			break;
		}
		case OP_rcSetPuid: {
			uint64_t var_puid = Unpack<uint64_t,uint64_t>(ptr + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 8, ptr + 8 + 8, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcSetPuid: GL checksumCalculator failure\n");
			}
			DEBUG("renderControl(%p): rcSetPuid(0x%016lx )\n", stream, var_puid);
			this->rcSetPuid(var_puid);
			SET_LASTCALL("rcSetPuid");
			break;
		}
		case OP_rcUpdateColorBufferDMA: {
			uint32_t var_colorbuffer = Unpack<uint32_t,uint32_t>(ptr + 8);
			GLint var_x = Unpack<GLint,uint32_t>(ptr + 8 + 4);
			GLint var_y = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4);
			GLint var_width = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4 + 4);
			GLint var_height = Unpack<GLint,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4);
			GLenum var_format = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4);
			GLenum var_type = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4);
			uint64_t var_pixels_guest_paddr = Unpack<uint64_t,uint64_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4);
			void* var_pixels = stream->getDmaForReading(var_pixels_guest_paddr);
			uint32_t size_pixels __attribute__((unused)) = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8);
			uint32_t var_pixels_size = Unpack<uint32_t,uint32_t>(ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 4, ptr + 8 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcUpdateColorBufferDMA: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(int);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcUpdateColorBufferDMA(0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x %p(%u) 0x%08x )\n", stream, var_colorbuffer, var_x, var_y, var_width, var_height, var_format, var_type, var_pixels, var_pixels_size);
			*(int *)(&tmpBuf[0]) = 			this->rcUpdateColorBufferDMA(var_colorbuffer, var_x, var_y, var_width, var_height, var_format, var_type, var_pixels, var_pixels_size);
			stream->unlockDma(var_pixels_guest_paddr);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcUpdateColorBufferDMA");
			break;
		}
		case OP_rcCreateColorBufferDMA: {
			uint32_t var_width = Unpack<uint32_t,uint32_t>(ptr + 8);
			uint32_t var_height = Unpack<uint32_t,uint32_t>(ptr + 8 + 4);
			GLenum var_internalFormat = Unpack<GLenum,uint32_t>(ptr + 8 + 4 + 4);
			int var_frameworkFormat = Unpack<int,uint32_t>(ptr + 8 + 4 + 4 + 4);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::validOrDie(checksumCalc, ptr, 8 + 4 + 4 + 4 + 4, ptr + 8 + 4 + 4 + 4 + 4, checksumSize, 
					"renderControl_decoder_context_t::decode, OP_rcCreateColorBufferDMA: GL checksumCalculator failure\n");
			}
			size_t totalTmpSize = sizeof(uint32_t);
			totalTmpSize += checksumSize;
			unsigned char *tmpBuf = stream->alloc(totalTmpSize);
			DEBUG("renderControl(%p): rcCreateColorBufferDMA(0x%08x 0x%08x 0x%08x %d )\n", stream, var_width, var_height, var_internalFormat, var_frameworkFormat);
			*(uint32_t *)(&tmpBuf[0]) = 			this->rcCreateColorBufferDMA(var_width, var_height, var_internalFormat, var_frameworkFormat);
			if (useChecksum) {
				ChecksumCalculatorThreadInfo::writeChecksum(checksumCalc, &tmpBuf[0], totalTmpSize - checksumSize, &tmpBuf[totalTmpSize - checksumSize], checksumSize);
			}
			stream->flush();
			SET_LASTCALL("rcCreateColorBufferDMA");
			break;
		}
		default:
			return ptr - (unsigned char*)buf;
		} //switch
		ptr += packetLen;
	} // while
	return ptr - (unsigned char*)buf;
}
