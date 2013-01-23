#include <png.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// 4 works best on a quiet machine at nice -20
// 8 on a noisy machine at default priority
#define NWORKERS 8

#define WIDTH 256

#define ERROR 1


int hsv2rgb(float h, float s, float v) {
	float r = 0;
	float g = 0;
	float b = 0;

	if (s == 0) {
		v *= 255;
		return ((int) v) << 24 | ((int) v) << 16 | ((int) v) << 8;
	}

	h /= 256 / 6.0;

	int i = (int) h;
	float f = h - i; //f is fractional part of h
	float p = v * (1 - s);
	float q = v * (1 - (s * f));
	float t = v * (1 - (s * (1 - f)));

	switch (i) {
		case 0:
		  r = v;
		  g = t;
		  b = p;

		  break;

		case 1:
		  r = q;
		  g = v;
		  b = p;

		  break;

		case 2:
		  r = p;
		  g = v;
		  b = t;

		  break;

		case 3:
		  r = p;
		  g = q;
		  b = v;

		  break;

		case 4:
		  r = t;
		  g = p;
		  b = v;

		  break;

		case 5:
		  r = v;
		  g = p;
		  b = q;

		  break;
	}

	// now assign everything....
	//             g                         r                ignored
	return ((int) (b * 255)) << 24 | ((int) (g * 255)) << 16 | ((int) r * 255) << 8;
}


void mkdir_r(const char *dir) {
        char tmp[256];
        char *p = NULL;
        size_t len;
        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO);
                        *p = '/';
                }
        mkdir(tmp, S_IRWXU | S_IRWXG | S_IRWXO);
}

int tiles;
int image[WIDTH][WIDTH];
int* rows[WIDTH];
png_color_16 trans_color;
double inverse_N;

void gen_tile(int tileZ, int tileX, int tileY) {
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;

	char file[256];
	sprintf(file, "%d/%d/%d.png", tileZ, tileX, tileY);
	
	// file exists
	if (access(file, R_OK | W_OK) == 0) {
		return;
	}
	
	fp = fopen(file, "wb");
	if (fp == NULL)
		return;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return;
	}
	
	// optimisation stuff
#if defined(PNG_LIBPNG_VER) && (PNG_LIBPNG_VER >= 10200)
	png_uint_32 mask, flags;

	flags = png_get_asm_flags(png_ptr);
	mask = png_get_asm_flagmask(PNG_SELECT_READ | PNG_SELECT_WRITE);
	png_set_asm_flags(png_ptr, flags | mask);
#endif
	png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  png_infopp_NULL);
		return;
	}


	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, WIDTH, WIDTH, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_set_tRNS(png_ptr, info_ptr, 0, 0, &trans_color);
   
	/* calc mandelbrot here */
	
	
	
	int y = -1;
	
	
	
	for (int i = 0; i < WIDTH; i++) {
		rows[i] = &(image[i]);
	}

	while ((++y) < WIDTH) {
		double Civ = (double) (y + tileY * 256) * inverse_N
				- 1.0;
		for (int x = 0; x < WIDTH; x++) {
			double Crv = (double) (x + tileX * 256)
					* inverse_N - 1.5;

			double Zrv = Crv;
			double Ziv = Civ;

			double Trv = Crv * Crv;
			double Tiv = Civ * Civ;

			int i = 256;
			do {
				Ziv = (Zrv * Ziv) + (Zrv * Ziv) + Civ;
				Zrv = Trv - Tiv + Crv;

				Trv = Zrv * Zrv;
				Tiv = Ziv * Ziv;
			} while (((Trv + Tiv) <= 4.0) && (--i > 0));

			if (i == 0) {
				image[y][x] = 0;
			} else {
				image[y][x] = hsv2rgb(256 - i, 1, 1);
			}
		}
	}
	png_set_rows(png_ptr, info_ptr, rows);

	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_FILLER, NULL);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}

int main (int argc, char **argv)
{

	int tileZ = atoi(argv[1]);
	int tileX = atoi(argv[2]);
	int tileY = atoi(argv[3]);
	
	tiles = 1 << tileZ;
	inverse_N = 2.0 / tiles / WIDTH;
	trans_color.red = trans_color.green = trans_color.blue = 0;
	
	/* open the file */
	char dir[256];
	sprintf(dir, "%d/%d", tileZ, tileX);

	mkdir_r(dir);
	
	if (tileY == -1) {
		for (int i = 0; i < tiles; i++) {
			gen_tile(tileZ, tileX, i);
		}
	} else {
		gen_tile(tileZ, tileX, tileY);
	}
}



