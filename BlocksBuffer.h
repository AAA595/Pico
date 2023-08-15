#ifndef BLOCKS_BUFFER_H
#define BLOCKS_BUFFER_H

#include "CC_API/Vectors.h"
#include "CC_API/Core.h"

typedef struct BlocksBuffer_ {
	int width;
	int height;
	int length;
	BlockID* content;
	IVec3 anchor;
} BlocksBuffer;

BlocksBuffer GetCopiedBuffer();
bool BufferIsEmpty();
void SetCopiedBuffer(BlocksBuffer buffer);
int Copy(IVec3 mark1, IVec3 mark2);

#endif /* BLOCKS_BUFFER_H */