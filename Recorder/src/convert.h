#pragma once

#include <stdint.h>
#include <intrin.h>

void p32_to_y(const uint32_t* pb, uint8_t* y, __m128i m, int width, int height)
{
	__m128i w[16];

	for (int i = 0; i < width * height; i += 32)
	{
		for (int j = 0; j < 8; j++)
		{
			__m128i p = _mm_load_si128((__m128i*)(pb + i + j * 4));

			w[j * 2] = _mm_cvtepu8_epi16(p);

			w[j * 2 + 1] = _mm_cvtepu8_epi16(_mm_unpackhi_epi64(p, p));
		}

		for (int j = 0; j < 16; j++)
		{
			w[j] = _mm_mullo_epi16(w[j], m);
		}

		for (int j = 0; j < 8; j++)
		{
			w[j] = _mm_hadd_epi16(w[j * 2], w[j * 2 + 1]);
		}

		for (int j = 0; j < 4; j++)
		{
			w[j] = _mm_hadd_epi16(w[j * 2], w[j * 2 + 1]);
		}

		for (int j = 0; j < 4; j++)
		{
			w[j] = _mm_srli_epi16(w[j], 8);
		}

		for (int j = 0; j < 4; j++)
		{
			w[j] = _mm_add_epi16(w[j], _mm_set1_epi16(20));
		}

		_mm_store_si128((__m128i*)(y + i), _mm_packus_epi16(w[0], w[1]));

		_mm_store_si128((__m128i*)(y + i + 16), _mm_packus_epi16(w[2], w[3]));
	}
}

void p32_to_uv(const uint32_t* pb, uint8_t* uv, __m128i m, int width, int height)
{
	for (int i = 0, r = 0; r < height; r += 2)
	{
		for (int c = 0; c < width; i += 32, c += 32)
		{
			__m128i w[16];

			for (int j = 0; j < 8; j++)
			{
				__m128i p0 = _mm_load_si128((__m128i*)(pb + r * width + c + j * 4));

				__m128i p1 = _mm_load_si128((__m128i*)(pb + (r + 1) * width + c + j * 4));

				p0 = _mm_avg_epu8(p0, p1);

				p1 = _mm_shuffle_epi32(p0, _MM_SHUFFLE(2, 3, 0, 1));

				p0 = _mm_avg_epu8(p0, p1);

				p1 = _mm_unpackhi_epi64(p0, p0);

				w[j * 2] = _mm_cvtepu8_epi16(p0);

				w[j * 2 + 1] = _mm_cvtepu8_epi16(p1);
			}

			for (int j = 0; j < 16; j++)
			{
				w[j] = _mm_mullo_epi16(w[j], m);
			}

			for (int j = 0; j < 8; j++)
			{
				w[j] = _mm_hadd_epi16(w[j * 2], w[j * 2 + 1]);
			}

			for (int j = 0; j < 4; j++)
			{
				w[j] = _mm_hadd_epi16(w[j * 2], w[j * 2 + 1]);
			}

			for (int j = 0; j < 4; j++)
			{
				w[j] = _mm_srai_epi16(w[j], 8);
			}

			for (int j = 0; j < 4; j++)
			{
				w[j] = _mm_add_epi16(w[j], _mm_set1_epi16(128));
			}

			_mm_store_si128((__m128i*)(uv + i), _mm_packus_epi16(w[0], w[1]));

			_mm_store_si128((__m128i*)(uv + i + 16), _mm_packus_epi16(w[2], w[3]));
		}
	}
}

void p32_to_uv(const uint32_t* bgr, uint8_t* u, uint8_t* v, __m128i mu, __m128i mv, int width, int height)
{
	for (int i = 0, r = 0; r < height; r += 2)
	{
		for (int c = 0; c < width; i += 16, c += 32)
		{
			__m128i w[16];

			for (int j = 0; j < 8; j++)
			{
				__m128i p0 = _mm_load_si128((__m128i*)(bgr + r * width + c + j * 4));

				__m128i p1 = _mm_load_si128((__m128i*)(bgr + (r + 1) * width + c + j * 4));

				p0 = _mm_avg_epu8(p0, p1);

				p1 = _mm_shuffle_epi32(p0, _MM_SHUFFLE(2, 3, 0, 1));

				p0 = _mm_avg_epu8(p0, p1);

				p1 = _mm_shuffle_epi32(p0, _MM_SHUFFLE(1, 2, 3, 0));

				w[j] = _mm_cvtepu8_epi16(p1);

				w[j + 8] = w[j];
			}

			for (int j = 0; j < 8; j++)
			{
				w[j] = _mm_mullo_epi16(w[j], mu);
			}

			for (int j = 8; j < 16; j++)
			{
				w[j] = _mm_mullo_epi16(w[j], mv);
			}

			for (int j = 0; j < 8; j++)
			{
				w[j] = _mm_hadd_epi16(w[j * 2], w[j * 2 + 1]);
			}

			for (int j = 0; j < 4; j++)
			{
				w[j] = _mm_hadd_epi16(w[j * 2], w[j * 2 + 1]);
			}

			for (int j = 0; j < 4; j++)
			{
				w[j] = _mm_srai_epi16(w[j], 8);
			}

			for (int j = 0; j < 4; j++)
			{
				w[j] = _mm_add_epi16(w[j], _mm_set1_epi16(128));
			}

			_mm_store_si128((__m128i*)(u + i), _mm_packus_epi16(w[0], w[1]));

			_mm_store_si128((__m128i*)(v + i), _mm_packus_epi16(w[2], w[3]));
		}
	}
}

void convert_bgr32_to_nv12(const uint32_t* bgr, uint8_t* y, uint8_t* uv, int width, int height) 
{
	__m128i my = _mm_set_epi16(0, 66, 129, 25, 0, 66, 129, 25);

	__m128i muv = _mm_set_epi16(0, 112, -94, -18, 0, -38, -74, 112);

	p32_to_y(bgr, y, my, width, height);

	p32_to_uv(bgr, uv, muv, width, height);
}

void convert_rgb32_to_nv12(const uint32_t* rgb, uint8_t* y, uint8_t* uv, int width, int height) 
{
	__m128i my = _mm_set_epi16(0, 25, 129, 66, 0, 25, 129, 66);

	__m128i muv = _mm_set_epi16(0, -18, -94, 112, 0, 112, -74, -38);

	p32_to_y(rgb, y, my, width, height);

	p32_to_uv(rgb, uv, muv, width, height);
}

void convert_bgr32_to_iyuv(const uint32_t* bgr, uint8_t* y, uint8_t* u, uint8_t* v, int width, int height)
{
	__m128i my = _mm_set_epi16(0, 66, 129, 25, 0, 66, 129, 25);

	__m128i mu = _mm_set_epi16(0, -38, -74, 112, 0, -38, -74, 112);

	__m128i mv = _mm_set_epi16(0, 112, -94, -18, 0, 112, -94, -18);

	p32_to_y(bgr, y, my, width, height);

	p32_to_uv(bgr, u, v, mu, mv, width, height);
}

void convert_rgb32_to_iyuv(const uint32_t* bgr, uint8_t* y, uint8_t* u, uint8_t* v, int width, int height)
{
	__m128i my = _mm_set_epi16(0, 25, 129, 66, 0, 25, 129, 66);

	__m128i mu = _mm_set_epi16(0, 112, -74, -38, 0, 112, -74, -38);

	__m128i mv = _mm_set_epi16(0, -18, -94, 112, 0, -18, -94, 112);

	p32_to_y(bgr, y, my, width, height);

	p32_to_uv(bgr, u, v, mu, mv, width, height);
}

void convert_bgr_to_rgb(const uint32_t* bgr, uint32_t* rgb, int width, int height) 
{
	__m128i m = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);

	for (int i = 0; i < width * height; i += 4) 
	{
		__m128i w = _mm_load_si128((__m128i*)(bgr + i));

		w = _mm_shuffle_epi8(w, m);

		_mm_store_si128((__m128i*)(rgb + i), w);
	}
}
