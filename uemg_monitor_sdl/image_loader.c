
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <png.h>
#include <string.h>
#include "image_loader.h"

typedef struct MemoryStruct 
{
	char *memory;
	size_t size;
	size_t pos;
}MemoryStruct;

void png_read_func(png_structp pngPtr, png_bytep data, png_size_t length)
{
	MemoryStruct* ms = (MemoryStruct*)png_get_io_ptr(pngPtr);
	memcpy(data, ms->memory + ms->pos, length);
	ms->pos += length;
}

uint8_t *img_load_png(char *filename, int *res_x, int *res_y)
{
	int fl = open(filename, O_RDONLY);
	if(fl < 0) return NULL;
	int fsz =lseek(fl, 0L, SEEK_END);
	lseek(fl, 0L, SEEK_SET);
	if(fsz > 92345678)
	{
		printf("png file too large (>92Mb)\n");
		return NULL; //file too large
	}
	if(fsz < 100)
	{
		printf("png file too small (<100b)\n");
		return NULL; //file too large
	}

	struct MemoryStruct img_png;

	img_png.memory = (char *)malloc(fsz);
	img_png.size = fsz;
	img_png.pos = 0;
	read(fl, img_png.memory, fsz);

	close(fl);

	uint8_t pngSignature[8];
	memcpy(pngSignature, img_png.memory, 8);
	img_png.pos = 8;
	if(!png_check_sig(pngSignature, 8))
	{
		free(img_png.memory);
		return NULL;		
	}

	// get PNG file info struct (memory is allocated by libpng)
	png_structp png_ptr = NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(png_ptr == NULL)
	{
		free(img_png.memory);
		return NULL;
	}

	// get PNG image data info struct (memory is allocated by libpng)
	png_infop info_ptr = NULL;
	info_ptr = png_create_info_struct(png_ptr);

	if(info_ptr == NULL)
	{
		// libpng must free file info struct memory before we bail
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		free(img_png.memory);
		return NULL;
	}

	png_set_read_fn(png_ptr, &img_png, png_read_func);
	png_set_sig_bytes(png_ptr, 8);

//=============
	png_read_info(png_ptr, info_ptr);
	png_uint_32 width = 0;
	png_uint_32 height = 0;
	int bitDepth = 0;
	int colorType = -1;
	png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth, &colorType, NULL, NULL, NULL);

	if(retval != 1)
	{
		free(img_png.memory);
		return NULL; // add error handling and cleanup
	}
	//printf("png: w=%d, h=%d, bpp=%d, colorType %d (%d %d %d) \n", width, height, bitDepth, colorType, PNG_COLOR_TYPE_RGB, PNG_COLOR_TYPE_RGB_ALPHA, PNG_COLOR_TYPE_PALETTE);

	uint8_t *pix_array = (uint8_t*)malloc(width*height*3);
	*res_x = width;
	*res_y = height;
	uint8_t *cpix = pix_array;

	if(colorType == PNG_COLOR_TYPE_RGB)
	{
		int w = width;
		int h = height;

		int bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
		uint8_t* rowData = (uint8_t*)malloc(bytesPerRow);
		for(int x = 0; x < h; ++x)
		{
			png_read_row(png_ptr, (png_bytep)rowData, NULL);
			for(int n = 0; n < w; ++n)
			{
				*cpix++ = rowData[n*3];
				*cpix++ = rowData[n*3+1];
				*cpix++ = rowData[n*3+2];
			}
		}
		free(rowData);
	}
	if(colorType == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		int w = width;
		int h = height;

		int bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
		uint8_t* rowData = (uint8_t*)malloc(bytesPerRow);
		for(int x = 0; x < h; ++x)
		{
			png_read_row(png_ptr, (png_bytep)rowData, NULL);
			for(int n = 0; n < w; ++n)
			{
				*cpix++ = rowData[n*4+1];
				*cpix++ = rowData[n*4+2];
				*cpix++ = rowData[n*4+3];
			}
		}
		free(rowData);
	}
	if(colorType == PNG_COLOR_TYPE_PALETTE)
	{
		png_color *plt;
		int psz = 0;
		png_get_PLTE(png_ptr, info_ptr, &plt, &psz);
		int w = width;
		int h = height;

		int bytesPerRow = png_get_rowbytes(png_ptr, info_ptr);
		uint8_t* rowData = (uint8_t*)malloc(bytesPerRow);
		for(int x = 0; x < h; ++x)
		{
			png_read_row(png_ptr, (png_bytep)rowData, NULL);
			for(int n = 0; n < w; ++n)
			{
				png_color cl = plt[rowData[n]];
				*cpix++ = cl.blue;
				*cpix++ = cl.green;
				*cpix++ = cl.red;
			}
		}
		free(rowData);
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	free(img_png.memory);
	return pix_array;
}