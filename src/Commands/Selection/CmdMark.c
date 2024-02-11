#include "ClassiCube/src/World.h"

#include "MarkSelection.h"
#include "Parse.h"
#include "Message.h"
#include "Player.h"

static void Mark_Command(const cc_string* args, int argsCount);

struct ChatCommand MarkCommand = {
    "Mark",
    Mark_Command,
    COMMAND_FLAG_SINGLEPLAYER_ONLY,
    {
        "&b/Mark [x y z]",
        "Places a marker for selections, e.g. &b/Z&f.",
        "You may use parenthesis to indicate relative position.",
        "For example, &b/Mark (0) (2) (0) &fwill mark above you.",
        NULL
    },
    NULL
};

IVec3 SnapToWorldBoundaries(IVec3 coords) {
    if (coords.X < 0) {
        coords.X = 0;
    } else if (coords.X >= World.Width) {
        coords.X = World.Width - 1;
    }

    if (coords.Y < 0) {
        coords.Y = 0;
    } else if (coords.Y >= World.Height) {
        coords.Y = World.Height - 1;
    }

    if (coords.Z < 0) {
        coords.Z = 0;
    } else if (coords.Z >= World.Length) {
        coords.Z = World.Length - 1;
    }

    return coords;
}

static void Mark_Command(const cc_string* args, int argsCount) {
    if (argsCount == 0) {
        MarkSelection_DoMark(SnapToWorldBoundaries(Player_GetPosition()));
        return;
    }

    if (argsCount != 3) {
        Message_CommandUsage(MarkCommand);
        return;
    }

    IVec3 target;
    
    if (!Parse_TryParseCoordinates(args, &target)) {
        return;
    }
    
    MarkSelection_DoMark(SnapToWorldBoundaries(target));
}
