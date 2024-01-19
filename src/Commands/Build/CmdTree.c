#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "ClassiCube/src/Chat.h"
#include "ClassiCube/src/Core.h"
#include "ClassiCube/src/Game.h"
#include "ClassiCube/src/Inventory.h"
#include "ClassiCube/src/BlockID.h"

#include "MarkSelection.h"
#include "Messaging.h"
#include "ParsingUtils.h"
#include "Draw.h"

static bool s_Repeat = false;

static void Tree_Command(const cc_string* args, int argsCount);

struct ChatCommand TreeCommand = {
	"Tree",
	Tree_Command,
	COMMAND_FLAG_SINGLEPLAYER_ONLY,
	{
		"&b/Tree [+]",
        "&fGrows a tree.",
        NULL,
        NULL,
		NULL
	},
	NULL
};

static void DrawTrunk(int x, int y, int z) {
    for (int i = 0; i < 6; i++) {
        Draw_Block(x, y + i, z, BLOCK_LOG);
    }
}

static void DrawLeavesLayer1(int x, int y, int z) {
    int height = y + 3;

    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            if (i == 0 && j == 0) continue;
            if (i == -2 && j == -2) continue;
            if (i == -2 && j == 2) continue;
            if (i == 2 && j == 2) continue;
            if (i == 2 && j == -2) continue;
            Draw_Block(x + i, height, z + j, BLOCK_LEAVES);
        }
    }
}

static void DrawLeavesLayer2(int x, int y, int z) {
    DrawLeavesLayer1(x, y + 1, z);
}

static void DrawLeavesLayer3(int x, int y, int z) {
    int height = y + 5;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            Draw_Block(x + i, height, z + j, BLOCK_LEAVES);
        }
    }
}

static void DrawLeavesLayer4(int x, int y, int z) {
    int height = y + 6;

    Draw_Block(x, height, z, BLOCK_LEAVES);
    Draw_Block(x + 1, height, z, BLOCK_LEAVES);
    Draw_Block(x - 1, height, z, BLOCK_LEAVES);
    Draw_Block(x, height, z + 1, BLOCK_LEAVES);
    Draw_Block(x, height, z - 1, BLOCK_LEAVES);
}

static void TreeSelectionHandler(IVec3* marks, int count) {
    if (count != 1) {
        return;
    }

    int x = marks[0].X;
    int y = marks[0].Y;
    int z = marks[0].Z;

    Draw_Start("Tree");
    DrawTrunk(x, y, z);
    DrawLeavesLayer1(x, y, z);
    DrawLeavesLayer2(x, y, z);
    DrawLeavesLayer3(x, y, z);
    DrawLeavesLayer4(x, y, z);
    int blocksAffected = Draw_End();
    Message_BlocksAffected(blocksAffected);

    if (s_Repeat) {
        MarkSelection_Make(TreeSelectionHandler, 1);
        Message_Player("Place or break a block to determine the root.");
    }
}

static void Tree_Command(const cc_string* args, int argsCount) {
    s_Repeat = Parse_LastArgumentIsRepeat(args, &argsCount);

    if (argsCount > 0) {
        Message_CommandUsage(TreeCommand);
        return;
    }

    if (s_Repeat) {
		Message_Player("Now repeating &bTree&f.");
	}

    MarkSelection_Make(TreeSelectionHandler, 1);
    Message_Player("Place or break a block to determine the root.");
}