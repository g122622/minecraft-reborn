#include "VanillaResources.hpp"
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 模型模板定义
// ============================================================================

// 单面纹理方块模型（所有面使用同一纹理）
const char* VanillaResources::MODEL_CUBE_ALL = R"({
    "parent": "block/cube",
    "textures": {
        "particle": "#all",
        "down": "#all",
        "up": "#all",
        "north": "#all",
        "east": "#all",
        "south": "#all",
        "west": "#all"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#down", "cullface": "down"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#up", "cullface": "up"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#north", "cullface": "north"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#south", "cullface": "south"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#west", "cullface": "west"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#east", "cullface": "east"}
            }
        }
    ]
})";

// 柱状方块模型（上下和侧面不同纹理）
const char* VanillaResources::MODEL_CUBE_COLUMN = R"({
    "parent": "block/cube",
    "textures": {
        "particle": "#side",
        "down": "#end",
        "up": "#end",
        "north": "#side",
        "east": "#side",
        "south": "#side",
        "west": "#side"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#down", "cullface": "down"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#up", "cullface": "up"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#north", "cullface": "north"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#south", "cullface": "south"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#west", "cullface": "west"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#east", "cullface": "east"}
            }
        }
    ]
})";

// 六面不同纹理的基础方块模型
const char* VanillaResources::MODEL_CUBE = R"({
    "textures": {
        "particle": "#north"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#down", "cullface": "down"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#up", "cullface": "up"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#north", "cullface": "north"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#south", "cullface": "south"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#west", "cullface": "west"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#east", "cullface": "east"}
            }
        }
    ]
})";

// 树叶模型
const char* VanillaResources::MODEL_LEAVES = R"({
    "textures": {
        "particle": "#all"
    },
    "elements": [
        {
            "from": [0, 0, 0],
            "to": [16, 16, 16],
            "faces": {
                "down": {"uv": [0, 0, 16, 16], "texture": "#all"},
                "up": {"uv": [0, 0, 16, 16], "texture": "#all"},
                "north": {"uv": [0, 0, 16, 16], "texture": "#all"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#all"},
                "west": {"uv": [0, 0, 16, 16], "texture": "#all"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#all"}
            }
        }
    ]
})";

// 交叉纹理模型
const char* VanillaResources::MODEL_CROSS = R"({
    "textures": {
        "particle": "#cross"
    },
    "elements": [
        {
            "from": [0.8, 0, 8],
            "to": [15.2, 16, 8],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "north": {"uv": [0, 0, 16, 16], "texture": "#cross"},
                "south": {"uv": [0, 0, 16, 16], "texture": "#cross"}
            }
        },
        {
            "from": [8, 0, 0.8],
            "to": [8, 16, 15.2],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "west": {"uv": [0, 0, 16, 16], "texture": "#cross"},
                "east": {"uv": [0, 0, 16, 16], "texture": "#cross"}
            }
        }
    ]
})";

// 染色交叉纹理模型
const char* VanillaResources::MODEL_TINTED_CROSS = R"({
    "textures": {
        "particle": "#cross"
    },
    "elements": [
        {
            "from": [0.8, 0, 8],
            "to": [15.2, 16, 8],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "north": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0},
                "south": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0}
            }
        },
        {
            "from": [8, 0, 0.8],
            "to": [8, 16, 15.2],
            "rotation": {"origin": [8, 8, 8], "axis": "y", "angle": 45, "rescale": true},
            "faces": {
                "west": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0},
                "east": {"uv": [0, 0, 16, 16], "texture": "#cross", "tintindex": 0}
            }
        }
    ]
})";

// 空气模型（不可见）
const char* VanillaResources::MODEL_AIR = R"({
    "textures": {},
    "elements": []
})";

// ============================================================================
// 创建资源包
// ============================================================================

std::unique_ptr<InMemoryResourcePack> VanillaResources::createResourcePack() {
    auto pack = std::make_unique<InMemoryResourcePack>("vanilla");

    registerBaseModels(*pack);
    registerBlockStates(*pack);

    return pack;
}

// ============================================================================
// 注册基础模型
// ============================================================================

void VanillaResources::registerBaseModels(InMemoryResourcePack& pack) {
    // 基础方块模型（无父模型）
    pack.addResource("assets/minecraft/models/block/cube.json", MODEL_CUBE);

    // 单面纹理方块
    pack.addResource("assets/minecraft/models/block/cube_all.json", MODEL_CUBE_ALL);

    // 柱状方块
    pack.addResource("assets/minecraft/models/block/cube_column.json", MODEL_CUBE_COLUMN);

    // 树叶
    pack.addResource("assets/minecraft/models/block/leaves.json", MODEL_LEAVES);

    // 交叉纹理
    pack.addResource("assets/minecraft/models/block/cross.json", MODEL_CROSS);
    pack.addResource("assets/minecraft/models/block/tinted_cross.json", MODEL_TINTED_CROSS);

    // 空气
    pack.addResource("assets/minecraft/models/block/air.json", MODEL_AIR);

    // 原版方块的完整模型定义
    // 石头
    pack.addResource("assets/minecraft/models/block/stone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/stone"
        }
    })");

    // 泥土
    pack.addResource("assets/minecraft/models/block/dirt.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/dirt"
        }
    })");

    // 草方块
    pack.addResource("assets/minecraft/models/block/grass_block.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/grass_block_top",
            "down": "block/grass_block_bottom",
            "up": "block/grass_block_top",
            "north": "block/grass_block_side",
            "east": "block/grass_block_side",
            "south": "block/grass_block_side",
            "west": "block/grass_block_side"
        }
    })");

    // 基岩
    pack.addResource("assets/minecraft/models/block/bedrock.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/bedrock"
        }
    })");

    // 沙子
    pack.addResource("assets/minecraft/models/block/sand.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/sand"
        }
    })");

    // 砾石
    pack.addResource("assets/minecraft/models/block/gravel.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/gravel"
        }
    })");

    // 橡木木板
    pack.addResource("assets/minecraft/models/block/oak_planks.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/oak_planks"
        }
    })");

    // 橡木原木
    pack.addResource("assets/minecraft/models/block/oak_log.json", R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/oak_log_top",
            "side": "block/oak_log"
        }
    })");

    // 橡木树叶
    pack.addResource("assets/minecraft/models/block/oak_leaves.json", R"({
        "parent": "block/leaves",
        "textures": {
            "all": "block/oak_leaves"
        }
    })");

    // 圆石
    pack.addResource("assets/minecraft/models/block/cobblestone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/cobblestone"
        }
    })");

    // 苔石圆石
    pack.addResource("assets/minecraft/models/block/mossy_cobblestone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/mossy_cobblestone"
        }
    })");

    // 各类矿石
    pack.addResource("assets/minecraft/models/block/coal_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/coal_ore"
        }
    })");

    pack.addResource("assets/minecraft/models/block/iron_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/iron_ore"
        }
    })");

    pack.addResource("assets/minecraft/models/block/gold_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/gold_ore"
        }
    })");

    pack.addResource("assets/minecraft/models/block/diamond_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/diamond_ore"
        }
    })");

    pack.addResource("assets/minecraft/models/block/emerald_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/emerald_ore"
        }
    })");

    pack.addResource("assets/minecraft/models/block/lapis_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/lapis_ore"
        }
    })");

    pack.addResource("assets/minecraft/models/block/redstone_ore.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/redstone_ore"
        }
    })");

    // 矿物方块
    pack.addResource("assets/minecraft/models/block/diamond_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/diamond_block"
        }
    })");

    pack.addResource("assets/minecraft/models/block/gold_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/gold_block"
        }
    })");

    pack.addResource("assets/minecraft/models/block/iron_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/iron_block"
        }
    })");

    pack.addResource("assets/minecraft/models/block/emerald_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/emerald_block"
        }
    })");

    pack.addResource("assets/minecraft/models/block/lapis_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/lapis_block"
        }
    })");

    pack.addResource("assets/minecraft/models/block/redstone_block.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/redstone_block"
        }
    })");

    // 石头变种
    pack.addResource("assets/minecraft/models/block/granite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/granite"
        }
    })");

    pack.addResource("assets/minecraft/models/block/polished_granite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_granite"
        }
    })");

    pack.addResource("assets/minecraft/models/block/diorite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/diorite"
        }
    })");

    pack.addResource("assets/minecraft/models/block/polished_diorite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_diorite"
        }
    })");

    pack.addResource("assets/minecraft/models/block/andesite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/andesite"
        }
    })");

    pack.addResource("assets/minecraft/models/block/polished_andesite.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_andesite"
        }
    })");

    // 砂岩
    pack.addResource("assets/minecraft/models/block/sandstone.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/sandstone_side",
            "down": "block/sandstone_bottom",
            "up": "block/sandstone_top",
            "north": "block/sandstone_side",
            "east": "block/sandstone_side",
            "south": "block/sandstone_side",
            "west": "block/sandstone_side"
        }
    })");

    // 其他方块
    pack.addResource("assets/minecraft/models/block/bricks.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/bricks"
        }
    })");

    pack.addResource("assets/minecraft/models/block/bookshelf.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/oak_planks",
            "down": "block/oak_planks",
            "up": "block/oak_planks",
            "north": "block/bookshelf",
            "east": "block/bookshelf",
            "south": "block/bookshelf",
            "west": "block/bookshelf"
        }
    })");

    pack.addResource("assets/minecraft/models/block/tnt.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/tnt_side",
            "down": "block/tnt_bottom",
            "up": "block/tnt_top",
            "north": "block/tnt_side",
            "east": "block/tnt_side",
            "south": "block/tnt_side",
            "west": "block/tnt_side"
        }
    })");

    pack.addResource("assets/minecraft/models/block/sponge.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/sponge"
        }
    })");

    pack.addResource("assets/minecraft/models/block/wet_sponge.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/wet_sponge"
        }
    })");

    // 下界方块
    pack.addResource("assets/minecraft/models/block/netherrack.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/netherrack"
        }
    })");

    pack.addResource("assets/minecraft/models/block/glowstone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/glowstone"
        }
    })");

    pack.addResource("assets/minecraft/models/block/end_stone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/end_stone"
        }
    })");

    pack.addResource("assets/minecraft/models/block/obsidian.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/obsidian"
        }
    })");

    pack.addResource("assets/minecraft/models/block/soul_sand.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/soul_sand"
        }
    })");

    pack.addResource("assets/minecraft/models/block/soul_soil.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/soul_soil"
        }
    })");

    pack.addResource("assets/minecraft/models/block/basalt.json", R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/basalt_top",
            "side": "block/basalt_side"
        }
    })");

    pack.addResource("assets/minecraft/models/block/polished_basalt.json", R"({
        "parent": "block/cube_column",
        "textures": {
            "end": "block/polished_basalt_top",
            "side": "block/polished_basalt_side"
        }
    })");

    pack.addResource("assets/minecraft/models/block/blackstone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/blackstone"
        }
    })");

    pack.addResource("assets/minecraft/models/block/polished_blackstone.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/polished_blackstone"
        }
    })");

    pack.addResource("assets/minecraft/models/block/crying_obsidian.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/crying_obsidian"
        }
    })");

    // 冰和雪
    pack.addResource("assets/minecraft/models/block/ice.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/ice"
        }
    })");

    pack.addResource("assets/minecraft/models/block/snow.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/snow"
        }
    })");

    // 水（透明方块）
    pack.addResource("assets/minecraft/models/block/water.json", R"({
        "textures": {
            "particle": "block/water_still"
        },
        "elements": [
            {
                "from": [0, 0, 0],
                "to": [16, 16, 16],
                "faces": {
                    "down": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "up": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "north": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "south": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "west": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "east": {"uv": [0, 0, 16, 16], "texture": "#particle"}
                }
            }
        ]
    })");

    // 岩浆
    pack.addResource("assets/minecraft/models/block/lava.json", R"({
        "textures": {
            "particle": "block/lava_still"
        },
        "elements": [
            {
                "from": [0, 0, 0],
                "to": [16, 16, 16],
                "faces": {
                    "down": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "up": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "north": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "south": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "west": {"uv": [0, 0, 16, 16], "texture": "#particle"},
                    "east": {"uv": [0, 0, 16, 16], "texture": "#particle"}
                }
            }
        ]
    })");

    // 各种木板
    const char* planksTemplate = R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/%s_planks"
        }
    })";

    const char* planksTypes[] = {"spruce", "birch", "jungle", "acacia", "dark_oak"};
    for (const auto& type : planksTypes) {
        String model = String(planksTemplate);
        size_t pos = model.find("%s");
        if (pos != String::npos) {
            model.replace(pos, 2, type);
        }
        pack.addResource("assets/minecraft/models/block/" + String(type) + "_planks.json", model);
    }

    // 羊毛
    const char* woolTemplate = R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/%s_wool"
        }
    })";

    const char* woolColors[] = {"white", "orange", "magenta", "light_blue", "yellow",
                                 "lime", "pink", "gray", "light_gray", "cyan",
                                 "purple", "blue", "brown", "green", "red", "black"};
    for (const auto& color : woolColors) {
        String model = String(woolTemplate);
        size_t pos = model.find("%s");
        if (pos != String::npos) {
            model.replace(pos, 2, color);
        }
        pack.addResource("assets/minecraft/models/block/" + String(color) + "_wool.json", model);
    }

    // 泥土变种
    pack.addResource("assets/minecraft/models/block/coarse_dirt.json", R"({
        "parent": "block/cube_all",
        "textures": {
            "all": "block/coarse_dirt"
        }
    })");

    pack.addResource("assets/minecraft/models/block/podzol.json", R"({
        "parent": "block/cube",
        "textures": {
            "particle": "block/podzol_side",
            "down": "block/dirt",
            "up": "block/podzol_top",
            "north": "block/podzol_side",
            "east": "block/podzol_side",
            "south": "block/podzol_side",
            "west": "block/podzol_side"
        }
    })");
}

// ============================================================================
// 注册 Blockstates
// ============================================================================

void VanillaResources::registerBlockStates(InMemoryResourcePack& pack) {
    // 大多数简单方块使用 normal 变体
    const char* simpleBlockstate = R"({
        "variants": {
            "normal": { "model": "%s" }
        }
    })";

    // 注册所有简单方块的 blockstates
    const char* simpleBlocks[] = {
        "stone", "dirt", "grass_block", "bedrock", "sand", "gravel",
        "cobblestone", "mossy_cobblestone", "oak_planks",
        "coal_ore", "iron_ore", "gold_ore", "diamond_ore", "emerald_ore", "lapis_ore", "redstone_ore",
        "diamond_block", "gold_block", "iron_block", "emerald_block", "lapis_block", "redstone_block",
        "granite", "polished_granite", "diorite", "polished_diorite", "andesite", "polished_andesite",
        "sandstone", "chiseled_sandstone", "cut_sandstone", "red_sandstone",
        "bricks", "bookshelf", "tnt", "sponge", "wet_sponge",
        "netherrack", "glowstone", "end_stone", "obsidian",
        "soul_sand", "soul_soil", "basalt", "polished_basalt", "blackstone", "polished_blackstone", "crying_obsidian",
        "ice", "snow", "water", "lava",
        "coarse_dirt", "podzol",
        "spruce_planks", "birch_planks", "jungle_planks", "acacia_planks", "dark_oak_planks",
        "white_wool", "orange_wool", "magenta_wool", "light_blue_wool", "yellow_wool",
        "lime_wool", "pink_wool", "gray_wool", "light_gray_wool", "cyan_wool",
        "purple_wool", "blue_wool", "brown_wool", "green_wool", "red_wool", "black_wool"
    };

    for (const auto& block : simpleBlocks) {
        String json = String(simpleBlockstate);
        size_t pos = json.find("%s");
        if (pos != String::npos) {
            json.replace(pos, 2, block);
        }
        pack.addResource("assets/minecraft/blockstates/" + String(block) + ".json", json);
    }

    // 橡木原木（有轴属性）
    pack.addResource("assets/minecraft/blockstates/oak_log.json", R"({
        "variants": {
            "axis=y": { "model": "oak_log" },
            "axis=z": { "model": "oak_log", "x": 90 },
            "axis=x": { "model": "oak_log", "x": 90, "y": 90 }
        }
    })");

    // 空气
    pack.addResource("assets/minecraft/blockstates/air.json", R"({
        "variants": {
            "normal": { "model": "air" }
        }
    })");

    // 橡木树叶
    pack.addResource("assets/minecraft/blockstates/oak_leaves.json", R"({
        "variants": {
            "normal": { "model": "oak_leaves" }
        }
    })");
}

} // namespace mc
