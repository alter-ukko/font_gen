#include "misc_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

bool file_exists(const char *input_file_name) {
	FILE *input_file = fopen(input_file_name, "rb");
	if (input_file == NULL) {
		return false;
	}
	fclose(input_file);
	return true;
}

unsigned char* load_file(const char *input_file_name) {
	FILE *input_file = fopen(input_file_name, "rb");
	if (input_file == NULL) {
		printf("could not open %s\n",input_file_name);
		return NULL;
	}
	fseek(input_file, 0, SEEK_END);
	long input_file_size = ftell(input_file);
	rewind(input_file);
	unsigned char *file_contents = (unsigned char *)malloc((input_file_size * (sizeof(unsigned char)))+1);
	fread(file_contents, sizeof(unsigned char), (size_t)input_file_size, input_file);
	fclose(input_file);
	file_contents[input_file_size] = 0;
	return file_contents;
}

float rand_float() {
	return (float)(rand() % 10000) / 10000.0f;
}

int rand_int(int mod) {
	return (rand() % mod);
}

unsigned int rand_uint(unsigned int mod) {
	return ((unsigned int)rand() % mod);
}

unsigned char rand_ubyte(int mod) {
	return (unsigned char) (rand() % mod);
}

// oscillates between min and min+range with a period of 1.0
float oscillate(float val, float min, float range) {
	return (((sinf(val * (float)TWO_PI) * 0.5f) + 0.5f) * range) + min;
}

float elastic(float p) {
	if (p < 0.3f) {
		return sinf(-13.0f * (float)M_PI_2 * (p + 1.0f)) * powf(2.0f, -22.0f * p) + 1.0f;
	} else {
		return (sinf((p+0.65f) * 4.0f * (float)M_PI)* 0.01f) + 1.0f;
	}
}

float signum(float x) {
	return x > 0 ? 1 : x < 0 ? -1 : 0;
}

float intbound(float s, float ds) {
	bool is_int = (roundf(s) == s);
	if (ds < 0 && is_int) return 0;
	return ((ds > 0) ? ceilf(s)-s : s-floorf(s)) / fabsf(ds);
}

