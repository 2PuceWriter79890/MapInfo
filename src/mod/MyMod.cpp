#include "mod/MyMod.h"

#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"

#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/saveddata/maps/MapItemSavedData.h"
#include "mc/world/level/BlockPos.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/nbt/CompoundTagVariant.h"
#include "mc/nbt/Int64Tag.h"
#include "mc/nbt/IntTag.h"
#include "mc/nbt/ByteTag.h"

namespace map_info {

enum class SubCommand_Get {
    get
};
enum class SubCommand_Snbt {
    snbt
};

struct Command_Get {
    SubCommand_Get subcommand;
};
struct Command_Snbt {
    SubCommand_Snbt subcommand;
};


MapInfoMod& MapInfoMod::getInstance() {
    static MapInfoMod instance;
    return instance;
}

bool MapInfoMod::load() {
    getSelf().getLogger().info("加载 MapInfoMod 中...");
    return true;
}

bool MapInfoMod::enable() {
    getSelf().getLogger().info("启用 MapInfoMod 中...");
    registerCommand();
    return true;
}

bool MapInfoMod::disable() {
    getSelf().getLogger().info("禁用 MapInfoMod 中...");
    return true;
}

void MapInfoMod::registerCommand() {
    auto& registrar = ll::command::CommandRegistrar::getInstance();

    registrar.tryRegisterEnum<SubCommand_Get>();
    registrar.tryRegisterEnum<SubCommand_Snbt>();

    auto& command = registrar.getOrCreateCommand("mapinfo", "获取手持地图的信息", CommandPermissionLevel::Any);

    command.overload<Command_Get>()
        .execute(
            [this](const ::CommandOrigin& origin, ::CommandOutput& output, const Command_Get& params) {
                auto* player = origin.getEntity();
                if (!player || !player->isPlayer()) {
                    output.error("该指令只能由玩家执行");
                    return;
                }
                const ItemStack& itemInHand = player->getCarriedItem();
                if (itemInHand.isNull()) {
                    output.error("你的手上没有任何东西");
                    return;
                }
                const auto& rawNameId = itemInHand.getRawNameId();
                if (rawNameId != "filled_map" && rawNameId != "map") {
                    output.error("请手持地图，当前手持物品：§r" + rawNameId);
                    return;
                }
                if (rawNameId == "map") {
                    output.success(
                        "§b--- 空地图信息 ---\n"
                        "§6地图 ID: §f" + std::to_string(itemInHand.getAuxValue()) + " (Potential)\n"
                        "§7此地图没有任何数据\n"
                        "§b----------------"
                    );
                    return;
                }
                long long mapIdValue = -1;
                bool mapIsScaling = false;
                int mapNameIndex = -1;
                const CompoundTag* nbt = itemInHand.mUserData.get();
                if (nbt && nbt->contains("map_uuid")) {
                    mapIdValue = nbt->at("map_uuid").get<Int64Tag>().data;
                }
                if (nbt && nbt->contains("map_is_scaling")) {
                    mapIsScaling = (nbt->at("map_is_scaling").get<ByteTag>().data != 0);
                }
                if (nbt && nbt->contains("map_name_index")) {
                    mapNameIndex = nbt->at("map_name_index").get<IntTag>().data;
                }
                if (mapIdValue == -1) {
                    output.error("无法从 NBT 数据中找到 uuid");
                    return;
                }
                ActorUniqueID mapId{mapIdValue};
                auto& level = player->getLevel();
                auto* mapData = level.getMapSavedData(mapId);
                if (!mapData) {
                    output.error(
                        "无法从存档数据中找到该地图，地图 ID ：§r" + std::to_string(mapIdValue) +
                        "§e如果这是新创建的地图，可能需要过几分钟重试"
                    );
                    return;
                }
                std::string mapIdStr = std::to_string(mapIdValue);
                std::string scaleStr = std::to_string(mapData->mScale);
                std::string lockedStr = mapData->mLocked ? "§c是" : "§a否";
                std::string centerPosStr = "(" + std::to_string(mapData->mOrigin->x) + ", " + std::to_string(mapData->mOrigin->z) + ")";
                std::string scalingStr = mapIsScaling ? "§c是" : "§a否";
                std::string nameIndexStr = (mapNameIndex != -1) ? std::to_string(mapNameIndex) : "N/A";
                output.success(
                    "§b--- 地图信息 ---\n"
                    "§6地图 ID ： §f" + mapIdStr + "\n"
                    "§6地图等级： §f" + scaleStr + "\n"
                    "§6中心坐标 (X, Z)： §f" + centerPosStr + "\n"
                    "§6是否上锁： " + lockedStr + "\n"
                    "§6是否缩放： " + scalingStr + "\n"
                    "§6名称索引： §f" + nameIndexStr + "\n"
                    "§b----------------"
                );
            }
        );


    command.overload<Command_Snbt>()
        .execute(
            [this](const ::CommandOrigin& origin, ::CommandOutput& output, const Command_Snbt& params) {
                auto* player = origin.getEntity();
                if (!player || !player->isPlayer()) {
                    output.error("该指令只能由玩家执行");
                    return;
                }

                const ItemStack& itemInHand = player->getCarriedItem();
                if (itemInHand.isNull()) {
                    output.error("你的手上没有任何东西");
                    return;
                }
                
                const auto& rawNameId = itemInHand.getRawNameId();
                if (rawNameId != "filled_map" && rawNameId != "map") {
                    output.error("请手持地图，当前手持物品：§r" + rawNameId);
                    return;
                }
                
                if (itemInHand.mUserData) {
                    std::string snbt = itemInHand.mUserData->toString();
                    output.success("Map SNBT Data:\n" + snbt);
                } else {
                    output.success("获取 SNBT 数据失败");
                }
            }
        );

    getSelf().getLogger().info("'/mapinfo' 命令已注册！");
    getSelf().getLogger().info("作者：Puce");
}

} // namespace map_info

LL_REGISTER_MOD(map_info::MapInfoMod, map_info::MapInfoMod::getInstance());
