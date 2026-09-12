#include "BilinearResizer.h"

BilinearResizer::BilinearResizer(Vector inputSize, Vector outputSize, const Rect& inputRect, const Rect& outputRect) : AbstractResizer(outputSize, outputRect)
{
	_indexBuffer = BufferUtil::allocateBuffer<int>(outputSize.x * outputSize.y * 4);
	_weightBuffer = BufferUtil::allocateBuffer<int>(outputSize.x * outputSize.y);
	int inputWidth = inputRect.upper.x - inputRect.lower.x;
	int inputHeight = inputRect.upper.y - inputRect.lower.y;
	int outputWidth = outputRect.upper.x - outputRect.lower.x;
	int outputHeight = outputRect.upper.y - outputRect.lower.y;
	__m128 weightFactor = _mm_set_ps1(32.0f);
	__m128i weightMask = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8, 12);
	__m128i offsetX = _mm_set1_epi32(inputRect.lower.x);
	__m128i offsetY = _mm_set1_epi32(inputRect.lower.y);
	__m128i stride = _mm_set1_epi32(inputSize.x);
	__m128i min = _mm_setzero_si128();
	__m128i maxX = _mm_set1_epi32(inputWidth - 1);
	__m128i maxY = _mm_set1_epi32(inputHeight - 1);
	for (int outputY = 0; outputY < outputHeight; outputY++)
	{
		float inputY = (outputY + 0.5f) * inputHeight / outputHeight - 0.5f;
		float fractionY = inputY - floorf(inputY);
		float w0 = fractionY;
		float w1 = 1.0f - fractionY;
		__m128 w = _mm_set_ps(w1, w1, w0, w0);
		int y0 = (int)floorf(inputY);
		int y1 = (int)ceilf(inputY);
		__m128i y = _mm_set_epi32(y1, y1, y0, y0);
		y = _mm_max_epi32(y, min);
		y = _mm_min_epi32(y, maxY);
		y = _mm_add_epi32(y, offsetY);
		y = _mm_mullo_epi32(y, stride);
		y = _mm_add_epi32(y, offsetX);
		float deltaX = (float)inputWidth / (float)outputWidth;
		float inputX = 0.5f * deltaX - 0.5f;
		int index = outputSize.x * (outputRect.lower.y + outputY) + outputRect.lower.x;
		for (int outputX = 0; outputX < outputWidth; outputX++)
		{
			int x0 = (int)floorf(inputX);
			int x1 = (int)ceilf(inputX);
			__m128i x = _mm_set_epi32(x1, x0, x1, x0);
			x = _mm_max_epi32(x, min);
			x = _mm_min_epi32(x, maxX);
			x = _mm_add_epi32(x, y);
			_mm_store_si128((__m128i*)&_indexBuffer[index * 4], x);
			float fractionX = inputX - floorf(inputX);
			float v0 = fractionX;
			float v1 = 1.0f - fractionX;
			__m128 v = _mm_set_ps(v1, v0, v1, v0);
			v = _mm_add_ps(v, w);
			v = _mm_mul_ps(v, weightFactor);
			__m128i u = _mm_cvtps_epi32(v);
			u = _mm_shuffle_epi8(u, weightMask);
			_weightBuffer[index] = _mm_cvtsi128_si32(u);
			inputX += deltaX;
			index++;
		}
	}
}

BilinearResizer::~BilinearResizer()
{
	BufferUtil::freeBuffer(_indexBuffer);
	BufferUtil::freeBuffer(_weightBuffer);
}

void BilinearResizer::resize(const uint32_t* inputPixels, uint32_t* outputPixels)
{
	__m128i mask1 = _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
	__m128i mask2 = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 3, 5, 1, 6, 4, 2, 0);
	for (int y = _outputRect.lower.y; y < _outputRect.upper.y; y++)
	{
		int* indexBuffer = _indexBuffer;
		int* weightBuffer = _weightBuffer;
		int row = _outputSize.x * y;
		int end = row + _outputRect.upper.x;
		for (int index = row + _outputRect.lower.x; index < end; index++)
		{
			int inputIndex0 = indexBuffer[index * 4 + 0];
			int inputIndex1 = indexBuffer[index * 4 + 1];
			int inputIndex2 = indexBuffer[index * 4 + 2];
			int inputIndex3 = indexBuffer[index * 4 + 3];
			__m128i v = _mm_set_epi32(inputPixels[inputIndex3], inputPixels[inputIndex2], inputPixels[inputIndex1], inputPixels[inputIndex0]);
			__m128i w = _mm_set1_epi32(weightBuffer[index]);
			v = _mm_shuffle_epi8(v, mask1);
			v = _mm_maddubs_epi16(v, w);
			v = _mm_hadd_epi16(v, v);
			v = _mm_srli_epi16(v, 7);
			v = _mm_shuffle_epi8(v, mask2);
			outputPixels[index] = _mm_cvtsi128_si32(v);
		}
	}
}
