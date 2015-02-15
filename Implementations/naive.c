#include "../Framework/interface.h"

targetType* performCrack(targetType* buffer, payloadType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	size_t lowerCursor = 0, upperCursor = bufferSize - 1;
	while (buffer[lowerCursor] < pivot)
		lowerCursor++;

	while (lowerCursor < upperCursor) {
		if (buffer[lowerCursor] >= pivot && buffer[upperCursor] < pivot) {
			targetType tmp = buffer[lowerCursor];
			buffer[lowerCursor] = buffer[upperCursor];
			buffer[upperCursor] = tmp;
		#ifdef DO_PAYLOAD_SHUFFLE
			payloadType tmpPayload = payloadBuffer[lowerCursor];
			payloadBuffer[lowerCursor] = payloadBuffer[upperCursor];
			payloadBuffer[upperCursor] = tmpPayload;
		#endif
			lowerCursor++;
			upperCursor--;
		} else if (buffer[lowerCursor] < pivot) {
			lowerCursor++;
		} else if (buffer[upperCursor] >= pivot) {
			upperCursor--;
		}
	}
	return NULL;
}

