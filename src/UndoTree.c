#include <string.h>
#include <stdlib.h>

#include "ClassiCube/src/World.h"
#include "ClassiCube/src/Game.h"
#include "ClassiCube/src/Event.h"

#include "Message.h"
#include "UndoTree.h"
#include "Format.h"
#include "Memory.h"
#include "MarkSelection.h"
#include "DataStructures/List.h"

typedef struct BlockChangeEntry_ {
    int x;
    int y;
    int z;
    DeltaBlockID delta;
} BlockChangeEntry;

typedef struct UndoNode_ UndoNode;

typedef struct UndoNode_ {
    int commit;
    char description[STRING_SIZE];
    BlockChangeEntry* blockDeltas;
    int blockDeltasCount;
    int blockDeltasCapacity;
    time_t timestamp;
    int parentIndex;
    int* children;
    size_t childrenCount;
    size_t childrenCapacity;
} UndoNode;

static bool s_Enabled = false;
static int s_CurrentNodeIndex = 0;

static UndoNode* s_Nodes = NULL;
static size_t s_NodesCount = 0;
static size_t s_NodesCapacity = 0;

static int* s_RedoStack = NULL;
static size_t s_RedoStackCapacity = 0;
static size_t s_RedoStackCount = 0;

static void OnBlockChanged(void* obj, IVec3 coords, BlockID oldBlock, BlockID block) {
    if (MarkSelection_RemainingMarks() > 0) {
        return;
    }

    if (block == BLOCK_AIR) {
        UndoTree_PrepareNewNode_MALLOC("Destroy");
    } else {
        UndoTree_PrepareNewNode_MALLOC("Place");
    }
 
    UndoTree_AddBlockChangeEntry(coords.X, coords.Y, coords.Z, block - oldBlock);
    UndoTree_Commit();
}

static void AddNode_MALLOC(UndoNode node) {
    if (s_NodesCount >= s_NodesCapacity) {
        size_t newCapacity = (s_NodesCapacity + 1) * 2;
        UndoNode* newNodes = Memory_Reallocate(s_Nodes, newCapacity * sizeof(UndoNode));

        if (Memory_AllocationError()) {
            return;
        }

        s_NodesCapacity = newCapacity;
        s_Nodes = newNodes;
    }

    s_Nodes[s_NodesCount] = node;
    s_NodesCount += 1;
    return;
}

bool UndoTree_Enable_MALLOC(void) {
    if (s_Enabled) {
        return false;
    }

    UndoNode root;
    root.commit = 0;
    root.description[0] = '#';
    root.description[1] = '\0';
    root.blockDeltasCount = 0;
    root.blockDeltasCapacity = 0;
    root.blockDeltas = NULL;
    root.timestamp = time(NULL);
    root.parentIndex = 0;
    root.children = NULL;
    root.childrenCount = 0;
    root.childrenCapacity = 0;

    AddNode_MALLOC(root);

    if (Memory_AllocationError()) {
        return false;
    }

    Event_Register((struct Event_Void*) &UserEvents.BlockChanged, NULL, (Event_Void_Callback)OnBlockChanged);
    s_Enabled = true;

    return true;
}

static void FreeUndoNode(UndoNode* node) {
    free(node->blockDeltas);
    free(node->children);
}

void UndoTree_Disable(void) {
    if (!s_Enabled) {
        return;
    }

    for (size_t i = 0; i < s_NodesCount; i++) {
        FreeUndoNode(&s_Nodes[i]);
    }

    s_CurrentNodeIndex = 0;

    free(s_Nodes);
    s_Nodes = NULL;
    s_NodesCount = 0;
    s_NodesCapacity = 0;

    free(s_RedoStack);
    s_RedoStack = NULL;
    s_RedoStackCount = 0;
    s_RedoStackCapacity = 0;

    Event_Unregister((struct Event_Void*) &UserEvents.BlockChanged, NULL, (Event_Void_Callback)OnBlockChanged);
    s_Enabled = false;
}

static void Descend(UndoNode* child) {
    // Note: `child` must be a direct child of `s_CurrentNodeIndex`, otherwise the behaviour is undefined.
    s_CurrentNodeIndex = child->commit;
    BlockChangeEntry* entries = s_Nodes[s_CurrentNodeIndex].blockDeltas;
    BlockID currentBlock; 

    for (int i = 0; i < s_Nodes[s_CurrentNodeIndex].blockDeltasCount; i++) {
        currentBlock = World_GetBlock(entries[i].x, entries[i].y, entries[i].z);
        Game_UpdateBlock(entries[i].x, entries[i].y, entries[i].z, currentBlock + entries[i].delta);
    }
}

static void Ascend(void) {
    if (s_CurrentNodeIndex == 0) {
        return;
    }

    BlockChangeEntry* entries = s_Nodes[s_CurrentNodeIndex].blockDeltas;
    BlockID currentBlock; 

    for (int i = 0; i < s_Nodes[s_CurrentNodeIndex].blockDeltasCount; i++) {
        currentBlock = World_GetBlock(entries[i].x, entries[i].y, entries[i].z);
        Game_UpdateBlock(entries[i].x, entries[i].y, entries[i].z, currentBlock - entries[i].delta);
    }

    s_CurrentNodeIndex = s_Nodes[s_CurrentNodeIndex].parentIndex;
}

static bool Checkout_MALLOC(int target, int* ascended, int* descended) {
    if (ascended != NULL && descended != NULL) {
        *ascended = 0;
        *descended = 0;
    }

    // Calculate the ancestors of the target.
    List* targetAncestors = List_CreateEmpty_MALLOC();

    if (Memory_AllocationError()) {
        return false;
    }

    UndoNode* ancestor = &s_Nodes[target];

    while (true) {
        List_Append_MALLOC(targetAncestors, ancestor);

        if (Memory_AllocationError()) {
            List_Free(targetAncestors);
            return false;
        }

        if (ancestor->commit == 0) {
            break;
        }

        ancestor = &s_Nodes[ancestor->parentIndex];
    }

    // Ascend while the current node is not an ancestor of the target.
    while (!List_Contains(targetAncestors, &s_Nodes[s_CurrentNodeIndex])) {
        Ascend();
        
        if (ascended != NULL) {
            (*ascended)++;
        }
    }

    // Remove all ancestors above `s_CurrentNodeIndex` until last element of `targetAncestors` is a child of `s_CurrentNodeIndex`.
    do {
        ancestor = (UndoNode*) List_Pop(targetAncestors);
    } while (ancestor->commit != s_CurrentNodeIndex);

    // Then, descend to the target.
    while (s_CurrentNodeIndex != target) {
        Descend((UndoNode*) List_Pop(targetAncestors));
        
        if (descended != NULL) {
            (*descended)++;
        }
    }

    List_Free(targetAncestors);
    return true;
}

static void StackRedo_MALLOC(int commit) {
    if (s_RedoStackCount >= s_RedoStackCapacity) {
        size_t newCapacity = (s_RedoStackCapacity + 1) * 2;
        int* newRedoStack = Memory_Reallocate(s_RedoStack, newCapacity * sizeof(int));

        if (Memory_AllocationError()) {
            return;
        }

        s_RedoStackCapacity = newCapacity;
        s_RedoStack = newRedoStack;
    }

    s_RedoStack[s_RedoStackCount] = commit;
    s_RedoStackCount++;
}

bool UndoTree_Earlier_MALLOC(int deltaTimeSeconds, int* out_commit) {
    if (!s_Enabled) {
        return false;
    }

    if (deltaTimeSeconds <= 0) {
        return false;
    }

    int newIndex = 0;

    for (int i = s_CurrentNodeIndex; i > 0; i--) {
        if (s_Nodes[s_CurrentNodeIndex].timestamp - s_Nodes[i].timestamp > deltaTimeSeconds) {
            newIndex = i;
            break;
        }
    }

    if (newIndex == s_CurrentNodeIndex) {
        return false;
    }

    StackRedo_MALLOC(s_CurrentNodeIndex);

    if (Memory_AllocationError()) {
        return false;
    }

    Checkout_MALLOC(newIndex, NULL, NULL);

    if (Memory_AllocationError()) {
        return false;
    }

    *out_commit = s_CurrentNodeIndex;
    return true;
}

bool UndoTree_Later_MALLOC(int deltaTimeSeconds, int* commit) {
    if (!s_Enabled) {
        return false;
    }

    if (deltaTimeSeconds <= 0) {
        return false;
    }

    int newIndex = s_NodesCount - 1;

    for (size_t i = s_CurrentNodeIndex; i < s_NodesCount - 1; i++) {
        if (s_Nodes[i].timestamp - s_Nodes[s_CurrentNodeIndex].timestamp > deltaTimeSeconds) {
            newIndex = (int)i;
            break;
        }
    }

    if (newIndex == s_CurrentNodeIndex) {
        return false;
    }

    StackRedo_MALLOC(s_CurrentNodeIndex);

    if (Memory_AllocationError()) {
        return false;
    }

    Checkout_MALLOC(newIndex, NULL, NULL);

    if (Memory_AllocationError()) {
        return false;
    }
        
    *commit = s_CurrentNodeIndex;
    return true;
}

bool UndoTree_Undo_MALLOC(void) {
    if (!s_Enabled || s_CurrentNodeIndex == 0) {
        return false;
    }

    StackRedo_MALLOC(s_CurrentNodeIndex);

    if (Memory_AllocationError()) {
        return false;
    }

    Ascend();
    return true;
}

bool UndoTree_Checkout_MALLOC(int commit, int* ascended, int* descended) {
    if (!s_Enabled) {
        return false;
    }

    if (commit < 0 || s_NodesCount <= (size_t)commit) {
        return false;
    }

    StackRedo_MALLOC(s_CurrentNodeIndex);

    if (Memory_AllocationError()) {
        return false;
    }

    Checkout_MALLOC(commit, ascended, descended);
    return true;
}

bool UndoTree_Redo_MALLOC(void) {
    if (!s_Enabled || s_RedoStackCount == 0) {
        return false;
    }

    int nodeIndexTarget = s_RedoStack[s_RedoStackCount - 1];
    s_RedoStackCount--;
    Checkout_MALLOC(nodeIndexTarget, NULL, NULL);
    return true;
}

static void AddChildren_MALLOC(void) {
    UndoNode* here = &s_Nodes[s_CurrentNodeIndex];

    if (here->childrenCount >= here->childrenCapacity) {
        size_t newCapacity = (here->childrenCapacity + 1) * 2;
        int* newChildren = Memory_Reallocate(here->children, newCapacity * sizeof(int));

        if (Memory_AllocationError()) {
            return;
        }

        here->childrenCapacity = newCapacity;
        here->children = newChildren;
    }

    here->children[here->childrenCount] = s_NodesCount;
    here->childrenCount++;
}

bool UndoTree_PrepareNewNode_MALLOC(char* description) {
    if (!s_Enabled) {
        return false;
    }

    UndoNode newNode;

    newNode.commit = s_NodesCount;
    strncpy(newNode.description, description, sizeof(newNode.description));
    newNode.blockDeltas = NULL;
    newNode.blockDeltasCount = 0;
    newNode.blockDeltasCapacity = 0;
    newNode.parentIndex = s_CurrentNodeIndex;
    newNode.children = NULL;
    newNode.childrenCount = 0;
    newNode.childrenCapacity = 0;
    newNode.timestamp = time(NULL);

    AddNode_MALLOC(newNode);

    if (Memory_AllocationError()) {
        return false;
    }

    AddChildren_MALLOC();

    if (Memory_AllocationError()) {
        s_NodesCount--;
        return false;
    }

    s_CurrentNodeIndex = newNode.commit;
    return true;
}

void UndoTree_AddBlockChangeEntry(int x, int y, int z, DeltaBlockID delta) {
    if (!s_Enabled) {
        return;
    }

    BlockChangeEntry entry = {
        .x = x,
        .y = y,
        .z = z,
        .delta = delta
    };

    s_Nodes[s_CurrentNodeIndex].blockDeltasCount += 1;

    if (s_Nodes[s_CurrentNodeIndex].blockDeltasCount > s_Nodes[s_CurrentNodeIndex].blockDeltasCapacity) {
        s_Nodes[s_CurrentNodeIndex].blockDeltasCapacity = (s_Nodes[s_CurrentNodeIndex].blockDeltasCapacity + 1) * 2;

        BlockChangeEntry* newEntries = Memory_Reallocate(s_Nodes[s_CurrentNodeIndex].blockDeltas, s_Nodes[s_CurrentNodeIndex].blockDeltasCapacity * sizeof(BlockChangeEntry));

        if (newEntries == NULL) {
            // TODO: Don't do that
            exit(1);
        }

        s_Nodes[s_CurrentNodeIndex].blockDeltas = newEntries;
    }

    s_Nodes[s_CurrentNodeIndex].blockDeltas[s_Nodes[s_CurrentNodeIndex].blockDeltasCount - 1] = entry;
}

void UndoTree_Commit(void) {
    if (!s_Enabled) {
        return;
    }

    if (s_Nodes[s_CurrentNodeIndex].blockDeltasCount == 0) {
        s_NodesCount--;
        s_CurrentNodeIndex = s_Nodes[s_CurrentNodeIndex].parentIndex;
        return;
    }

    free(s_RedoStack);
    s_RedoStack = NULL;
    s_RedoStackCount = 0;
    s_RedoStackCapacity = 0;
}

static void FormatNode(cc_string* destination, UndoNode* node) {
    char buffer_result[STRING_SIZE];
    cc_string result = String_FromArray(buffer_result);

    char buffer_currentNodePrefix[] = { '&', 'b', 0x10, ' ', '&', 'f', '\0' };
    cc_string currentNodePrefix = { buffer_currentNodePrefix, .length = 6, .capacity = 6 };

    char buffer_formattedTime[STRING_SIZE];
    cc_string formattedTime = String_FromArray(buffer_formattedTime);

    char buffer_formattedBlocks[STRING_SIZE];
    cc_string formattedBlocks = String_FromArray(buffer_formattedBlocks);

    char buffer_formattedCommit[STRING_SIZE];
    cc_string formattedCommit = String_FromArray(buffer_formattedCommit);

    Format_HHMMSS(&formattedTime, node->timestamp);
    Format_Int32(&formattedBlocks, node->blockDeltasCount);
    Format_Int32(&formattedCommit, node->commit);

    String_Format4(&result, "[&b%s&f] %c @ %s (%s)", &formattedCommit, node->description, &formattedTime, &formattedBlocks);

    if (node->commit == s_CurrentNodeIndex) {
        String_AppendString(destination, &currentNodePrefix);
    }

    String_AppendString(destination, &result);
}

void UndoTree_UndoList(cc_string* descriptions, int* count) {
    const int max = 5;

    UndoNode* terminalNodes[max];
    *count = 0;

    for (int i = s_NodesCount - 1; i >= 0; i--) {
        if (s_Nodes[i].childrenCount == 0) {
            terminalNodes[*count] = &s_Nodes[i];
            *count += 1;
        }

        if (*count == max) {
            break;
        }
    }

    UndoNode* currentNode;

    for (int i = 0; i < *count; i++) {
        currentNode = terminalNodes[i];
        FormatNode(&descriptions[i], currentNode);
    }
}

long UndoTree_CurrentTimestamp(void) {
    return s_Nodes[s_CurrentNodeIndex].timestamp;
}

void UndoTree_FormatCurrentNode(cc_string* destination) {
    UndoNode* currentNode = &s_Nodes[s_CurrentNodeIndex];
    FormatNode(destination, currentNode);
}
