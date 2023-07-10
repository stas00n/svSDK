#pragma once
#include <stdint.h>
#include <string.h>

template <const uint32_t BUF_SIZE>
class CStreamBuffer {
	static_assert ((BUF_SIZE& (BUF_SIZE - 1)) == 0, "BUF_SIZE must be a power of 2!");

private:
	const uint32_t 			mask = BUF_SIZE - 1;
	alignas(8) uint8_t		_buffer[BUF_SIZE]{};
	volatile uint32_t		head = 0;
	volatile uint32_t   	tail = 0;

public:
	/* Error counter */
	uint32_t nOverflows = 0;

public:
	/**
	 * Write single byte
	 */
	inline bool Put(uint8_t dataByte) {
		if (IsFull()) {
			nOverflows++;
			return false;
		}
		_buffer[(tail & mask)] = dataByte;
		tail++;
		return true;
	}


	/**
	 * @brief ������ � ����� (������, ���������).
	 */
	uint32_t Write(uint8_t* data, uint32_t dataLen) {
		uint32_t wrcnt = 0;
		for (uint32_t i = 0; i < dataLen; i++) {
			if (!Put(data[i])) {
				break;
			}
			wrcnt++;
		}
		return wrcnt;
	}

	/**
	 * @brief ������ � �����
	 * @param data ��������� �� ������ ��� ������
	 * @param size ������������� ���������� ���� ��� ������
	 * @retval ���������� ���������� ���������� ����. � ���������� ������� ��������� ���������
	 * size � retval. ��� ����������� - ���������� ��� ������ ������������ ������
	 */
	size_t Write2(void* data, size_t size) {

		/* ��������� ���������� �� ����� */
		uint32_t space = GetSpace();
		if (size > space) {
			size = space;
			nOverflows++;
		}

		/* ��� ������������� ��������� ������ �� 2 ����� - ������ � ������� */
		uint32_t indx_tail = tail & mask;
		uint32_t spaceToEndBuf = BUF_SIZE - indx_tail;
		uint32_t sizeFirst = size;
		uint32_t sizeRem;
		if (spaceToEndBuf < size) {
			sizeFirst = spaceToEndBuf;
			sizeRem = size - sizeFirst;
		}
		else {
			sizeRem = 0;
		}

		uint8_t* p8 = (uint8_t*)data;
		/* �������� ��������� ����� */
		if (sizeFirst) {
			memcpy(_buffer + indx_tail, p8, sizeFirst);
		}

		/* �������� ������� */
		if (sizeRem) {
			p8 += sizeFirst;
			memcpy(_buffer, p8, sizeRem);
		}

		tail += size;
		return size;
	}

	/**
	 * Read multiple bytes
	 */
	uint32_t Read(void* dest, uint32_t nBytesToRead) {
		uint32_t nBytesAvailible = GetCount();

		if (nBytesToRead > nBytesAvailible) {
			nBytesToRead = nBytesAvailible;
		}

		uint32_t br = 0;
		uint8_t* pu8 = (uint8_t*)dest;
		while (nBytesToRead--) {
			uint32_t indxRead = head;
			indxRead &= mask;
			*pu8 = _buffer[indxRead];
			pu8++;
			br++;
			head++;
		}
		return br;
	}

	/**
	 * @brief ������ �� ������
	 * @param dest ����� ����������
	 * @param nBytesToRead ������������� ���������� ���� ��� ������
	 * @retval ���������� ���������� ����������� ����.
	 */
	size_t Read2(void* dest, uint32_t nBytesToRead) {

		/* ������ �� ������ ��� ���� � ������ */
		uint32_t count = GetCount();
		if (nBytesToRead > count) {
			nBytesToRead = count;
		}

		/* ��� ������������� ��������� ������ �� 2 ����� - ������ � ������� */
		uint32_t indx_head = head & mask;
		uint32_t spaceToEndBuf = BUF_SIZE - indx_head;
		uint32_t sizeFirst = nBytesToRead;
		uint32_t sizeRem;
		if (spaceToEndBuf < nBytesToRead) {
			sizeFirst = spaceToEndBuf;
			sizeRem = nBytesToRead - sizeFirst;
		}
		else {
			sizeRem = 0;
		}

		uint8_t* p8 = (uint8_t*)dest;
		/* �������� ��������� ����� */
		if (sizeFirst) {
			memcpy(p8, (_buffer + indx_head), sizeFirst);
		}

		/* �������� ������� */
		if (sizeRem) {
			p8 += sizeFirst;
			memcpy(p8, _buffer, sizeRem);
		}

		head += nBytesToRead;
		return nBytesToRead;
	}

	/**
	 * @retval ���-�� ���� ��������� ��� ������
	 */
	inline uint32_t GetCount() const {
		uint32_t count = tail;
		count -= head;
		count &= mask;
		return count;
	}

	/**
	 * @retval ��������� �����
	 */
	inline uint32_t GetSpace() {
		return (BUF_SIZE - 1 - GetCount());
	}

	/**
	 * @brief ���� �� �����
	 */
	inline bool IsEmpty() const {
		uint32_t tmp = head;
		return (tail == tmp);
	}

	/**
	 * @brief ����� �� �����
	 */
	inline bool IsFull() const {
		uint32_t count = tail;
		count -= head;
		return ((count & (~mask)) != 0);

	}

	/**
	 * @brief �������. ������� ��� ������������
	 */
	inline void Reset() {
		head = tail = 0;
	}
};

