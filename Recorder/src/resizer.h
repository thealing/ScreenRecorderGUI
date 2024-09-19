#pragma once

#include "platform.h"

class Resizer : public IMFTransform {
public:
	STDMETHODIMP QueryInterface(REFIID, void**) override {
		return E_NOINTERFACE;
	}

	STDMETHODIMP_(ULONG) AddRef() override {
		return 0;
	}

	STDMETHODIMP_(ULONG) Release() override {
		uninit();
		delete this;
		return 0;
	}

	STDMETHODIMP GetStreamLimits(DWORD*, DWORD*, DWORD*, DWORD*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetStreamCount(DWORD*, DWORD*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetStreamIDs(DWORD, DWORD*, DWORD, DWORD*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetInputStreamInfo(DWORD, MFT_INPUT_STREAM_INFO*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetOutputStreamInfo(DWORD, MFT_OUTPUT_STREAM_INFO*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetAttributes(IMFAttributes**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetInputStreamAttributes(DWORD, IMFAttributes**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetOutputStreamAttributes(DWORD, IMFAttributes**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP DeleteInputStream(DWORD) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP AddInputStreams(DWORD, DWORD*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetInputAvailableType(DWORD, DWORD, IMFMediaType**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetOutputAvailableType(DWORD, DWORD, IMFMediaType**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetInputCurrentType(DWORD, IMFMediaType**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetOutputCurrentType(DWORD, IMFMediaType**) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetInputStatus(DWORD, DWORD*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP GetOutputStatus(DWORD*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP SetOutputBounds(LONGLONG, LONGLONG) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP ProcessEvent(DWORD, IMFMediaEvent*) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP ProcessMessage(MFT_MESSAGE_TYPE, ULONG_PTR) override {
		return E_NOTIMPL;
	}

	STDMETHODIMP SetInputType(DWORD, IMFMediaType* type, DWORD) override {
		type->GetGUID(MF_MT_SUBTYPE, &m_format);
		type->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&m_src_stride);
		UINT32 width, height;
		MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
		m_src_count = m_src_stride * height;
		return S_OK;
	}

	STDMETHODIMP SetOutputType(DWORD, IMFMediaType* type, DWORD) override {
		type->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&m_dst_stride);
		UINT32 width, height;
		MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
		m_dst_count = m_dst_stride * height;
		return S_OK;
	}

	STDMETHODIMP ProcessInput(DWORD, IMFSample* sample, DWORD) override {
		m_input_sample = sample;
		m_input_sample->AddRef();
		return S_OK;
	}

	STDMETHODIMP ProcessOutput(DWORD, DWORD, MFT_OUTPUT_DATA_BUFFER* samples, DWORD*) override {
		IMFSample* output_sample = samples[0].pSample;
		IMFMediaBuffer* input_buffer;
		m_input_sample->GetBufferByIndex(0, &input_buffer);
		IMFMediaBuffer* output_buffer;
		output_sample->GetBufferByIndex(0, &output_buffer);
		BYTE* input_data;
		input_buffer->Lock(&input_data, NULL, NULL);
		BYTE* output_data;
		output_buffer->Lock(&output_data, NULL, NULL);
		work(input_data, output_data);
		input_buffer->Unlock();
		output_buffer->Unlock();
		input_buffer->Release();
		output_buffer->Release();
		LONGLONG value = 0;
		m_input_sample->GetSampleTime(&value);
		output_sample->SetSampleTime(value);
		m_input_sample->GetSampleDuration(&value);
		output_sample->SetSampleDuration(value);
		while (m_input_sample->Release() > 0);
		return S_OK;
	}

	void set_src_rect(RECT* rect) {
		m_src_rect = *rect;
	}

	void set_dst_rect(RECT* rect) {
		m_dst_rect = *rect;
	}

	void set_quality(bool quality) {
		m_quality = quality;
		init();
	}

private:
	void init() {
		int src_width = m_src_rect.right - m_src_rect.left;
		int src_height = m_src_rect.bottom - m_src_rect.top;
		int dst_width = m_dst_rect.right - m_dst_rect.left;
		int dst_height = m_dst_rect.bottom - m_dst_rect.top;
		m_table_1 = (int*)malloc(m_dst_count * sizeof(int));
		m_table_2 = (int*)malloc(m_dst_count / 4 * sizeof(int));
		for (int dst_y = 0; dst_y < dst_height; dst_y++) {
			int src_y = dst_y * src_height / dst_height;
			for (int dst_x = 0; dst_x < dst_width; dst_x++) {
				int src_x = dst_x * src_width / dst_width;
				m_table_1[m_dst_stride * (m_dst_rect.top + dst_y) + (m_dst_rect.left + dst_x)] = m_src_stride * (m_src_rect.top + src_y) + (m_src_rect.left + src_x);
				m_table_2[m_dst_stride / 2 * ((m_dst_rect.top + dst_y) / 2) + (m_dst_rect.left + dst_x) / 2] = m_src_stride / 2 * ((m_src_rect.top + src_y) / 2) + (m_src_rect.left + src_x) / 2;
			}
		}
	}

	void uninit() {
		free(m_table_1);
		free(m_table_2);
	}

	void work(BYTE* input, BYTE* output) {
		if (m_quality) {
			// TODO: high quality algorithm
		}
		stretch_y(input, output);
		if (m_format == MFVideoFormat_IYUV) {
			stretch_u(input + m_src_count, output + m_dst_count);
			stretch_u(input + m_src_count * 5 / 4, output + m_dst_count * 5 / 4);
		}
		if (m_format == MFVideoFormat_NV12) {
			stretch_uv((uint16_t*)(input + m_src_count), (uint16_t*)(output + m_dst_count));
		}
	}

	void stretch_y(uint8_t* src, uint8_t* dst) {
		for (int i = m_dst_rect.top; i < m_dst_rect.bottom; i++) {
			for (int j = m_dst_rect.left, k = m_dst_stride * i + j; j < m_dst_rect.right; j++, k++) {
				dst[k] = src[m_table_1[k]];
			}
		}
	}

	void stretch_u(uint8_t* src, uint8_t* dst) {
		for (int i = m_dst_rect.top / 2; i < m_dst_rect.bottom / 2; i++) {
			for (int j = m_dst_rect.left / 2, k = m_dst_stride / 2 * i + j; j < m_dst_rect.right / 2; j++, k++) {
				dst[k] = src[m_table_2[k]];
			}
		}
	}

	void stretch_uv(uint16_t* src, uint16_t* dst) {
		for (int i = m_dst_rect.top / 2; i < m_dst_rect.bottom / 2; i++) {
			for (int j = m_dst_rect.left / 2, k = m_dst_stride / 2 * i + j; j < m_dst_rect.right / 2; j++, k++) {
				dst[k] = src[m_table_2[k]];
			}
		}
	}

private:
	GUID m_format;
	int m_src_stride;
	int m_dst_stride;
	int m_src_count;
	int m_dst_count;
	RECT m_src_rect;
	RECT m_dst_rect;
	bool m_quality;
	int* m_table_1;
	int* m_table_2;
	IMFSample* m_input_sample;
};
